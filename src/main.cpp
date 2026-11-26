/**
 * ============================================================================
 * SMART VOLT - MEDIDOR MONOFÁSICO: VOLTAJE (ZMPT101B) + CORRIENTE (SCT-013-000)
 * ============================================================================
 * Hardware:
 *   - Voltaje  : ZMPT101B a 5 V -> divisor 3.3k / 4.7k -> GPIO1 (centro ~1470 mV)
 *   - Corriente: SCT-013-000 + burden -> bias 10k/10k -> GPIO2 (centro ~1650 mV)
 *       burden 33.3 Ω (3 x 100 Ω) -> KI teórico 60.06 A/V, máx ~59 A
 *       burden 50 Ω   (2 x 100 Ω) -> KI teórico 40.00 A/V, máx ~37 A (mejor para 0,1-25 A)
 *   - Red: 120 VAC / 60 Hz
 *
 * Cada segundo toma 4000 muestras (4 kHz = 60 ciclos). La corriente se lee dos veces
 * por muestra (I, V, I) y se promedia: baja el ruido y queda alineada en el tiempo con V.
 * Después se aplica una media móvil de N segundos (comando 'media N'); si la corriente
 * cambia de verdad (encender/apagar una carga), la media se reinicia para responder rápido.
 *
 * Comandos por el monitor serie (terminar con Enter):
 *   cal v <V>   calibra KV con la tensión real del multímetro     (ej: cal v 121.5)
 *   cal i <A>   calibra KI con la corriente real de una pinza     (ej: cal i 4.82)
 *   ruido       SIN carga: mide el ruido de corriente y lo descuenta
 *   invertir    cambia el signo de la corriente (si P sale negativa)
 *   kv <n> / ki <n>   fija las constantes a mano (ej: ki 40 con burden de 50 Ω)
 *   media <n>   segundos de la media móvil (1 a 30)
 *   wifi <red> <clave>   guarda la red Wi-Fi de la casa
 *   ip          muestra la dirección de la página web
 *   eth / eth test   estado del Ethernet ENC28J60 / prueba de internet (si está activado)
 *   zero e      pone la energía en 0
 *   raw         muestra / oculta los datos de los pines
 *   show        constantes guardadas       pausa   detiene / reanuda la consola
 * ============================================================================
 */
#include <Arduino.h>
#include <Preferences.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "board_pins.h"
#include "meter.h"
#include "web.h"
#include "eth.h"

// --- Muestreo ---
static const uint32_t SAMPLE_RATE_HZ = 4000;
static const uint32_t SAMPLE_PERIOD_US = 1000000 / SAMPLE_RATE_HZ; // 250 us
static const uint32_t N_SAMPLES = SAMPLE_RATE_HZ;                  // 1 s = 60 ciclos

// --- Diagnóstico (mV en el pin) ---
static const float V_BIAS_MIN_MV = 1150.0f, V_BIAS_MAX_MV = 1800.0f;  // esperado ~1470
static const float I_BIAS_MIN_MV = 1500.0f, I_BIAS_MAX_MV = 1800.0f;  // esperado ~1650
static const float SAT_LOW_MV = 100.0f, SAT_HIGH_MV = 3000.0f;
static const float MIN_V_SIGNAL_MV = 5.0f;
static const float V_CLIP_ASYM = 0.12f;  // >12 % de diferencia entre pico + y pico − = recorte

// --- Valores por defecto ---
static const float DEFAULT_KV = 200.0f;
static const float DEFAULT_KI = 60.06f;     // burden 33.3 Ω. Con 50 Ω usar 'ki 40'
static const float I_THRESHOLD_A = 0.05f;   // por debajo se muestra 0 A

// Detección de corriente real: componente de 60 Hz de la corriente (DFT de un solo
// punto). En 1 s a 4 kHz hay exactamente 60 ciclos, así que el ruido del ADC (que se
// reparte en todas las frecuencias) casi no aparece aquí. Si la componente de 60 Hz
// es menor que esto, no hay nada en la pinza: corriente, potencia y onda van a 0.
static const float I_FUND_MIN_A = 0.04f;
static const uint16_t DFT_LEN = 200;        // 60/4000 = 3/200 -> la tabla se repite cada 200 muestras
static float s_cos60[DFT_LEN], s_sin60[DFT_LEN];
static float g_last_i1_a = 0.0f;            // componente de 60 Hz del último segundo (A)
static const uint8_t DEFAULT_AVG_N = 5;     // media móvil de 5 s
static const uint8_t AVG_MAX = 30;

