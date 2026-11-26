#include "cloud.h"
#include "web.h"
#include "meter.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <esp_mac.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const uint32_t WATCH_MS = 30000;        // tras un "watch", envía datos en vivo durante 30 s
static const uint32_t LIVE_DATA_MS = 1000;     // lectura cada 1 s mientras alguien mira
static const uint32_t LIVE_WAVE_MS = 2000;     // formas de onda cada 2 s mientras alguien mira
static const uint32_t IDLE_DATA_MS = 60000;    // sin nadie mirando: 1 lectura por minuto
static const uint32_t RETRY_MIN_MS = 5000;
static const uint32_t RETRY_MAX_MS = 60000;
static const size_t   HIST_CHUNK = 100;        // puntos por mensaje de historial

struct CloudCfg {
  bool en;
  char host[96];
  uint16_t port;
  char user[64];
  char pass[96];
};

static CloudCfg s_cfg;                 // solo la usa la tarea de la nube
static CloudCfg s_new_cfg;             // cambio pedido desde la web o la consola
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool s_cfg_pending = false;
static volatile bool s_connected = false;
static volatile bool s_req_status = false;

static char s_id[16];
static char s_base[40];                // "smartvolt/<id>/"
static Preferences s_prefs;
static WiFiClient s_plain;
static WiFiClientSecure s_tls;
static PubSubClient s_mqtt;
static HistPoint s_hist[HIST_LEN];

static volatile uint32_t s_watch_until = 0;
static volatile bool s_hist_req = false;
static volatile bool s_force_data = false;
static char s_cmd_buf[200];
static volatile bool s_cmd_pending = false;

// ----------------------------------------------------------------------------
static void make_id() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  snprintf(s_id, sizeof(s_id), "sv%02x%02x%02x", mac[3], mac[4], mac[5]);
  snprintf(s_base, sizeof(s_base), "smartvolt/%s/", s_id);
}

static String topic(const char *sub) { return String(s_base) + sub; }

void cloud_configure(const char *host, uint16_t port, const char *user, const char *pass) {
  portENTER_CRITICAL(&s_mux);
  s_new_cfg.en = true;
  strlcpy(s_new_cfg.host, host, sizeof(s_new_cfg.host));
  s_new_cfg.port = port;
  strlcpy(s_new_cfg.user, user, sizeof(s_new_cfg.user));
  strlcpy(s_new_cfg.pass, pass, sizeof(s_new_cfg.pass));
  s_cfg_pending = true;
  portEXIT_CRITICAL(&s_mux);
}

void cloud_disable() {
  portENTER_CRITICAL(&s_mux);
  memset(&s_new_cfg, 0, sizeof(s_new_cfg));
  s_new_cfg.en = false;
  s_cfg_pending = true;
  portEXIT_CRITICAL(&s_mux);
}

CloudInfo cloud_get_info() {
  CloudInfo c;
  portENTER_CRITICAL(&s_mux);
  const CloudCfg &src = s_cfg_pending ? s_new_cfg : s_cfg;
  c.enabled = src.en;
  strlcpy(c.host, src.host, sizeof(c.host));
  c.port = src.port;
  strlcpy(c.user, src.user, sizeof(c.user));
  portEXIT_CRITICAL(&s_mux);
  c.connected = s_connected;
  strlcpy(c.id, s_id, sizeof(c.id));
  c.page = CLOUD_PAGE_URL;
  return c;
}

void cloud_request_status() { s_req_status = true; }

// ----------------------------------------------------------------------------
static void print_status() {
  if (!s_cfg.en) {
    Serial.printf("[NUBE] Desactivada. Código del medidor: %s\n", s_id);
    Serial.println("[NUBE] Para activarla: nube <servidor> <usuario> <clave>   (o 'nube publica' para probar)");
    return;
  }
  Serial.printf("[NUBE] %s a %s:%u (usuario '%s') | código %s\n",
                s_connected ? "Conectada" : "Sin conexión", s_cfg.host, s_cfg.port, s_cfg.user, s_id);
  Serial.printf("[NUBE] Página remota: %s\n", CLOUD_PAGE_URL);
}

