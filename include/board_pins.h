#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include <Arduino.h>

/**
 * ============================================================================
 * SMART VOLT - PINES (ESP32-S3-WROOM-1-N16R8)
 * Red AC: 120 VAC / 60 Hz monofásica (Colombia)
 * ============================================================================
 */

// Tensión AC: ZMPT101B alimentado a 5 V, divisor 3.3 kΩ / 4.7 kΩ (factor ≈
// 0.588) Centro DC teórico ≈ 1.47 V (1470 mV), pico máximo teórico ≈ 2.94 V
#define PIN_ADC_VOLT 1 // GPIO 1 -> ADC1_CHANNEL_0

// Corriente AC: SCT-013-000 (100 A : 50 mA, N = 2000), burden 33.3 Ω (3 x 100 Ω)
// Bias DC teórico ≈ 1.65 V (1650 mV) con divisor 10k/10k + 10 µF
#define PIN_ADC_CURRENT 2 // GPIO 2 -> ADC1_CHANNEL_1

// Salidas de alerta y señalización (ESP32-S3-DevKitC-1)
#define PIN_BUZZER 11     // Buzzer pasivo (señal PWM / tone)
#define PIN_LED_ALARM 12  // LED de advertencia, activo en ALTO

// Ethernet ENC28J60 (bus SPI dedicado). Se usa solo si ENABLE_ETHERNET = 1 (src/eth.h)
// Alimentar el módulo a 3,3 V. No usar GPIO 35-37 (PSRAM octal).
#define PIN_ENC_SCK   39
#define PIN_ENC_MISO  40
#define PIN_ENC_MOSI  38
#define PIN_ENC_CS    41  // pull-up externa de 10 kΩ
#define PIN_ENC_INT   42  // no se usa (la librería trabaja por consulta)
#define PIN_ENC_RESET 47

#endif // BOARD_PINS_H