// Cambio de corriente que se considera "real" y reinicia la media móvil
static const float JUMP_MIN_A = 0.20f;      // al menos 0,2 A ...
static const float JUMP_REL = 0.15f;        // ... y más del 15 % del valor promedio

static const uint32_t ENERGY_SAVE_MS = 10UL * 60UL * 1000UL;  // guardar kWh cada 10 min
static const uint16_t WAVE_BUF_LEN = 240;                     // ~60 ms a 4 kHz para buscar cruce por cero
static const uint16_t BUZZER_FREQ_HZ = 2000;                  // frecuencia para buzzer pasivo

static Preferences prefs;
static float g_kv = DEFAULT_KV;
static float g_ki = DEFAULT_KI;
static float g_noise_mv = 0.0f;
static int8_t g_sign = 1;
static uint8_t g_avg_n = DEFAULT_AVG_N;
static double g_kwh = 0.0;
static float g_i_limit = 0.0f;
static float g_v_min = 0.0f;
static float g_v_max = 0.0f;
static bool g_buzzer_en = true;
static bool g_led_en = true;
static bool g_buzzer_manual = false;
static bool g_led_manual = false;
static bool g_overcurrent = false;
static bool g_undervolt = false;
static bool g_overvolt = false;
static bool g_buzzer_active = false;
static bool g_led_active = false;
static bool g_paused = false;
static bool g_raw = false;

// Última lectura de 1 s sin promediar (para los comandos de calibración)
static float g_last_vrms_pin_mv = 0.0f;
static float g_last_irms_pin_raw_mv = 0.0f;
static float g_last_irms_pin_clean_mv = 0.0f;

// Media móvil
static float avg_v[AVG_MAX], avg_i[AVG_MAX], avg_p[AVG_MAX];
static uint8_t avg_pos = 0, avg_count = 0;

// Datos compartidos con el servidor web
static SemaphoreHandle_t s_lock = NULL;
static MeterSnapshot s_snap = {};
static HistPoint s_hist[HIST_LEN];
static size_t s_hist_head = 0, s_hist_count = 0;
static float s_wave_v[WAVE_LEN];
static float s_wave_i[WAVE_LEN];
static size_t s_wave_count = 0;
static volatile int s_req_avg = -1;
static volatile bool s_req_energy_reset = false;

static char cmd_buf[96];
static uint8_t cmd_len = 0;

// ----------------------------------------------------------------------------
// API para la tarea web
// ----------------------------------------------------------------------------
MeterSnapshot meter_get_snapshot() {
  MeterSnapshot out;
  xSemaphoreTake(s_lock, portMAX_DELAY);
  out = s_snap;
  xSemaphoreGive(s_lock);
  return out;
}

size_t meter_copy_history(HistPoint *dst, size_t max_points) {
  xSemaphoreTake(s_lock, portMAX_DELAY);
  size_t n = (s_hist_count < max_points) ? s_hist_count : max_points;
  size_t start = (s_hist_head + HIST_LEN - n) % HIST_LEN;
  for (size_t k = 0; k < n; k++) dst[k] = s_hist[(start + k) % HIST_LEN];
  xSemaphoreGive(s_lock);
  return n;
}