static void load_cfg() {
  s_prefs.begin("cloud", false);
  memset(&s_cfg, 0, sizeof(s_cfg));
  s_cfg.port = 8883;
  if (!s_prefs.isKey("host")) return;   // primera vez: nada guardado
  s_cfg.en = s_prefs.getBool("en", false);
  s_prefs.getString("host", s_cfg.host, sizeof(s_cfg.host));
  s_cfg.port = s_prefs.getUShort("port", 8883);
  s_prefs.getString("user", s_cfg.user, sizeof(s_cfg.user));
  s_prefs.getString("pass", s_cfg.pass, sizeof(s_cfg.pass));
  if (!s_cfg.host[0]) s_cfg.en = false;
}

static void save_cfg() {
  s_prefs.putBool("en", s_cfg.en);
  s_prefs.putString("host", s_cfg.host);
  s_prefs.putUShort("port", s_cfg.port);
  s_prefs.putString("user", s_cfg.user);
  s_prefs.putString("pass", s_cfg.pass);
}

// Llega una orden desde la página remota (se ejecuta fuera del callback)
static void on_message(char *t, byte *payload, unsigned int len) {
  if (len >= sizeof(s_cmd_buf)) return;
  char msg[sizeof(s_cmd_buf)];
  memcpy(msg, payload, len);
  msg[len] = '\0';
  if (strcmp(msg, "watch") == 0) {
    bool was_idle = (int32_t)(s_watch_until - millis()) <= 0;
    s_watch_until = millis() + WATCH_MS;
    if (was_idle) s_force_data = true;
  } else if (strcmp(msg, "history") == 0) {
    s_hist_req = true;
  } else if (!s_cmd_pending) {
    memcpy(s_cmd_buf, msg, len + 1);
    s_cmd_pending = true;
  }
}

static bool publish_str(const char *sub, const String &s, bool retained) {
  return s_mqtt.publish(topic(sub).c_str(), (const uint8_t *)s.c_str(), s.length(), retained);
}

static void publish_history() {
  size_t n = meter_copy_history(s_hist, HIST_LEN);
  String chunk;
  chunk.reserve(HIST_CHUNK * 34 + 16);
  char item[64];
  for (size_t start = 0; start < n; start += HIST_CHUNK) {
    chunk = "{\"pts\":[";
    size_t end = min(n, start + HIST_CHUNK);
    for (size_t k = start; k < end; k++) {
      snprintf(item, sizeof(item), "%s[%lu,%.1f,%.3f,%.1f]", k > start ? "," : "",
               (unsigned long)s_hist[k].t, s_hist[k].v, s_hist[k].i, s_hist[k].p);
      chunk += item;
    }
    chunk += "]}";
    publish_str("hist", chunk, false);
    s_mqtt.loop();
  }
}

static bool mqtt_connect() {
  bool tls = (s_cfg.port == 8883 || s_cfg.port == 443);
  if (tls) {
    // Cifrado TLS sin verificar el certificado del servidor (simple y suficiente para empezar)
    s_tls.setInsecure();
    s_mqtt.setClient(s_tls);
  } else {
    s_mqtt.setClient(s_plain);
  }
  s_mqtt.setServer(s_cfg.host, s_cfg.port);

  char cid[32];
  snprintf(cid, sizeof(cid), "%s-%04x", s_id, (unsigned)(esp_random() & 0xFFFF));
  String will = topic("status");
  bool ok = s_mqtt.connect(cid, s_cfg.user[0] ? s_cfg.user : NULL, s_cfg.user[0] ? s_cfg.pass : NULL,
                           will.c_str(), 1, true, "offline");
  if (!ok) return false;

  s_mqtt.publish(will.c_str(), "online", true);
  s_mqtt.subscribe(topic("cmd").c_str());
  publish_str("data", web_data_json(), true);
  return true;
}

