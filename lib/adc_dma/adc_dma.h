#ifndef ADC_DMA_H
#define ADC_DMA_H

#include "board_pins.h"
#include <Arduino.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <esp_err.h>

/**
 * Parámetros del ADC Continuo por DMA
 * - Tasa global: 16.000 muestras/segundo
 * - Con 4 canales en secuencia: 4.000 muestras/s por canal
 * - Resolución: 12 bits (0 a 4095)
 * - Atenuación: 12 dB (rango 0 a 3.1 V)
 */
#define ADC_DMA_SAMPLE_FREQ_HZ 16000
#define ADC_DMA_CONV_NUM_EACH_INTR 512
#define ADC_DMA_STORE_BUF_SIZE 4096

// Índices lógicos de los canales en los arrays de procesamiento
#define CH_INDEX_VOLT 0
#define CH_INDEX_CURRENT_MAIN 1
#define CH_INDEX_CURRENT_AUX1 2
#define CH_INDEX_CURRENT_AUX2 3

struct ChannelRawStats {
  uint32_t count;
  uint32_t sum_mv;
  uint16_t min_mv;
  uint16_t max_mv;
  uint16_t avg_mv;
  uint16_t pk_to_pk_mv;
};

/**
 * Inicializa el driver ADC1 continuo con DMA y la calibración eFuse (Curve
 * Fitting / Two-Point)
 */
esp_err_t adc_dma_init();

/**
 * Lee un lote de bytes desde el buffer DMA
 */
esp_err_t adc_dma_read(uint8_t *dest, uint32_t max_bytes, uint32_t *bytes_read,
                       uint32_t timeout_ms);

/**
 * Convierte un código crudo (0-4095) a mV en O(1) usando la LUT precalculada
 */
uint16_t adc_dma_code_to_mv(uint16_t raw_code);

/**
 * Mapea el número físico de canal de hardware de ADC1 al índice lógico (0..3)
 * Retorna -1 si el canal no pertenece al grupo de medición
 */
int adc_dma_channel_to_index(uint8_t hw_channel);

/**
 * Obtiene el puntero a las características de calibración de Espressif
 */
const esp_adc_cal_characteristics_t *adc_dma_get_cal_chars();

#endif // ADC_DMA_H
