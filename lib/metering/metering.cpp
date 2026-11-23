#include "metering.h"
#include <esp_log.h>
#include <math.h>

static const char *TAG = "METERING";

QueueHandle_t g_metering_queue = NULL;

static void metering_task(void *pvParameters);

void metering_init() {
    g_metering_queue = xQueueCreate(2, sizeof(MeteringReport));

    xTaskCreatePinnedToCore(
        metering_task,
        "metering_task",
        8192,
        NULL,
        configMAX_PRIORITIES - 2, // Prioridad alta
        NULL,
        1                         // Core 1
    );

    ESP_LOGI(TAG, "Tarea metering_task iniciada en Core 1");
}

static void metering_task(void *pvParameters) {
    (void)pvParameters;

    static uint16_t s_channel_samples[ADC_NUM_CHANNELS][METERING_WINDOW_SAMPLES];
    static uint16_t s_channel_counts[ADC_NUM_CHANNELS] = {0};

    ChannelRawStats sec_stats[ADC_NUM_CHANNELS];
    for (int ch = 0; ch < ADC_NUM_CHANNELS; ch++) {
        sec_stats[ch].count = 0;
        sec_stats[ch].sum_mv = 0;
        sec_stats[ch].min_mv = 0xFFFF;
        sec_stats[ch].max_mv = 0;
    }

    float sum_v_rms = 0.0f;
    float sum_v_adc_raw = 0.0f;
    float sum_i_rms[3] = {0};
    float sum_i_adc_raw[3] = {0};
    float sum_p[3] = {0};
    float sum_s[3] = {0};
    int window_count = 0;

    uint32_t last_nvs_energy_save = millis();

    const uint32_t DMA_READ_SIZE = 512;
    uint8_t dma_read_buf[DMA_READ_SIZE];
    uint32_t bytes_read = 0;

    float prev_v_sample = 0.0f;

    ESP_LOGI(TAG, "Bucle de medición DMA activo");

    while (1) {
        esp_err_t res = adc_dma_read(dma_read_buf, DMA_READ_SIZE, &bytes_read, 20 / portTICK_PERIOD_MS);

        if (res == ESP_OK && bytes_read > 0) {
            uint32_t num_records = bytes_read / sizeof(adc_digi_output_data_t);
            adc_digi_output_data_t *p_data = (adc_digi_output_data_t *)dma_read_buf;

            for (uint32_t i = 0; i < num_records; i++) {
                uint8_t hw_chan = p_data[i].type2.channel;
                uint16_t raw_code = p_data[i].type2.data;

                int ch_idx = adc_dma_channel_to_index(hw_chan);
                if (ch_idx >= 0 && ch_idx < ADC_NUM_CHANNELS) {
                    uint16_t mv = adc_dma_code_to_mv(raw_code);

                    sec_stats[ch_idx].count++;
                    sec_stats[ch_idx].sum_mv += mv;
                    if (mv < sec_stats[ch_idx].min_mv) sec_stats[ch_idx].min_mv = mv;
                    if (mv > sec_stats[ch_idx].max_mv) sec_stats[ch_idx].max_mv = mv;

                    if (s_channel_counts[ch_idx] < METERING_WINDOW_SAMPLES) {
                        s_channel_samples[ch_idx][s_channel_counts[ch_idx]] = mv;
                        s_channel_counts[ch_idx]++;
                    }
                }
            }

            bool window_ready = true;
            for (int ch = 0; ch < ADC_NUM_CHANNELS; ch++) {
                if (s_channel_counts[ch] < METERING_WINDOW_SAMPLES) {
                    window_ready = false;
                    break;
                }
            }

            if (window_ready) {
                for (int ch = 0; ch < ADC_NUM_CHANNELS; ch++) {
                    s_channel_counts[ch] = 0;
                }

                CalibrationConfig cfg = calibration_get();

                // 1. Quitar offset DC restando la media
                float v_mean = 0.0f;
                float i_mean[3] = {0.0f, 0.0f, 0.0f};

                for (int n = 0; n < METERING_WINDOW_SAMPLES; n++) {
                    v_mean += s_channel_samples[CH_INDEX_VOLT][n];
                    i_mean[0] += s_channel_samples[CH_INDEX_CURRENT_MAIN][n];
                    i_mean[1] += s_channel_samples[CH_INDEX_CURRENT_AUX1][n];
                    i_mean[2] += s_channel_samples[CH_INDEX_CURRENT_AUX2][n];
                }
                v_mean /= (float)METERING_WINDOW_SAMPLES;
                i_mean[0] /= (float)METERING_WINDOW_SAMPLES;
                i_mean[1] /= (float)METERING_WINDOW_SAMPLES;
                i_mean[2] /= (float)METERING_WINDOW_SAMPLES;

                // 2. RMS de tensión
                double sum_sq_v = 0.0;
                for (int n = 0; n < METERING_WINDOW_SAMPLES; n++) {
                    float v_ac = (float)s_channel_samples[CH_INDEX_VOLT][n] - v_mean;
                    sum_sq_v += (double)(v_ac * v_ac);
                }
                float v_rms_mv = sqrtf((float)(sum_sq_v / METERING_WINDOW_SAMPLES));
                float v_adc_v = v_rms_mv / 1000.0f;
                float v_real = Zmpt101bSensor::calculateVrms(v_rms_mv, cfg.kv);

                // 3. RMS de corriente, corrección de fase e integración de potencia
                float i_real[3] = {0};
                float i_adc_v[3] = {0};
                float p_act[3] = {0};
                float s_app[3] = {0};

                for (int k = 0; k < 3; k++) {
                    int ch_idx = k + 1;
                    float alpha = cfg.phase_alpha[k];
                    double sum_sq_i = 0.0;
                    double sum_vi = 0.0;

                    for (int n = 0; n < METERING_WINDOW_SAMPLES; n++) {
                        float i_ac = (float)s_channel_samples[ch_idx][n] - i_mean[k];
                        sum_sq_i += (double)(i_ac * i_ac);

                        float v_now = (float)s_channel_samples[CH_INDEX_VOLT][n] - v_mean;
                        float v_past = (n == 0) ? prev_v_sample : ((float)s_channel_samples[CH_INDEX_VOLT][n - 1] - v_mean);

                        // Corrección de fase secuencial: V_corr = V[n-1] + alpha * (V[n] - V[n-1])
                        float v_corr = v_past + alpha * (v_now - v_past);

                        sum_vi += (double)(v_corr * i_ac);
                    }

                    float i_rms_mv = sqrtf((float)(sum_sq_i / METERING_WINDOW_SAMPLES));
                    i_adc_v[k] = i_rms_mv / 1000.0f;
                    i_real[k] = Sct013Sensor::calculateIrms(i_rms_mv, cfg.ki[k], cfg.noise_thresh[k]);

                    float p_mw_raw = (float)(sum_vi / METERING_WINDOW_SAMPLES);
                    float p_watts = (p_mw_raw / 1000000.0f) * cfg.kv * cfg.ki[k] * (float)cfg.current_sign[k];

                    if (i_real[k] < cfg.noise_thresh[k]) {
                        i_real[k] = 0.0f;
                        p_watts = 0.0f;
                    }

                    p_act[k] = p_watts;
                    s_app[k] = v_real * i_real[k];
                }

                prev_v_sample = (float)s_channel_samples[CH_INDEX_VOLT][METERING_WINDOW_SAMPLES - 1] - v_mean;

                sum_v_rms += v_real;
                sum_v_adc_raw += v_adc_v;
                for (int k = 0; k < 3; k++) {
                    sum_i_rms[k] += i_real[k];
                    sum_i_adc_raw[k] += i_adc_v[k];
                    sum_p[k] += p_act[k];
                    sum_s[k] += s_app[k];
                }
                window_count++;

                // 4. Cada 6 ventanas (1 segundo)
                if (window_count >= METERING_WINDOWS_PER_SEC) {
                    MeteringReport report;
                    report.timestamp_ms = millis();
                    report.v_rms = sum_v_rms / (float)window_count;
                    report.v_adc_raw_rms = sum_v_adc_raw / (float)window_count;

                    const double sec_to_kwh = 1.0 / (3600.0 * 1000.0);

                    for (int k = 0; k < 3; k++) {
                        report.i_rms[k] = sum_i_rms[k] / (float)window_count;
                        report.i_adc_raw_rms[k] = sum_i_adc_raw[k] / (float)window_count;
                        report.p_active[k] = sum_p[k] / (float)window_count;
                        report.s_apparent[k] = sum_s[k] / (float)window_count;

                        float s_val = report.s_apparent[k];
                        float p_val = report.p_active[k];
                        float diff = (s_val * s_val) - (p_val * p_val);
                        report.q_reactive[k] = (diff > 0.0f) ? sqrtf(diff) : 0.0f;

                        if (s_val > 0.01f) {
                            float fp = p_val / s_val;
                            if (fp > 1.0f) fp = 1.0f;
                            if (fp < -1.0f) fp = -1.0f;
                            report.power_factor[k] = fp;
                        } else {
                            report.power_factor[k] = 0.0f;
                        }

                        if (p_val > 0.0f) {
                            double delta_kwh = (double)p_val * sec_to_kwh;
                            calibration_accumulate_energy(k, delta_kwh);
                        }
                    }

                    CalibrationConfig latest_cfg = calibration_get();
                    for (int k = 0; k < 3; k++) {
                        report.energy_kwh[k] = latest_cfg.energy_kwh[k];
                    }

                    for (int ch = 0; ch < ADC_NUM_CHANNELS; ch++) {
                        if (sec_stats[ch].count > 0) {
                            sec_stats[ch].avg_mv = (uint16_t)(sec_stats[ch].sum_mv / sec_stats[ch].count);
                            sec_stats[ch].pk_to_pk_mv = sec_stats[ch].max_mv - sec_stats[ch].min_mv;
                        }
                        report.stats[ch] = sec_stats[ch];

                        sec_stats[ch].count = 0;
                        sec_stats[ch].sum_mv = 0;
                        sec_stats[ch].min_mv = 0xFFFF;
                        sec_stats[ch].max_mv = 0;
                    }

                    if (g_metering_queue != NULL) {
                        xQueueSend(g_metering_queue, &report, 0);
                    }

                    if (millis() - last_nvs_energy_save >= 300000) {
                        calibration_save_energy();
                        last_nvs_energy_save = millis();
                    }

                    sum_v_rms = 0.0f;
                    sum_v_adc_raw = 0.0f;
                    for (int k = 0; k < 3; k++) {
                        sum_i_rms[k] = 0.0f;
                        sum_i_adc_raw[k] = 0.0f;
                        sum_p[k] = 0.0f;
                        sum_s[k] = 0.0f;
                    }
                    window_count = 0;
                }
            }
        }
    }
}
