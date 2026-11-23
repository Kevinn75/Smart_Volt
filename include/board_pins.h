#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include <Arduino.h>
#include <driver/adc.h>

/**
 * ============================================================================
 * SMART VOLT - MAPA CENTRAL DE PINES DE HARDWARE (ESP32-S3-WROOM-1-N16R8)
 * Red AC: 120 VAC / 60 Hz monofásica (Colombia)
 * ============================================================================
 */

// ----------------------------------------------------------------------------
// 1. ENTRADAS ANALÓGICAS (Exclusivamente en ADC1 - ADC2 NUNCA se usa)
// ----------------------------------------------------------------------------

// Tensión AC: ZMPT101B alimentado a 5 V, divisor 3.3 kΩ / 4.7 kΩ (factor ≈ 0.588)
// Centro DC teórico ≈ 1.47 V (1470 mV), pico máximo teórico ≈ 2.94 V
#define PIN_ADC_VOLT                1   // GPIO 1 -> ADC1_CHANNEL_0
#define ADC_CHANNEL_VOLT            ADC1_CHANNEL_0

// Corriente Principal: SCT-013-000 (100 A : 50 mA, N = 2000), Burden 33.3 Ω (3 x 100 Ω)
// Bias DC teórico ≈ 1.65 V (1650 mV) mediante divisor 10k/10k + 10 µF
#define PIN_ADC_CURRENT_MAIN        2   // GPIO 2 -> ADC1_CHANNEL_1
#define ADC_CHANNEL_CURRENT_MAIN    ADC1_CHANNEL_1

// Corriente Auxiliar 1 (SH1): SCT-013-000, Burden 33.3 Ω, Bias 1.65 V
#define PIN_ADC_CURRENT_AUX1        4   // GPIO 4 -> ADC1_CHANNEL_3
#define ADC_CHANNEL_CURRENT_AUX1    ADC1_CHANNEL_3

// Corriente Auxiliar 2 (SH2): SCT-013-000, Burden 33.3 Ω, Bias 1.65 V
#define PIN_ADC_CURRENT_AUX2        5   // GPIO 5 -> ADC1_CHANNEL_4
#define ADC_CHANNEL_CURRENT_AUX2    ADC1_CHANNEL_4

// Número de canales muestreados
#define ADC_NUM_CHANNELS            4

// ----------------------------------------------------------------------------
// 2. SALIDAS DE ALERTA Y SEÑALIZACIÓN (Inicializadas en LOW)
// ----------------------------------------------------------------------------
#define PIN_BUZZER                  11  // MOSFET low-side, activo en ALTO
#define PIN_LED_ALARM               12  // LED de advertencia, activo en ALTO

// ----------------------------------------------------------------------------
// 3. PINES RESERVADOS - ¡ESTRICTAMENTE NO TOCAR EN EL MUESTREO DE ENERGÍA!
// ----------------------------------------------------------------------------

// Ethernet ENC28J60 (Bus SPI dedicado)
#define PIN_RESERVED_ENC_SCK        39
#define PIN_RESERVED_ENC_MISO       40
#define PIN_RESERVED_ENC_MOSI       38
#define PIN_RESERVED_ENC_CS         41  // Pull-up externa de 10 kΩ
#define PIN_RESERVED_ENC_INT        42
#define PIN_RESERVED_ENC_RESET      47

// USB Nativo CDC (Comunicación y Programación)
#define PIN_RESERVED_USB_DM         19  // D-
#define PIN_RESERVED_USB_DP         20  // D+

// UART0 (Puerto Serie Primario / Bootloader)
#define PIN_RESERVED_UART0_TX       43
#define PIN_RESERVED_UART0_RX       44

// Pin de BOOT
#define PIN_RESERVED_BOOT           0

// Bus PSRAM Octal (OPI) en ESP32-S3-WROOM-1-N16R8
#define PIN_RESERVED_PSRAM_IO35     35
#define PIN_RESERVED_PSRAM_IO36     36
#define PIN_RESERVED_PSRAM_IO37     37

// Bus Flash SPI (GPIO 26 a 32 reservados internamente para Flash de 16 MB)
// GPIO 26, 27, 28, 29, 30, 31, 32

#endif // BOARD_PINS_H
