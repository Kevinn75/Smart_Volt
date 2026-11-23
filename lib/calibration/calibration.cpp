#include "calibration.h"
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static Preferences s_prefs;
static CalibrationConfig s_config;
static SystemConfig s_sys_config;
static SemaphoreHandle_t s_calib_mutex = NULL;

#define PREFS_NAMESPACE "smart_volt"
#define PREFS_MAGIC_KEY "magic"
#define PREFS_MAGIC_VAL 0x564F4C55 // 'VOLU' actualizado

void calibration_init() {
    if (s_calib_mutex == NULL) {
        s_calib_mutex = xSemaphoreCreateMutex();
    }

    xSemaphoreTake(s_calib_mutex, portMAX_DELAY);

    s_prefs.begin(PREFS_NAMESPACE, false);
    uint32_t magic = s_prefs.getUInt(PREFS_MAGIC_KEY, 0);

    if (magic == PREFS_MAGIC_VAL) {
        s_config.kv = s_prefs.getFloat("kv", DEFAULT_KV);
        s_config.ki[0] = s_prefs.getFloat("ki0", DEFAULT_KI);
        s_config.ki[1] = s_prefs.getFloat("ki1", DEFAULT_KI);
        s_config.ki[2] = s_prefs.getFloat("ki2", DEFAULT_KI);

        s_config.noise_thresh[0] = s_prefs.getFloat("nth0", DEFAULT_NOISE_THRESH_A);
        s_config.noise_thresh[1] = s_prefs.getFloat("nth1", DEFAULT_NOISE_THRESH_A);
        s_config.noise_thresh[2] = s_prefs.getFloat("nth2", DEFAULT_NOISE_THRESH_A);

        s_config.phase_alpha[0] = s_prefs.getFloat("pha0", DEFAULT_PHASE_I1);
        s_config.phase_alpha[1] = s_prefs.getFloat("pha1", DEFAULT_PHASE_I2);
        s_config.phase_alpha[2] = s_prefs.getFloat("pha2", DEFAULT_PHASE_I3);

        s_config.current_sign[0] = (int8_t)s_prefs.getChar("sgn0", 1);
        s_config.current_sign[1] = (int8_t)s_prefs.getChar("sgn1", 1);
        s_config.current_sign[2] = (int8_t)s_prefs.getChar("sgn2", 1);

        s_config.energy_kwh[0] = s_prefs.getDouble("e0", 0.0);
        s_config.energy_kwh[1] = s_prefs.getDouble("e1", 0.0);
        s_config.energy_kwh[2] = s_prefs.getDouble("e2", 0.0);

        // Cargar configuración de usuario y red
        String user = s_prefs.getString("usr", "Mi Hogar");
        strncpy(s_sys_config.user_name, user.c_str(), sizeof(s_sys_config.user_name) - 1);
        s_sys_config.rooms_count = s_prefs.getUChar("rooms", 3);
        if (s_sys_config.rooms_count < 1 || s_sys_config.rooms_count > 3) s_sys_config.rooms_count = 3;

        String r0 = s_prefs.getString("rn0", "Tablero Principal");
        String r1 = s_prefs.getString("rn1", "Habitacion 1");
        String r2 = s_prefs.getString("rn2", "Habitacion 2 / Cocina");
        strncpy(s_sys_config.room_names[0], r0.c_str(), sizeof(s_sys_config.room_names[0]) - 1);
        strncpy(s_sys_config.room_names[1], r1.c_str(), sizeof(s_sys_config.room_names[1]) - 1);
        strncpy(s_sys_config.room_names[2], r2.c_str(), sizeof(s_sys_config.room_names[2]) - 1);

        String ssid = s_prefs.getString("ssid", "");
        String pass = s_prefs.getString("pass", "");
        strncpy(s_sys_config.wifi_ssid, ssid.c_str(), sizeof(s_sys_config.wifi_ssid) - 1);
        strncpy(s_sys_config.wifi_pass, pass.c_str(), sizeof(s_sys_config.wifi_pass) - 1);
    } else {
        s_config.kv = DEFAULT_KV;
        for (int i = 0; i < 3; i++) {
            s_config.ki[i] = DEFAULT_KI;
            s_config.noise_thresh[i] = DEFAULT_NOISE_THRESH_A;
            s_config.current_sign[i] = 1;
            s_config.energy_kwh[i] = 0.0;
        }
        s_config.phase_alpha[0] = DEFAULT_PHASE_I1;
        s_config.phase_alpha[1] = DEFAULT_PHASE_I2;
        s_config.phase_alpha[2] = DEFAULT_PHASE_I3;

        // Valores por defecto del sistema
        strncpy(s_sys_config.user_name, "Mi Hogar", sizeof(s_sys_config.user_name));
        s_sys_config.rooms_count = 3;
        strncpy(s_sys_config.room_names[0], "Tablero Principal", sizeof(s_sys_config.room_names[0]));
        strncpy(s_sys_config.room_names[1], "Habitacion 1", sizeof(s_sys_config.room_names[1]));
        strncpy(s_sys_config.room_names[2], "Habitacion 2 / Cocina", sizeof(s_sys_config.room_names[2]));
        s_sys_config.wifi_ssid[0] = '\0';
        s_sys_config.wifi_pass[0] = '\0';

        s_prefs.putUInt(PREFS_MAGIC_KEY, PREFS_MAGIC_VAL);
        s_prefs.putFloat("kv", s_config.kv);
        s_prefs.putFloat("ki0", s_config.ki[0]);
        s_prefs.putFloat("ki1", s_config.ki[1]);
        s_prefs.putFloat("ki2", s_config.ki[2]);
        s_prefs.putFloat("nth0", s_config.noise_thresh[0]);
        s_prefs.putFloat("nth1", s_config.noise_thresh[1]);
        s_prefs.putFloat("nth2", s_config.noise_thresh[2]);
        s_prefs.putFloat("pha0", s_config.phase_alpha[0]);
        s_prefs.putFloat("pha1", s_config.phase_alpha[1]);
        s_prefs.putFloat("pha2", s_config.phase_alpha[2]);
        s_prefs.putChar("sgn0", (int8_t)s_config.current_sign[0]);
        s_prefs.putChar("sgn1", (int8_t)s_config.current_sign[1]);
        s_prefs.putChar("sgn2", (int8_t)s_config.current_sign[2]);
        s_prefs.putDouble("e0", 0.0);
        s_prefs.putDouble("e1", 0.0);
        s_prefs.putDouble("e2", 0.0);

        s_prefs.putString("usr", s_sys_config.user_name);
        s_prefs.putUChar("rooms", s_sys_config.rooms_count);
        s_prefs.putString("rn0", s_sys_config.room_names[0]);
        s_prefs.putString("rn1", s_sys_config.room_names[1]);
        s_prefs.putString("rn2", s_sys_config.room_names[2]);
        s_prefs.putString("ssid", s_sys_config.wifi_ssid);
        s_prefs.putString("pass", s_sys_config.wifi_pass);
    }

    s_prefs.end();
    xSemaphoreGive(s_calib_mutex);
}

