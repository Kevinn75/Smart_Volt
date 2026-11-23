#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <Arduino.h>
#include "sct013.h"
#include "zmpt101b.h"

// Constantes teóricas de inicio
#define DEFAULT_KV              200.0f
#define DEFAULT_KI              SCT013_THEORETICAL_KI   // ≈ 60.06 A/V
#define DEFAULT_NOISE_THRESH_A  0.05f                   // Umbral de ruido 0.05 A
#define DEFAULT_PHASE_I1        0.25f                   // Corrección ranura 1 (~1.35° a 60 Hz)
#define DEFAULT_PHASE_I2        0.50f                   // Corrección ranura 2 (~2.70° a 60 Hz)
#define DEFAULT_PHASE_I3        0.75f                   // Corrección ranura 3 (~4.05° a 60 Hz)

struct CalibrationConfig {
    float kv;                   // Factor escala tensión (V reales / V ADC)
    float ki[3];                // Factor escala corriente (A reales / V ADC) para I1, I2, I3
    float noise_thresh[3];      // Umbral mínimo de corriente en Amperes
    float phase_alpha[3];       // Coeficiente alpha de interpolación secuencial por canal
    int8_t current_sign[3];     // Signo de corriente (+1 o -1 si la pinza está invertida)
    double energy_kwh[3];       // Energía acumulada por canal
};

struct SystemConfig {
    char user_name[32];         // Nombre del usuario / titular de la instalación
    uint8_t rooms_count;        // Cantidad de habitaciones/circuitos monitoreados (1 a 3)
    char room_names[3][32];     // Nombre personalizado de cada circuito/habitación
    char wifi_ssid[32];         // Nombre de la red Wi-Fi
    char wifi_pass[64];         // Contraseña de la red Wi-Fi
};

void calibration_init();
CalibrationConfig calibration_get();
void calibration_set(const CalibrationConfig &cfg);
bool calibration_save();
void calibration_reset_defaults();
void calibration_accumulate_energy(int channel_idx, double delta_kwh);
void calibration_save_energy();
void calibration_zero_energy();
void calibration_adjust_kv(float v_real, float measured_v_adc);
void calibration_adjust_ki(int channel_idx, float i_real, float measured_i_adc);
void calibration_adjust_phase_degrees(int channel_idx, float degrees);

// Configuración de usuario, habitaciones y Wi-Fi
SystemConfig system_config_get();
void system_config_set(const SystemConfig &cfg);
bool system_config_save();

#endif // CALIBRATION_H
