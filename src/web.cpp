#include "web.h"
#include "meter.h"
#include "web_page.h"   // generado desde docs/index.html por tools/embed_page.py
#include "cloud.h"
#include <functional>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *AP_SSID = "SmartVolt";
static const char *AP_PASS = "smartvolt123";   // mínimo 8 caracteres
static const char *MDNS_NAME = "smartvolt";

static WebServer server(80);
static Preferences wprefs;   // espacio propio: "wifi_cfg"

// Cambio de red pedido desde la consola o la página (se aplica en la tarea web)
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool s_wifi_pending = false;
static char s_new_ssid[33];
static char s_new_pass[65];
static volatile bool s_print_ip = false;

static char s_ssid[33] = "";
static HistPoint s_hist_buf[HIST_LEN];   // solo lo usa la tarea web

// ----------------------------------------------------------------------------
void web_set_wifi(const char *ssid, const char *pass) {
  portENTER_CRITICAL(&s_mux);
  strncpy(s_new_ssid, ssid, sizeof(s_new_ssid) - 1);
  s_new_ssid[sizeof(s_new_ssid) - 1] = '\0';
  strncpy(s_new_pass, pass, sizeof(s_new_pass) - 1);
  s_new_pass[sizeof(s_new_pass) - 1] = '\0';
  s_wifi_pending = true;
  portEXIT_CRITICAL(&s_mux);
}

void web_request_print_ip() { s_print_ip = true; }

static void print_addresses() {
  Serial.printf("[WEB] Red propia '%s' (clave %s) -> http://%s\n",
                AP_SSID, AP_PASS, WiFi.softAPIP().toString().c_str());
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WEB] Wi-Fi '%s' conectado -> http://%s  o  http://%s.local\n",
                  s_ssid, WiFi.localIP().toString().c_str(), MDNS_NAME);
  } else if (s_ssid[0]) {
    Serial.printf("[WEB] Wi-Fi '%s': sin conexión todavía\n", s_ssid);
  } else {
    Serial.println("[WEB] Sin red de la casa guardada. Usa: wifi <red> <clave>");
  }
}

// Texto JSON seguro (comillas y barras escapadas)
static String json_str(const char *in) {
  String out = "\"";
  for (const char *c = in; *c; c++) {
    if (*c == '"' || *c == '\\') out += '\\';
    if ((unsigned char)*c >= 0x20) out += *c;
  }
  out += '"';
  return out;
}

// ----------------------------------------------------------------------------
// Handlers HTTP
// ----------------------------------------------------------------------------
static void handle_root() {
  server.send_P(200, "text/html; charset=utf-8", WEB_PAGE);
}

String web_data_json() {
  MeterSnapshot m = meter_get_snapshot();
  bool sta = (WiFi.status() == WL_CONNECTED);
  char buf[768];
  snprintf(buf, sizeof(buf),
           "{\"valid\":%s,\"t\":%lu,\"v\":%.1f,\"i\":%.3f,\"p\":%.1f,\"s\":%.1f,\"fp\":%.2f,"
           "\"kwh\":%.4f,\"vb\":%.0f,\"ib\":%.0f,\"v_ok\":%s,\"v_sig\":%s,\"i_ok\":%s,\"avg\":%u,"
           "\"i_lim\":%.2f,\"v_min\":%.1f,\"v_max\":%.1f,\"buz_en\":%s,\"led_en\":%s,"
           "\"buz_man\":%s,\"led_man\":%s,\"buz_on\":%s,\"led_on\":%s,"
           "\"over\":%s,\"uv\":%s,\"ov\":%s,"
           "\"v_clip\":%s,\"vpk_p\":%.0f,\"vpk_n\":%.0f,"
           "\"sta\":%s,\"ip\":\"%s\",\"ap_ip\":\"%s\",\"ssid\":",
           m.valid ? "true" : "false", (unsigned long)m.t, m.v, m.i, m.p, m.s, m.fp,
           m.kwh, m.v_bias_mv, m.i_bias_mv, m.v_ok ? "true" : "false",
           m.v_signal ? "true" : "false", m.i_ok ? "true" : "false", m.avg_n,
           m.i_limit, m.v_min, m.v_max, m.buzzer_en ? "true" : "false", m.led_en ? "true" : "false",
           m.buzzer_manual ? "true" : "false", m.led_manual ? "true" : "false",
           m.buzzer_on ? "true" : "false", m.led_on ? "true" : "false",
           m.overcurrent ? "true" : "false", m.undervolt ? "true" : "false", m.overvolt ? "true" : "false",
           m.v_clip ? "true" : "false", m.v_pk_pos_mv, m.v_pk_neg_mv,
           sta ? "true" : "false", sta ? WiFi.localIP().toString().c_str() : "",
           WiFi.softAPIP().toString().c_str());
  String json = buf;
  json += json_str(s_ssid);
  json += "}";
  return json;
}

static void handle_data() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", web_data_json());
}