CalibrationConfig calibration_get() {
    CalibrationConfig cfg;
    if (s_calib_mutex) xSemaphoreTake(s_calib_mutex, portMAX_DELAY);
    cfg = s_config;
    if (s_calib_mutex) xSemaphoreGive(s_calib_mutex);
    return cfg;
}

void calibration_set(const CalibrationConfig &cfg) {
    if (s_calib_mutex) xSemaphoreTake(s_calib_mutex, portMAX_DELAY);
    s_config = cfg;
    if (s_calib_mutex) xSemaphoreGive(s_calib_mutex);
}

bool calibration_save() {
    if (s_calib_mutex) xSemaphoreTake(s_calib_mutex, portMAX_DELAY);

    s_prefs.begin(PREFS_NAMESPACE, false);
    s_prefs.putUInt(PREFS_MAGIC_KEY, PREFS_MAGIC_VAL);
    s_prefs.putFloat("kv", s_config.kv);
    s_prefs.putFloat("ki0", s_config.ki[0]);
    s_prefs.putFloat("ki1", s_config.ki[1]);
    s_prefs.putFloat("ki2", s_config.ki[2]);
    s_prefs.putFloat("nth0", s_config.noise_thresh[0]);
    s_prefs.putFloat("nth1", s_config.noise_thresh[1]);
    s_prefs.putFloat("nth2", s_config.noise_thresh[2]);
    s_prefs.putFloat("pha0", s_config.phase_alpha[0]);
    s_prefs.putFloat("pha1", s_config.phase_alpha[1]);
    s_prefs.putFloat("pha2", s_config.phase_alpha[2]);
    s_prefs.putChar("sgn0", (int8_t)s_config.current_sign[0]);
    s_prefs.putChar("sgn1", (int8_t)s_config.current_sign[1]);
    s_prefs.putChar("sgn2", (int8_t)s_config.current_sign[2]);
    s_prefs.putDouble("e0", s_config.energy_kwh[0]);
    s_prefs.putDouble("e1", s_config.energy_kwh[1]);
    s_prefs.putDouble("e2", s_config.energy_kwh[2]);
    s_prefs.end();

    if (s_calib_mutex) xSemaphoreGive(s_calib_mutex);
    return true;
}