static void apply_outputs_locked() {
  bool alarm = g_overcurrent || g_undervolt || g_overvolt;
  bool led = g_led_manual || (alarm && g_led_en);
  bool buz = g_buzzer_manual || (alarm && g_buzzer_en);
  if (led != g_led_active) {
    g_led_active = led;
    digitalWrite(PIN_LED_ALARM, led ? HIGH : LOW);
  }
  if (buz != g_buzzer_active) {
    g_buzzer_active = buz;
    if (buz) tone(PIN_BUZZER, BUZZER_FREQ_HZ);
    else noTone(PIN_BUZZER);
  }
  s_snap.i_limit = g_i_limit;
  s_snap.v_min = g_v_min;
  s_snap.v_max = g_v_max;
  s_snap.buzzer_en = g_buzzer_en;
  s_snap.led_en = g_led_en;
  s_snap.buzzer_manual = g_buzzer_manual;
  s_snap.led_manual = g_led_manual;
  s_snap.buzzer_on = g_buzzer_active;
  s_snap.led_on = g_led_active;
  s_snap.overcurrent = g_overcurrent;
  s_snap.undervolt = g_undervolt;
  s_snap.overvolt = g_overvolt;
}

size_t meter_copy_wave(float *v_dst, float *i_dst, size_t max_points) {
  xSemaphoreTake(s_lock, portMAX_DELAY);
  size_t n = (s_wave_count < max_points) ? s_wave_count : max_points;
  for (size_t k = 0; k < n; k++) {
    v_dst[k] = s_wave_v[k];
    i_dst[k] = s_wave_i[k];
  }
  xSemaphoreGive(s_lock);
  return n;
}

void meter_request_avg(uint8_t n) { s_req_avg = n; }
void meter_request_energy_reset() { s_req_energy_reset = true; }

void meter_set_limit(float limit_a, float v_min, float v_max, bool buzzer_en, bool led_en) {
  if (limit_a < 0.0f) limit_a = 0.0f;
  if (limit_a > 100.0f) limit_a = 100.0f;
  if (v_min < 0.0f) v_min = 0.0f;
  if (v_min > 300.0f) v_min = 300.0f;
  if (v_max < 0.0f) v_max = 0.0f;
  if (v_max > 300.0f) v_max = 300.0f;
  xSemaphoreTake(s_lock, portMAX_DELAY);
  g_i_limit = limit_a;
  g_v_min = v_min;
  g_v_max = v_max;
  g_buzzer_en = buzzer_en;
  g_led_en = led_en;
  g_overcurrent = (g_i_limit > 0.0f) && (s_snap.i >= g_i_limit);
  g_undervolt = s_snap.v_ok && (g_v_min > 0.0f) && (s_snap.v < g_v_min);
  g_overvolt = s_snap.v_ok && (g_v_max > 0.0f) && (s_snap.v > g_v_max);
  apply_outputs_locked();
  xSemaphoreGive(s_lock);
  prefs.putFloat("i_lim", g_i_limit);
  prefs.putFloat("v_min", g_v_min);
  prefs.putFloat("v_max", g_v_max);
  prefs.putBool("buz_en", g_buzzer_en);
  prefs.putBool("led_en", g_led_en);
  Serial.printf(">> Límites: I=%.2f A | V=%.1f..%.1f V | Buzzer auto: %s | LED auto: %s\n",
                g_i_limit, g_v_min, g_v_max, g_buzzer_en ? "SI" : "NO", g_led_en ? "SI" : "NO");
}

void meter_set_manual_outputs(bool buzzer_on, bool led_on) {
  xSemaphoreTake(s_lock, portMAX_DELAY);
  g_buzzer_manual = buzzer_on;
  g_led_manual = led_on;
  apply_outputs_locked();
  xSemaphoreGive(s_lock);
}

// ----------------------------------------------------------------------------
static void set_avg(int n) {
  if (n < 1) n = 1;
  if (n > AVG_MAX) n = AVG_MAX;
  g_avg_n = (uint8_t)n;
  prefs.putUChar("avg", g_avg_n);
  Serial.printf(">> Media móvil: %u s. Guardado.\n", g_avg_n);
}

static void reset_energy() {
  g_kwh = 0.0;
  prefs.putDouble("kwh", 0.0);
  Serial.println(">> Energía en 0 kWh.");
}

static void show_constants() {
  Serial.printf(">> KV=%.2f | KI=%.2f A/V | ruido I=%.2f mV | signo=%+d | media=%u s | E=%.3f kWh\n",
                g_kv, g_ki, g_noise_mv, g_sign, g_avg_n, g_kwh);
}

