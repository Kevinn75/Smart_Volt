#include "adc_dma.h"
#include <esp_log.h>

static const char *TAG = "ADC_DMA";

static esp_adc_cal_characteristics_t s_adc_chars;
static bool s_calibrated = false;

// Tabla de consulta (LUT) para conversión O(1) de 12 bits a mV (8 KB de SRAM)
static uint16_t s_raw_to_mv_lut[4096];

esp_err_t adc_dma_init() {
  ESP_LOGI(TAG, "Configurando ADC continuo DMA a %d Hz...",
           ADC_DMA_SAMPLE_FREQ_HZ);

  // 1. Caracterización con esquema de Espressif (Curve Fitting / Two-Point) a
  // 12 dB
  esp_adc_cal_value_t cal_val = esp_adc_cal_characterize(
      ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 0, &s_adc_chars);

  if (cal_val == ESP_ADC_CAL_VAL_EFUSE_TP_FIT) {
    ESP_LOGI(TAG, "ADC calibrado con Curve Fitting almacenado en eFuse");
    s_calibrated = true;
  } else if (cal_val == ESP_ADC_CAL_VAL_EFUSE_TP) {
    ESP_LOGI(TAG, "ADC calibrado con Two-Point en eFuse");
    s_calibrated = true;
  } else {
    ESP_LOGW(TAG, "Calibración por defecto para atenuación de 12 dB");
    s_calibrated = false;
  }

  // 2. Precalcular la LUT
  for (int raw = 0; raw < 4096; raw++) {
    uint32_t mv = esp_adc_cal_raw_to_voltage(raw, &s_adc_chars);
    s_raw_to_mv_lut[raw] = (mv > 65535) ? 65535 : (uint16_t)mv;
  }

  // 3. Inicialización del driver DMA
  uint32_t chan_mask =
      (1 << ADC_CHANNEL_VOLT) | (1 << ADC_CHANNEL_CURRENT_MAIN) |
      (1 << ADC_CHANNEL_CURRENT_AUX1) | (1 << ADC_CHANNEL_CURRENT_AUX2);

  adc_digi_init_config_t init_config = {
      .max_store_buf_size = ADC_DMA_STORE_BUF_SIZE,
      .conv_num_each_intr = ADC_DMA_CONV_NUM_EACH_INTR,
      .adc1_chan_mask = chan_mask,
      .adc2_chan_mask = 0,
  };

  esp_err_t ret = adc_digi_initialize(&init_config);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Fallo en adc_digi_initialize: %s", esp_err_to_name(ret));
    return ret;
  }

  // 4. Configurar patrón de los 4 canales en secuencia
  adc_digi_pattern_config_t patterns[ADC_NUM_CHANNELS];

  patterns[0].atten = ADC_ATTEN_DB_12;
  patterns[0].channel = ADC_CHANNEL_VOLT;
  patterns[0].unit = ADC_UNIT_1;
  patterns[0].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

  patterns[1].atten = ADC_ATTEN_DB_12;
  patterns[1].channel = ADC_CHANNEL_CURRENT_MAIN;
  patterns[1].unit = ADC_UNIT_1;
  patterns[1].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

  patterns[2].atten = ADC_ATTEN_DB_12;
  patterns[2].channel = ADC_CHANNEL_CURRENT_AUX1;
  patterns[2].unit = ADC_UNIT_1;
  patterns[2].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

  patterns[3].atten = ADC_ATTEN_DB_12;
  patterns[3].channel = ADC_CHANNEL_CURRENT_AUX2;
  patterns[3].unit = ADC_UNIT_1;
  patterns[3].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

  adc_digi_configuration_t dig_cfg = {
      .conv_limit_en = false,
      .conv_limit_num = 250,
      .pattern_num = ADC_NUM_CHANNELS,
      .adc_pattern = patterns,
      .sample_freq_hz = ADC_DMA_SAMPLE_FREQ_HZ,
      .conv_mode = ADC_CONV_SINGLE_UNIT_1,
      .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
  };

  ret = adc_digi_controller_configure(&dig_cfg);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Fallo en adc_digi_controller_configure: %s",
             esp_err_to_name(ret));
    return ret;
  }

  // 5. Iniciar DMA
  ret = adc_digi_start();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Fallo en adc_digi_start: %s", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGI(TAG, "Driver ADC DMA iniciado exitosamente");
  return ESP_OK;
}

esp_err_t adc_dma_read(uint8_t *dest, uint32_t max_bytes, uint32_t *bytes_read,
                       uint32_t timeout_ms) {
  return adc_digi_read_bytes(dest, max_bytes, bytes_read, timeout_ms);
}

uint16_t adc_dma_code_to_mv(uint16_t raw_code) {
  if (raw_code >= 4096)
    raw_code = 4095;
  return s_raw_to_mv_lut[raw_code];
}

int adc_dma_channel_to_index(uint8_t hw_channel) {
  switch (hw_channel) {
  case ADC_CHANNEL_VOLT:
    return CH_INDEX_VOLT;
  case ADC_CHANNEL_CURRENT_MAIN:
    return CH_INDEX_CURRENT_MAIN;
  case ADC_CHANNEL_CURRENT_AUX1:
    return CH_INDEX_CURRENT_AUX1;
  case ADC_CHANNEL_CURRENT_AUX2:
    return CH_INDEX_CURRENT_AUX2;
  default:
    return -1;
  }
}

const esp_adc_cal_characteristics_t *adc_dma_get_cal_chars() {
  return &s_adc_chars;
}
