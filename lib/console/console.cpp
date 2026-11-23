#include "console.h"
#include "metering.h"
#include "calibration.h"
#include "web_server.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static bool s_raw_mode = false;
static MeteringReport s_last_report;
static bool s_has_report = false;

static char s_cmd_buf[64];
static uint8_t s_cmd_len = 0;

static void parse_command(const char *cmd);

static void console_task(void *pvParameters) {
    (void)pvParameters;
    MeteringReport report;

    while (1) {
        if (g_metering_queue != NULL && xQueueReceive(g_metering_queue, &report, 50 / portTICK_PERIOD_MS) == pdTRUE) {
            s_last_report = report;
            s_has_report = true;
            SmartVoltWebServer::updateMetrics(report);

            if (s_raw_mode) {
                Serial.printf(
                    "[RAW] V: avg=%umV min=%umV max=%umV pk-pk=%umV | "
                    "I1: avg=%umV min=%umV max=%umV pk-pk=%umV | "
                    "I2: avg=%umV min=%umV max=%umV pk-pk=%umV | "
                    "I3: avg=%umV min=%umV max=%umV pk-pk=%umV\n",
                    report.stats[CH_INDEX_VOLT].avg_mv,
                    report.stats[CH_INDEX_VOLT].min_mv,
                    report.stats[CH_INDEX_VOLT].max_mv,
                    report.stats[CH_INDEX_VOLT].pk_to_pk_mv,
                    report.stats[CH_INDEX_CURRENT_MAIN].avg_mv,
                    report.stats[CH_INDEX_CURRENT_MAIN].min_mv,
                    report.stats[CH_INDEX_CURRENT_MAIN].max_mv,
                    report.stats[CH_INDEX_CURRENT_MAIN].pk_to_pk_mv,
                    report.stats[CH_INDEX_CURRENT_AUX1].avg_mv,
                    report.stats[CH_INDEX_CURRENT_AUX1].min_mv,
                    report.stats[CH_INDEX_CURRENT_AUX1].max_mv,
                    report.stats[CH_INDEX_CURRENT_AUX1].pk_to_pk_mv,
                    report.stats[CH_INDEX_CURRENT_AUX2].avg_mv,
                    report.stats[CH_INDEX_CURRENT_AUX2].min_mv,
                    report.stats[CH_INDEX_CURRENT_AUX2].max_mv,
                    report.stats[CH_INDEX_CURRENT_AUX2].pk_to_pk_mv
                );
            } else {
                Serial.printf(
                    "V=%.1fV | "
                    "I1=%.2fA P1=%.0fW S1=%.0fVA FP1=%.2f E1=%.3fkWh | "
                    "I2=%.2fA P2=%.0fW S2=%.0fVA FP2=%.2f E2=%.3fkWh | "
                    "I3=%.2fA P3=%.0fW S3=%.0fVA FP3=%.2f E3=%.3fkWh\n",
                    report.v_rms,
                    report.i_rms[0], report.p_active[0], report.s_apparent[0], report.power_factor[0], report.energy_kwh[0],
                    report.i_rms[1], report.p_active[1], report.s_apparent[1], report.power_factor[1], report.energy_kwh[1],
                    report.i_rms[2], report.p_active[2], report.s_apparent[2], report.power_factor[2], report.energy_kwh[2]
                );
            }
        }

        while (Serial.available() > 0) {
            char c = (char)Serial.read();
            if (c == '\r' || c == '\n') {
                if (s_cmd_len > 0) {
                    s_cmd_buf[s_cmd_len] = '\0';
                    parse_command(s_cmd_buf);
                    s_cmd_len = 0;
                }
            } else if (s_cmd_len < sizeof(s_cmd_buf) - 1) {
                s_cmd_buf[s_cmd_len++] = c;
            }
        }
    }
}