static void handle_command(char *cmd) {
  while (*cmd == ' ') cmd++;

  if (strncmp(cmd, "cal v", 5) == 0) {
    float v_real = atof(cmd + 5);
    if (v_real <= 0.0f) {
      Serial.println(">> Uso: cal v <Vreal>   (ej: cal v 121.5)");
    } else if (g_last_vrms_pin_mv < MIN_V_SIGNAL_MV) {
      Serial.println(">> Error: no hay señal AC en GPIO1, no se puede calibrar.");
    } else {
      g_kv = v_real / (g_last_vrms_pin_mv / 1000.0f);
      prefs.putFloat("kv", g_kv);
      avg_count = 0;
      Serial.printf(">> KV = %.2f (%.1f V / %.2f mV en el pin). Guardado.\n", g_kv, v_real, g_last_vrms_pin_mv);
    }
  } else if (strncmp(cmd, "cal i", 5) == 0) {
    float i_real = atof(cmd + 5);
    if (i_real <= 0.0f) {
      Serial.println(">> Uso: cal i <Areal>   (ej: cal i 4.82) con una carga encendida");
    } else if (g_last_irms_pin_clean_mv < 1.0f) {
      Serial.println(">> Error: casi no hay señal de corriente. Enciende una carga de varios amperios.");
    } else {
      g_ki = i_real / (g_last_irms_pin_clean_mv / 1000.0f);
      prefs.putFloat("ki", g_ki);
      avg_count = 0;
      Serial.printf(">> KI = %.2f A/V (%.2f A / %.2f mV en el pin). Guardado.\n", g_ki, i_real, g_last_irms_pin_clean_mv);
    }
  } else if (strcmp(cmd, "ruido") == 0) {
    g_noise_mv = g_last_irms_pin_raw_mv;
    prefs.putFloat("noise", g_noise_mv);
    avg_count = 0;
    Serial.printf(">> Ruido sin carga = %.2f mV (= %.3f A). Se descontará. Guardado.\n",
                  g_noise_mv, g_noise_mv / 1000.0f * g_ki);
    Serial.println(">> (Debe hacerse con el SCT puesto y SIN ninguna carga encendida)");
  } else if (strcmp(cmd, "invertir") == 0) {
    g_sign = -g_sign;
    prefs.putChar("sign", g_sign);
    avg_count = 0;
    Serial.printf(">> Signo de la corriente: %+d. Guardado.\n", g_sign);
  } else if (strncmp(cmd, "kv", 2) == 0 && atof(cmd + 2) > 0.0f) {
    g_kv = atof(cmd + 2);
    prefs.putFloat("kv", g_kv);
    avg_count = 0;
    show_constants();
  } else if (strncmp(cmd, "ki", 2) == 0 && atof(cmd + 2) > 0.0f) {
    g_ki = atof(cmd + 2);
    prefs.putFloat("ki", g_ki);
    avg_count = 0;
    show_constants();
  } else if (strncmp(cmd, "media", 5) == 0) {
    int n = atoi(cmd + 5);
    if (n >= 1 && n <= AVG_MAX) set_avg(n);
    else Serial.printf(">> Uso: media <1..%u>   (actual: %u s)\n", AVG_MAX, g_avg_n);
  } else if (strncmp(cmd, "wifi ", 5) == 0) {
    // La clave es la última palabra; la red puede tener espacios
    char *args = cmd + 5;
    char *sp = strrchr(args, ' ');
    if (sp == NULL || sp == args) {
      Serial.println(">> Uso: wifi <red> <clave>   (ej: wifi MiCasa clave1234)");
    } else {
      *sp = '\0';
      web_set_wifi(args, sp + 1);
      Serial.printf(">> Red '%s' guardada. Conectando...\n", args);
    }
  } else if (strcmp(cmd, "ip") == 0) {
    web_request_print_ip();
  } else if (strcmp(cmd, "eth") == 0) {
    eth_request_status();
  } else if (strcmp(cmd, "eth test") == 0) {
    eth_request_internet_test();
  } else if (strcmp(cmd, "zero e") == 0) {
    reset_energy();
  } else if (strcmp(cmd, "raw") == 0) {
    g_raw = !g_raw;
    Serial.printf(">> Datos de los pines: %s\n", g_raw ? "visibles" : "ocultos");
  } else if (strcmp(cmd, "show") == 0) {
    show_constants();
  } else if (strcmp(cmd, "pausa") == 0) {
    g_paused = !g_paused;
    Serial.println(g_paused ? ">> Consola en pausa (la medición y la web siguen)" : ">> Consola reanudada");
  } else {
    Serial.println(">> Comandos: cal v <V> | cal i <A> | ruido | invertir | kv <n> | ki <n> | media <n>");
    Serial.println(">>           wifi <red> <clave> | ip | eth | eth test | zero e | raw | show | pausa");
  }
}