String web_wave_json() {
  float wv[WAVE_LEN], wi[WAVE_LEN];   // en la pila: la usan la tarea web y la de la nube
  MeterSnapshot m = meter_get_snapshot();
  size_t n = meter_copy_wave(wv, wi, WAVE_LEN);
  String json;
  json.reserve(2400);
  char hdr[96];
  snprintf(hdr, sizeof(hdr), "{\"dt_ms\":0.25,\"fp\":%.2f,\"v\":[", m.fp);
  json += hdr;
  char num[16];
  for (size_t k = 0; k < n; k++) {
    snprintf(num, sizeof(num), "%s%.1f", k ? "," : "", wv[k]);
    json += num;
  }
  json += "],\"i\":[";
  for (size_t k = 0; k < n; k++) {
    snprintf(num, sizeof(num), "%s%.3f", k ? "," : "", wi[k]);
    json += num;
  }
  json += "]}";
  return json;
}

static void handle_wave() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", web_wave_json());
}

// Historial en partes, para no armar un texto enorme en memoria
static void handle_history() {
  size_t n = meter_copy_history(s_hist_buf, HIST_LEN);
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  server.sendContent("[");
  String chunk;
  chunk.reserve(1500);
  char item[64];
  for (size_t k = 0; k < n; k++) {
    snprintf(item, sizeof(item), "%s[%lu,%.1f,%.3f,%.1f]", k ? "," : "",
             (unsigned long)s_hist_buf[k].t, s_hist_buf[k].v, s_hist_buf[k].i, s_hist_buf[k].p);
    chunk += item;
    if (chunk.length() > 1400) {
      server.sendContent(chunk);
      chunk = "";
    }
  }
  chunk += "]";
  server.sendContent(chunk);
  server.sendContent("");  // fin de la respuesta
}

// ----------------------------------------------------------------------------
// Acciones de la página. Las mismas sirven por HTTP (red local) y por la nube (MQTT).
// ----------------------------------------------------------------------------
typedef std::function<bool(const char *key, String &out)> ArgGetter;

static bool is_true(const String &s) { return s == "1" || s == "true"; }

static bool do_action(const char *action, ArgGetter get, String &err) {
  String a;
  if (strcmp(action, "avg") == 0) {
    int n = get("n", a) ? a.toInt() : 0;
    if (n < 1 || n > 30) { err = "n debe estar entre 1 y 30"; return false; }
    meter_request_avg((uint8_t)n);
    return true;
  }
  if (strcmp(action, "energy_reset") == 0) {
    meter_request_energy_reset();
    return true;
  }
  if (strcmp(action, "limit") == 0) {
    float lim  = get("limit", a) ? a.toFloat() : 0.0f;
    float vmin = get("vmin", a) ? a.toFloat() : 0.0f;
    float vmax = get("vmax", a) ? a.toFloat() : 0.0f;
    if (lim < 0.0f || lim > 100.0f || vmin < 0.0f || vmin > 300.0f || vmax < 0.0f || vmax > 300.0f) {
      err = "Límites fuera de rango (I: 0-100 A, V: 0-300 V)";
      return false;
    }
    if (vmin > 0.0f && vmax > 0.0f && vmin >= vmax) { err = "V mín debe ser menor que V máx"; return false; }
    bool buz_en = get("buz_en", a) && is_true(a);
    bool led_en = get("led_en", a) && is_true(a);
    meter_set_limit(lim, vmin, vmax, buz_en, led_en);
    return true;
  }
  if (strcmp(action, "outputs") == 0) {
    MeterSnapshot m = meter_get_snapshot();
    bool buz = m.buzzer_manual, led = m.led_manual;
    if (get("buz", a)) buz = is_true(a);
    if (get("led", a)) led = is_true(a);
    meter_set_manual_outputs(buz, led);
    return true;
  }
  err = "Acción desconocida";
  return false;
}

// Lee "clave" de un texto tipo "a=1&b=hola+mundo" (decodifica + y %XX)
static bool query_get(const char *q, const char *key, String &out) {
  size_t klen = strlen(key);
  const char *p = q;
  while (p && *p) {
    const char *amp = strchr(p, '&');
    const char *end = amp ? amp : p + strlen(p);
    if ((size_t)(end - p) > klen && strncmp(p, key, klen) == 0 && p[klen] == '=') {
      out = "";
      for (const char *c = p + klen + 1; c < end; c++) {
        if (*c == '+') out += ' ';
        else if (*c == '%' && c + 2 < end &&isxdigit((unsigned char)c[1]) && isxdigit((unsigned char)c[2])) {
          char hex[3] = {c[1], c[2], 0};
          out += (char)strtol(hex, NULL, 16);
          c += 2;
        } else out += *c;
      }
      return true;
    }
    p = amp ? amp + 1 : NULL;
  }
  return false;
}

bool web_apply_action(const char *action, const char *query, String &err) {
  return do_action(action, [query](const char *k, String &o) { return query_get(query, k, o); }, err);
}

static void http_action(const char *action) {
  String err;
  bool ok = do_action(action, [](const char *k, String &o) {
    if (!server.hasArg(k)) return false;
    o = server.arg(k);
    return true;
  }, err);
  if (ok) server.send(200, "application/json", "{\"ok\":true}");
  else server.send(400, "application/json", "{\"error\":" + json_str(err.c_str()) + "}");
}