static void parse_command(const char *cmd) {
    while (*cmd == ' ') cmd++;

    if (strcmp(cmd, "raw") == 0) {
        s_raw_mode = !s_raw_mode;
        Serial.printf(">> Modo RAW: %s\n", s_raw_mode ? "ACTIVADO (centros en mV)" : "DESACTIVADO (magnitudes físicas)");
    } 
    else if (strncmp(cmd, "cal v", 5) == 0) {
        float v_real = atof(cmd + 5);
        if (v_real > 0.0f) {
            if (s_has_report && s_last_report.v_adc_raw_rms > 0.001f) {
                calibration_adjust_kv(v_real, s_last_report.v_adc_raw_rms);
                CalibrationConfig cfg = calibration_get();
                Serial.printf(">> Calibrado KV = %.2f (V_real=%.1f V, V_adc=%.4f V). Usa 'save' para persistir.\n",
                              cfg.kv, v_real, s_last_report.v_adc_raw_rms);
            } else {
                Serial.println(">> Error: No hay lecturas válidas de tensión aún.");
            }
        } else {
            Serial.println(">> Uso: cal v <Vreal> (Ejemplo: cal v 121.5)");
        }
    } 
    else if (strncmp(cmd, "cal i", 5) == 0) {
        int channel_num = cmd[5] - '0';
        if (channel_num >= 1 && channel_num <= 3) {
            float i_real = atof(cmd + 6);
            if (i_real >= 0.0f) {
                int ch_idx = channel_num - 1;
                if (s_has_report && s_last_report.i_adc_raw_rms[ch_idx] > 0.0001f) {
                    calibration_adjust_ki(ch_idx, i_real, s_last_report.i_adc_raw_rms[ch_idx]);
                    CalibrationConfig cfg = calibration_get();
                    Serial.printf(">> Calibrado KI%d = %.2f A/V (I_real=%.2f A, V_adc=%.4f V). Usa 'save' para persistir.\n",
                                  channel_num, cfg.ki[ch_idx], i_real, s_last_report.i_adc_raw_rms[ch_idx]);
                } else {
                    Serial.println(">> Error: No hay lectura de corriente válida en ese canal.");
                }
            } else {
                Serial.printf(">> Uso: cal i%d <Areal> (Ejemplo: cal i%d 4.50)\n", channel_num, channel_num);
            }
        } else {
            Serial.println(">> Uso: cal i<1..3> <Areal>");
        }
    } 
    else if (strncmp(cmd, "phase i", 7) == 0) {
        int channel_num = cmd[7] - '0';
        if (channel_num >= 1 && channel_num <= 3) {
            float deg = atof(cmd + 8);
            calibration_adjust_phase_degrees(channel_num - 1, deg);
            CalibrationConfig cfg = calibration_get();
            Serial.printf(">> Desfase canal I%d configurado a %.2f° (alpha=%.3f). Usa 'save' para persistir.\n",
                          channel_num, deg, cfg.phase_alpha[channel_num - 1]);
        } else {
            Serial.println(">> Uso: phase i<1..3> <grados> (Ejemplo: phase i1 1.35)");
        }
    } 
    else if (strcmp(cmd, "show") == 0) {
        CalibrationConfig cfg = calibration_get();
        Serial.println("\n========== CONSTANTES DE CALIBRACIÓN ==========");
        Serial.printf("KV : %.2f V/V (V_adc actual: %.4f V -> Vrms: %.1f V)\n", cfg.kv, s_last_report.v_adc_raw_rms, s_last_report.v_rms);
        for (int i = 0; i < 3; i++) {
            Serial.printf("I%d: KI=%.2f A/V | Phase_alpha=%.3f | Ruido=%.2f A | Signo=%d | E=%.3f kWh\n",
                          i + 1, cfg.ki[i], cfg.phase_alpha[i], cfg.noise_thresh[i], cfg.current_sign[i], cfg.energy_kwh[i]);
        }
        Serial.println("================================================");
    } 
    else if (strcmp(cmd, "save") == 0) {
        calibration_save();
        Serial.println(">> Configuración guardada exitosamente en NVS.");
    } 
    else if (strcmp(cmd, "reset") == 0) {
        calibration_reset_defaults();
        Serial.println(">> Valores de fábrica restaurados y guardados en NVS.");
    } 
    else if (strcmp(cmd, "zero e") == 0) {
        calibration_zero_energy();
        Serial.println(">> Energía acumulada reiniciada a 0.000 kWh.");
    } 
    else {
        Serial.println(">> Comandos disponibles:");
        Serial.println("   raw             : Alterna volcado de centros en mV y picos");
        Serial.println("   cal v <Vreal>   : Calibra KV con la lectura actual (Ej: cal v 120.5)");
        Serial.println("   cal i<n> <Areal>: Calibra KIn con la lectura actual (Ej: cal i1 4.82)");
        Serial.println("   phase i<n> <deg>: Ajusta desfase en grados (Ej: phase i1 1.35)");
        Serial.println("   show            : Muestra constantes y lecturas actuales");
        Serial.println("   save            : Guarda calibración en memoria flash NVS");
        Serial.println("   reset           : Restaura valores teóricos de fábrica");
        Serial.println("   zero e          : Reinicia el acumulador de energía (kWh)");
    }
}

void console_init() {
    xTaskCreatePinnedToCore(
        console_task,
        "console_task",
        4096,
        NULL,
        1,
        NULL,
        0 // Core 0
    );
}