void calibration_reset_defaults() {
    if (s_calib_mutex) xSemaphoreTake(s_calib_mutex, portMAX_DELAY);

    s_config.kv = DEFAULT_KV;
    for (int i = 0; i < 3; i++) {
        s_config.ki[i] = DEFAULT_KI;
        s_config.noise_thresh[i] = DEFAULT_NOISE_THRESH_A;
        s_config.current_sign[i] = 1;
        s_config.energy_kwh[i] = 0.0;
    }
    s_config.phase_alpha[0] = DEFAULT_PHASE_I1;
    s_config.phase_alpha[1] = DEFAULT_PHASE_I2;
    s_config.phase_alpha[2] = DEFAULT_PHASE_I3;

    s_prefs.begin(PREFS_NAMESPACE, false);
    s_prefs.clear();
    s_prefs.end();

    if (s_calib_mutex) xSemaphoreGive(s_calib_mutex);

    calibration_save();
    system_config_save();
}

void calibration_accumulate_energy(int channel_idx, double delta_kwh) {
    if (channel_idx < 0 || channel_idx >= 3) return;
    if (s_calib_mutex) xSemaphoreTake(s_calib_mutex, portMAX_DELAY);
    s_config.energy_kwh[channel_idx] += delta_kwh;
    if (s_calib_mutex) xSemaphoreGive(s_calib_mutex);
}

void calibration_save_energy() {
    if (s_calib_mutex) xSemaphoreTake(s_calib_mutex, portMAX_DELAY);
    s_prefs.begin(PREFS_NAMESPACE, false);
    s_prefs.putDouble("e0", s_config.energy_kwh[0]);
    s_prefs.putDouble("e1", s_config.energy_kwh[1]);
    s_prefs.putDouble("e2", s_config.energy_kwh[2]);
    s_prefs.end();
    if (s_calib_mutex) xSemaphoreGive(s_calib_mutex);
}

void calibration_zero_energy() {
    if (s_calib_mutex) xSemaphoreTake(s_calib_mutex, portMAX_DELAY);
    s_config.energy_kwh[0] = 0.0;
    s_config.energy_kwh[1] = 0.0;
    s_config.energy_kwh[2] = 0.0;
    s_prefs.begin(PREFS_NAMESPACE, false);
    s_prefs.putDouble("e0", 0.0);
    s_prefs.putDouble("e1", 0.0);
    s_prefs.putDouble("e2", 0.0);
    s_prefs.end();
    if (s_calib_mutex) xSemaphoreGive(s_calib_mutex);
}

void calibration_adjust_kv(float v_real, float measured_v_adc) {
    if (measured_v_adc <= 0.001f) return;
    if (s_calib_mutex) xSemaphoreTake(s_calib_mutex, portMAX_DELAY);
    s_config.kv = v_real / measured_v_adc;
    if (s_calib_mutex) xSemaphoreGive(s_calib_mutex);
}

void calibration_adjust_ki(int channel_idx, float i_real, float measured_i_adc) {
    if (channel_idx < 0 || channel_idx >= 3) return;
    if (measured_i_adc <= 0.0001f) return;
    if (s_calib_mutex) xSemaphoreTake(s_calib_mutex, portMAX_DELAY);
    s_config.ki[channel_idx] = i_real / measured_i_adc;
    if (s_calib_mutex) xSemaphoreGive(s_calib_mutex);
}

void calibration_adjust_phase_degrees(int channel_idx, float degrees) {
    if (channel_idx < 0 || channel_idx >= 3) return;
    float alpha = degrees / 5.4f;
    if (s_calib_mutex) xSemaphoreTake(s_calib_mutex, portMAX_DELAY);
    s_config.phase_alpha[channel_idx] = alpha;
    if (s_calib_mutex) xSemaphoreGive(s_calib_mutex);
}

SystemConfig system_config_get() {
    SystemConfig cfg;
    if (s_calib_mutex) xSemaphoreTake(s_calib_mutex, portMAX_DELAY);
    cfg = s_sys_config;
    if (s_calib_mutex) xSemaphoreGive(s_calib_mutex);
    return cfg;
}

void system_config_set(const SystemConfig &cfg) {
    if (s_calib_mutex) xSemaphoreTake(s_calib_mutex, portMAX_DELAY);
    s_sys_config = cfg;
    if (s_calib_mutex) xSemaphoreGive(s_calib_mutex);
}

bool system_config_save() {
    if (s_calib_mutex) xSemaphoreTake(s_calib_mutex, portMAX_DELAY);
    s_prefs.begin(PREFS_NAMESPACE, false);
    s_prefs.putString("usr", s_sys_config.user_name);
    s_prefs.putUChar("rooms", s_sys_config.rooms_count);
    s_prefs.putString("rn0", s_sys_config.room_names[0]);
    s_prefs.putString("rn1", s_sys_config.room_names[1]);
    s_prefs.putString("rn2", s_sys_config.room_names[2]);
    s_prefs.putString("ssid", s_sys_config.wifi_ssid);
    s_prefs.putString("pass", s_sys_config.wifi_pass);
    s_prefs.end();
    if (s_calib_mutex) xSemaphoreGive(s_calib_mutex);
    return true;
}