// Texto del error de PubSubClient
static const char *state_text(int st) {
  switch (st) {
    case -4: return "el servidor no respondió a tiempo";
    case -3: return "se perdió la conexión";
    case -2: return "no se pudo abrir la conexión (¿servidor o puerto mal escritos?)";
    case 4:  return "usuario o clave incorrectos";
    case 5:  return "no autorizado";
    default: return "error de conexión";
  }
}

// ----------------------------------------------------------------------------
static void cloud_task(void *) {
  load_cfg();
  s_mqtt.setBufferSize(4096);
  s_mqtt.setKeepAlive(30);
  s_mqtt.setSocketTimeout(10);
  s_mqtt.setCallback(on_message);

  uint32_t last_try = 0, retry_ms = RETRY_MIN_MS;
  uint32_t last_data = 0, last_wave = 0;
  bool first_print = true;

  while (true) {
    // Cambio de configuración pedido desde la web o la consola
    if (s_cfg_pending) {
      portENTER_CRITICAL(&s_mux);
      s_cfg = s_new_cfg;
      s_cfg_pending = false;
      portEXIT_CRITICAL(&s_mux);
      save_cfg();
      if (s_mqtt.connected()) {
        s_mqtt.publish(topic("status").c_str(), "offline", true);
        s_mqtt.disconnect();
      }
      s_connected = false;
      last_try = 0;
      retry_ms = RETRY_MIN_MS;
      Serial.println(s_cfg.en ? "[NUBE] Configuración guardada. Conectando..." : "[NUBE] Desactivada.");
    }

    if (s_req_status) { s_req_status = false; print_status(); }

    if (!s_cfg.en || WiFi.status() != WL_CONNECTED) {
      if (s_mqtt.connected()) s_mqtt.disconnect();
      s_connected = false;
      vTaskDelay(pdMS_TO_TICKS(500));
      continue;
    }

    if (!s_mqtt.connected()) {
      s_connected = false;
      if (last_try == 0 || millis() - last_try >= retry_ms) {
        last_try = millis();
        if (first_print) Serial.printf("[NUBE] Conectando a %s:%u...\n", s_cfg.host, s_cfg.port);
        if (mqtt_connect()) {
          s_connected = true;
          retry_ms = RETRY_MIN_MS;
          first_print = true;
          last_data = millis();
          Serial.printf("[NUBE] Conectada. Código del medidor: %s\n", s_id);
          Serial.printf("[NUBE] Desde internet: %s\n", CLOUD_PAGE_URL);
        } else {
          if (first_print) Serial.printf("[NUBE] No conectó: %s. Reintentando...\n", state_text(s_mqtt.state()));
          first_print = false;
          retry_ms = min(retry_ms * 2, RETRY_MAX_MS);
        }
      }
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    s_mqtt.loop();

    // Orden recibida desde la página remota
    if (s_cmd_pending) {
      char cmd[sizeof(s_cmd_buf)];
      strlcpy(cmd, s_cmd_buf, sizeof(cmd));
      s_cmd_pending = false;
      char *q = strchr(cmd, '?');
      if (q) *q++ = '\0';
      String err;
      if (web_apply_action(cmd, q ? q : "", err)) s_force_data = true;
      else Serial.printf("[NUBE] Orden '%s' rechazada: %s\n", cmd, err.c_str());
    }

    uint32_t now = millis();
    bool watching = (int32_t)(s_watch_until - now) > 0;

    if (s_hist_req) { s_hist_req = false; publish_history(); }

    if (s_force_data || now - last_data >= (watching ? LIVE_DATA_MS : IDLE_DATA_MS)) {
      // Pequeña espera tras una orden para que la medición ya la refleje
      if (s_force_data) vTaskDelay(pdMS_TO_TICKS(150));
      s_force_data = false;
      last_data = now;
      publish_str("data", web_data_json(), true);
    }
    if (watching && now - last_wave >= LIVE_WAVE_MS) {
      last_wave = now;
      publish_str("wave", web_wave_json(), false);
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void cloud_init() {
  make_id();
  // Pila amplia: el saludo TLS necesita bastante memoria
  xTaskCreatePinnedToCore(cloud_task, "cloud_task", 12288, NULL, 1, NULL, 0);  // Core 0
}