// --- Acceso desde internet (configuración de la nube) ---
static void handle_cloud_get() {
  CloudInfo c = cloud_get_info();
  String j = "{\"enabled\":";
  j += c.enabled ? "true" : "false";
  j += ",\"connected\":";
  j += c.connected ? "true" : "false";
  j += ",\"host\":" + json_str(c.host);
  j += ",\"port\":" + String(c.port);
  j += ",\"user\":" + json_str(c.user);
  j += ",\"id\":" + json_str(c.id);
  j += ",\"page\":" + json_str(c.page);
  j += "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", j);
}

static void handle_cloud_post() {
  if (server.hasArg("off")) {
    cloud_disable();
    server.send(200, "application/json", "{\"ok\":true}");
    return;
  }
  String host = server.arg("host"), user = server.arg("user"), pass = server.arg("pass");
  host.trim(); user.trim();
  int port = server.hasArg("port") ? server.arg("port").toInt() : 8883;
  if (host.length() == 0 || host.length() > 95 || user.length() > 63 || pass.length() > 95 || port < 1 || port > 65535) {
    server.send(400, "application/json", "{\"error\":\"Datos del servidor no válidos\"}");
    return;
  }
  cloud_configure(host.c_str(), (uint16_t)port, user.c_str(), pass.c_str());
  server.send(200, "application/json", "{\"ok\":true}");
}

static void handle_wifi() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  if (ssid.length() == 0 || ssid.length() > 32 || pass.length() > 64) {
    server.send(400, "application/json", "{\"error\":\"Nombre de red o clave no válidos\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true}");
  web_set_wifi(ssid.c_str(), pass.c_str());
}

// ----------------------------------------------------------------------------
static void web_task(void *) {
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(true);
  WiFi.softAP(AP_SSID, AP_PASS);

  wprefs.begin("wifi_cfg", false);
  String ssid = wprefs.getString("ssid", "");
  String pass = wprefs.getString("pass", "");
  strncpy(s_ssid, ssid.c_str(), sizeof(s_ssid) - 1);
  if (s_ssid[0]) WiFi.begin(s_ssid, pass.c_str());

  server.on("/", HTTP_GET, handle_root);
  server.on("/api/data", HTTP_GET, handle_data);
  server.on("/api/history", HTTP_GET, handle_history);
  server.on("/api/wave", HTTP_GET, handle_wave);
  server.on("/api/avg", HTTP_POST, []() { http_action("avg"); });
  server.on("/api/energy/reset", HTTP_POST, []() { http_action("energy_reset"); });
  server.on("/api/limit", HTTP_POST, []() { http_action("limit"); });
  server.on("/api/outputs", HTTP_POST, []() { http_action("outputs"); });
  server.on("/api/wifi", HTTP_POST, handle_wifi);
  server.on("/api/cloud", HTTP_GET, handle_cloud_get);
  server.on("/api/cloud", HTTP_POST, handle_cloud_post);
  server.onNotFound([]() { server.send(404, "text/plain", "No encontrado"); });
  server.begin();

  if (MDNS.begin(MDNS_NAME)) MDNS.addService("http", "tcp", 80);

  print_addresses();

  bool was_connected = false;
  uint32_t last_check = millis();

  while (true) {
    server.handleClient();

    if (s_wifi_pending) {
      char new_ssid[33], new_pass[65];
      portENTER_CRITICAL(&s_mux);
      memcpy(new_ssid, s_new_ssid, sizeof(new_ssid));
      memcpy(new_pass, s_new_pass, sizeof(new_pass));
      s_wifi_pending = false;
      portEXIT_CRITICAL(&s_mux);

      wprefs.putString("ssid", new_ssid);
      wprefs.putString("pass", new_pass);
      strncpy(s_ssid, new_ssid, sizeof(s_ssid) - 1);
      s_ssid[sizeof(s_ssid) - 1] = '\0';
      delay(300);  // dejar que salga la respuesta HTTP antes de cambiar de red
      WiFi.disconnect();
      WiFi.begin(new_ssid, new_pass);
      Serial.printf("[WEB] Conectando a '%s'...\n", new_ssid);
    }

    if (s_print_ip) {
      s_print_ip = false;
      print_addresses();
    }

    if (millis() - last_check >= 1000) {
      last_check = millis();
      bool now_connected = (WiFi.status() == WL_CONNECTED);
      if (now_connected && !was_connected) {
        Serial.printf("[WEB] Wi-Fi '%s' conectado -> http://%s  o  http://%s.local\n",
                      s_ssid, WiFi.localIP().toString().c_str(), MDNS_NAME);
      } else if (!now_connected && was_connected) {
        Serial.println("[WEB] Se perdió el Wi-Fi; reintentando...");
      }
      was_connected = now_connected;
    }

    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

void web_init() {
  xTaskCreatePinnedToCore(web_task, "web_task", 8192, NULL, 1, NULL, 0);  // Core 0
}