static void poll_serial() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      if (cmd_len > 0) {
        cmd_buf[cmd_len] = '\0';
        handle_command(cmd_buf);
        cmd_len = 0;
      }
    } else if (cmd_len < sizeof(cmd_buf) - 1) {
      cmd_buf[cmd_len++] = c;
    }
  }
}

// Promedio de los últimos g_avg_n segundos guardados
static float avg_of(const float *buf) {
  uint8_t n = (avg_count < g_avg_n) ? avg_count : g_avg_n;
  if (n == 0) return 0.0f;
  float sum = 0.0f;
  for (uint8_t k = 0; k < n; k++) sum += buf[(avg_pos + AVG_MAX - 1 - k) % AVG_MAX];
  return sum / n;
}

// ----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1500);

  s_lock = xSemaphoreCreateMutex();

  for (uint16_t k = 0; k < DFT_LEN; k++) {
    float ph = 2.0f * (float)M_PI * 60.0f * (float)k / (float)SAMPLE_RATE_HZ;
    s_cos60[k] = cosf(ph);
    s_sin60[k] = sinf(ph);
  }

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_ADC_VOLT, ADC_11db);
  analogSetPinAttenuation(PIN_ADC_CURRENT, ADC_11db);
  pinMode(PIN_ADC_VOLT, INPUT);
  pinMode(PIN_ADC_CURRENT, INPUT);
  pinMode(PIN_LED_ALARM, OUTPUT);
  digitalWrite(PIN_LED_ALARM, LOW);
  pinMode(PIN_BUZZER, OUTPUT);
  noTone(PIN_BUZZER);

  prefs.begin("volt_test", false);  // mismo espacio: se conservan KV/KI ya calibrados
  g_kv = prefs.getFloat("kv", DEFAULT_KV);
  g_ki = prefs.getFloat("ki", DEFAULT_KI);
  g_noise_mv = prefs.getFloat("noise", 0.0f);
  g_sign = prefs.getChar("sign", 1);
  if (g_sign != 1 && g_sign != -1) g_sign = 1;
  g_avg_n = prefs.getUChar("avg", DEFAULT_AVG_N);
  if (g_avg_n < 1 || g_avg_n > AVG_MAX) g_avg_n = DEFAULT_AVG_N;
  g_kwh = prefs.getDouble("kwh", 0.0);
  g_i_limit = prefs.getFloat("i_lim", 0.0f);
  if (g_i_limit < 0.0f || g_i_limit > 100.0f) g_i_limit = 0.0f;
  g_v_min = prefs.getFloat("v_min", 0.0f);
  if (g_v_min < 0.0f || g_v_min > 300.0f) g_v_min = 0.0f;
  g_v_max = prefs.getFloat("v_max", 0.0f);
  if (g_v_max < 0.0f || g_v_max > 300.0f) g_v_max = 0.0f;
  g_buzzer_en = prefs.getBool("buz_en", true);
  g_led_en = prefs.getBool("led_en", true);
  xSemaphoreTake(s_lock, portMAX_DELAY);
  apply_outputs_locked();
  xSemaphoreGive(s_lock);

  Serial.println();
  Serial.println("==================================================");
  Serial.println(" SMART VOLT - MEDIDOR (voltaje + corriente + web)");
  Serial.printf (" V: GPIO%d (~1470 mV) | I: GPIO%d (~1650 mV)\n", PIN_ADC_VOLT, PIN_ADC_CURRENT);
  Serial.println(" Escribe 'ayuda' para ver los comandos");
  Serial.println("==================================================");
  show_constants();

  web_init();
  eth_init();   // no hace nada mientras ENABLE_ETHERNET = 0 (src/eth.h)
}

