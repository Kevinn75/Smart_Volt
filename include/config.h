#ifndef CONFIG_H
#define CONFIG_H

// Asignación de Pines Analógicos y de Alarma
#define PIN_ZMPT101B      2  // Entrada ADC Voltaje (ADC1)[cite: 1]
#define PIN_SCT013        1 // Entrada ADC Corriente (ADC1)[cite: 1]
#define PIN_LED_ALARM     4  // Salida LED Indicador[cite: 1]
#define PIN_BUZZER_ALARM  8  // Salida Buzzer Activo[cite: 1]

// Pines del Módulo Ethernet LAN8720 (RMII)
#define ETH_ADDR          1
#define ETH_POWER_PIN     5  // Pin de control de alimentación LAN8720[cite: 1]
#define ETH_MDC_PIN       12  // Clock de gestión[cite: 1]
#define ETH_MDIO_PIN      18  // Datos de gestión[cite: 1]
#define ETH_TYPE          ETH_PHY_LAN8720
#define ETH_CLK_MODE      ETH_CLOCK_GPIO0_IN  // Reloj de 50MHz desde el módulo[cite: 1]
// Pines RMII adicionales para ESP32-S3 (necesarios en Arduino ESP32 v3.x / ESP-IDF)
#define ETH_TX0_PIN       9   // GPIO 9
#define ETH_TX1_PIN       11  // GPIO 11
#define ETH_TX_EN_PIN     21  // GPIO 21
#define ETH_RX0_PIN       13  // GPIO 13
#define ETH_RX1_PIN       14  // GPIO 14
#define ETH_CRS_PIN       10  // GPIO 10
// Factores de Calibración de EmonLib[cite: 1]
#define CALIB_VOLTAJE     230.0f
#define CALIB_CORRIENTE   60.6f
#define FASE_VOLTAJE      1.7f

// Umbrales para Activación de Alerta
#define UMBRAL_MAX_VOLTAJE   240.0f
#define UMBRAL_MAX_CORRIENTE 15.0f

#endif