void loop() {
  static uint32_t last_ms = millis();
  static uint32_t last_save_ms = millis();

  poll_serial();

  // Pedidos de la página web
  if (s_req_avg > 0) { set_avg(s_req_avg); s_req_avg = -1; }
  if (s_req_energy_reset) { s_req_energy_reset = false; reset_energy(); }

  // --- Captura de 1 s: por muestra se lee I, V, I (la media de las dos I queda alineada con V) ---
  double sv = 0, svv = 0, si = 0, sii = 0, svi = 0;
  double i_re = 0, i_im = 0;   // DFT de 60 Hz de la corriente
  uint16_t dft_k = 0;
  uint32_t vmin = 0xFFFF, vmax = 0, imin = 0xFFFF, imax = 0;
  uint32_t late = 0;
  static uint16_t wave_v_raw[WAVE_BUF_LEN];
  static float wave_i_raw[WAVE_BUF_LEN];

  uint32_t t_next = micros();
  for (uint32_t n = 0; n < N_SAMPLES; n++) {
    while ((int32_t)(micros() - t_next) < 0) { }
    if ((int32_t)(micros() - t_next) > (int32_t)SAMPLE_PERIOD_US) late++;
    t_next += SAMPLE_PERIOD_US;

    uint32_t i1 = analogReadMilliVolts(PIN_ADC_CURRENT);
    uint32_t v = analogReadMilliVolts(PIN_ADC_VOLT);
    uint32_t i2 = analogReadMilliVolts(PIN_ADC_CURRENT);
    double i = (i1 + i2) * 0.5;

    if (n >= N_SAMPLES - WAVE_BUF_LEN) {
      uint16_t wi = (uint16_t)(n - (N_SAMPLES - WAVE_BUF_LEN));
      wave_v_raw[wi] = (uint16_t)v;
      wave_i_raw[wi] = (float)i;
    }

    sv += v;  svv += (double)v * v;
    si += i;  sii += i * i;
    svi += v * i;
    i_re += i * s_cos60[dft_k];
    i_im += i * s_sin60[dft_k];
    if (++dft_k >= DFT_LEN) dft_k = 0;
    if (v < vmin) vmin = v;
    if (v > vmax) vmax = v;
    if (i1 < imin) imin = i1;
    if (i1 > imax) imax = i1;
  }

  // --- Cálculos de este segundo ---
  const double N = N_SAMPLES;
  double v_mean = sv / N, i_mean = si / N;
  double v_var = svv / N - v_mean * v_mean;
  double i_var = sii / N - i_mean * i_mean;
  double vi_cov = svi / N - v_mean * i_mean;

  float vrms_pin = (v_var > 0) ? (float)sqrt(v_var) : 0.0f;
  float irms_pin_raw = (i_var > 0) ? (float)sqrt(i_var) : 0.0f;
  float i_clean_sq = irms_pin_raw * irms_pin_raw - g_noise_mv * g_noise_mv;
  float irms_pin = (i_clean_sq > 0) ? sqrtf(i_clean_sq) : 0.0f;

  g_last_vrms_pin_mv = vrms_pin;
  g_last_irms_pin_raw_mv = irms_pin_raw;
  g_last_irms_pin_clean_mv = irms_pin;

  bool v_ok = (v_mean >= V_BIAS_MIN_MV && v_mean <= V_BIAS_MAX_MV);
  bool v_signal = v_ok && vrms_pin >= MIN_V_SIGNAL_MV;
  bool i_ok = (i_mean >= I_BIAS_MIN_MV && i_mean <= I_BIAS_MAX_MV);

  // Recorte de la onda de voltaje: una senoidal sana tiene el pico + igual al pico −.
  // El LM358 del ZMPT101B (a 5 V) no sube de ~3.5 V, así que recorta primero arriba
  // (~2060 mV en el pin con el divisor 3.3k/4.7k), lejos del límite del ADC.
  float v_pk_pos = (float)vmax - (float)v_mean;
  float v_pk_neg = (float)v_mean - (float)vmin;
  bool v_clip = v_signal && (vmin <= SAT_LOW_MV || vmax >= SAT_HIGH_MV ||
                (v_pk_neg > 50.0f && fabsf(v_pk_pos - v_pk_neg) > V_CLIP_ASYM * v_pk_neg));

  float v_sec = v_ok ? vrms_pin / 1000.0f * g_kv : 0.0f;
  // Componente de 60 Hz (RMS) de la corriente: sqrt(2)*|X|/N. El nivel DC se cancela
  // porque la ventana tiene ciclos completos.
  float i1_pin_mv = (float)(sqrt(i_re * i_re + i_im * i_im) * M_SQRT2 / N);
  float i1_a = i1_pin_mv / 1000.0f * g_ki;
  g_last_i1_a = i1_a;
  bool i_present = i_ok && (i1_a >= I_FUND_MIN_A);

  float i_sec = i_present ? irms_pin / 1000.0f * g_ki : 0.0f;
  float p_sec = (v_ok && i_present) ? (float)(vi_cov / 1e6) * g_kv * g_ki * g_sign : 0.0f;

  // --- Energía: con la potencia de cada segundo y el tiempo real transcurrido ---
  uint32_t now_ms = millis();
  float dt_s = (now_ms - last_ms) / 1000.0f;
  last_ms = now_ms;
  if (i_sec >= I_THRESHOLD_A && p_sec > 0.0f && dt_s > 0.0f && dt_s < 5.0f) {
    g_kwh += (double)p_sec * dt_s / 3600000.0;
  }
  if (now_ms - last_save_ms >= ENERGY_SAVE_MS) {
    prefs.putDouble("kwh", g_kwh);
    last_save_ms = now_ms;
  }

  // --- Media móvil. Un cambio grande de corriente (carga encendida/apagada) la reinicia ---
  if (avg_count > 0) {
    float i_avg_prev = avg_of(avg_i);
    float jump = fabsf(i_sec - i_avg_prev);
    if (jump > JUMP_MIN_A && jump > JUMP_REL * i_avg_prev) avg_count = 0;
  }
  avg_v[avg_pos] = v_sec;
  avg_i[avg_pos] = i_sec;
  avg_p[avg_pos] = p_sec;
  avg_pos = (avg_pos + 1) % AVG_MAX;
  if (avg_count < AVG_MAX) avg_count++;

  float v_rms = avg_of(avg_v);
  float i_rms = avg_of(avg_i);
  float p = avg_of(avg_p);
  if (i_rms < I_THRESHOLD_A) { i_rms = 0.0f; p = 0.0f; }
  float s = v_rms * i_rms;
  float fp = (s > 0.5f) ? p / s : 0.0f;
  if (fp > 1.0f) fp = 1.0f;
  if (fp < -1.0f) fp = -1.0f;

  // --- Formas de onda instantáneas (sincronizadas al cruce por cero ascendente de V) ---
  static float wave_v_inst[WAVE_BUF_LEN];
  static float wave_i_inst[WAVE_BUF_LEN];
  for (uint16_t k = 0; k < WAVE_BUF_LEN; k++) {
    wave_v_inst[k] = v_ok ? ((float)wave_v_raw[k] - (float)v_mean) / 1000.0f * g_kv : 0.0f;
    wave_i_inst[k] = i_present ? (wave_i_raw[k] - (float)i_mean) / 1000.0f * g_ki * g_sign : 0.0f;
  }
  // Suavizado ligero de la onda de corriente para la gráfica (filtro 1-2-1, sin desfase)
  if (i_present) {
    float prev = wave_i_inst[0];
    for (uint16_t k = 1; k + 1 < WAVE_BUF_LEN; k++) {
      float cur = wave_i_inst[k];
      wave_i_inst[k] = 0.25f * prev + 0.5f * cur + 0.25f * wave_i_inst[k + 1];
      prev = cur;
    }
  }
  uint16_t wave_start = 0;
  if (v_signal) {
    uint16_t max_start = WAVE_BUF_LEN - WAVE_LEN;
    for (uint16_t k = 1; k < max_start; k++) {
      if (wave_v_inst[k - 1] < 0.0f && wave_v_inst[k] >= 0.0f) {
        wave_start = k;
        break;
      }
    }
  }

  // --- Publicar para la página web ---
  uint32_t t_s = now_ms / 1000;
  xSemaphoreTake(s_lock, portMAX_DELAY);
  s_snap.t = t_s;
  s_snap.v = v_rms;
  s_snap.i = i_rms;
  s_snap.p = p;
  s_snap.s = s;
  s_snap.fp = fp;
  s_snap.kwh = g_kwh;
  s_snap.v_bias_mv = (float)v_mean;
  s_snap.i_bias_mv = (float)i_mean;
  s_snap.v_ok = v_ok;
  s_snap.v_signal = v_signal;
  s_snap.v_clip = v_clip;
  s_snap.v_pk_pos_mv = v_pk_pos;
  s_snap.v_pk_neg_mv = v_pk_neg;
  s_snap.i_ok = i_ok;
  s_snap.avg_n = g_avg_n;
  s_snap.valid = true;
  g_overcurrent = (g_i_limit > 0.0f) && (i_rms >= g_i_limit);
  g_undervolt = v_ok && (g_v_min > 0.0f) && (v_rms < g_v_min);
  g_overvolt = v_ok && (g_v_max > 0.0f) && (v_rms > g_v_max);
  apply_outputs_locked();
  for (size_t k = 0; k < WAVE_LEN; k++) {
    s_wave_v[k] = wave_v_inst[wave_start + k];
    s_wave_i[k] = wave_i_inst[wave_start + k];
  }
  s_wave_count = WAVE_LEN;
  s_hist[s_hist_head] = {t_s, v_rms, i_rms, p};
  s_hist_head = (s_hist_head + 1) % HIST_LEN;
  if (s_hist_count < HIST_LEN) s_hist_count++;
  xSemaphoreGive(s_lock);

  // --- Consola ---
  if (!g_paused) {
    if (g_raw) {
      Serial.printf(" V pin: media=%4.0f min=%4u max=%4u pk-pk=%4u mV | Vrms %7.2f mV\n",
                    v_mean, vmin, vmax, vmax - vmin, vrms_pin);
      Serial.printf(" I pin: media=%4.0f min=%4u max=%4u pk-pk=%4u mV | Irms %7.2f mV (limpio %.2f) | 60 Hz %.2f mV = %.3f A%s\n",
                    i_mean, imin, imax, imax - imin, irms_pin_raw, irms_pin, i1_pin_mv, i1_a,
                    i_present ? "" : " -> sin carga");
    }
    Serial.printf(" %6.1f V | %6.3f A | %7.1f W | %7.1f VA | FP %5.2f | %.3f kWh\n",
                  v_rms, i_rms, p, s, fp, g_kwh);

    if (!v_ok)
      Serial.printf(" ! Voltaje: centro %.0f mV (esperado ~1470): ZMPT sin 5 V, divisor o cable.\n", v_mean);
    else if (!v_signal)
      Serial.println(" ! Voltaje: sin señal AC (¿ZMPT sin conectar a la red?).");
    else if (v_clip)
      Serial.printf(" ! Voltaje recortado: pico + %.0f mV / pico - %.0f mV (max %u mV). "
                    "Baja el potenciómetro del ZMPT hasta que se igualen y luego 'cal v'.\n",
                    v_pk_pos, v_pk_neg, vmax);

    if (!i_ok)
      Serial.printf(" ! Corriente: centro %.0f mV (esperado ~1650): revisa el divisor 10k/10k, el 10 uF o el cable.\n", i_mean);
    else if (imin <= SAT_LOW_MV || imax >= SAT_HIGH_MV)
      Serial.println(" ! Corriente recortada: la carga supera el rango del SCT/burden.");
    else if (p < -1.0f)
      Serial.println(" ! Potencia negativa: la pinza está al revés. Gírala o escribe 'invertir'.");

    if (late > N_SAMPLES / 100)
      Serial.printf(" ! Aviso: %u muestras fuera de tiempo (no se alcanzan 4 kHz)\n", late);
  }

  poll_serial();
  vTaskDelay(1);  // deja correr a las tareas de menor prioridad del Core 1
}
