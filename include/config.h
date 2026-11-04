#ifndef CONFIG_H
#define CONFIG_H

// Asignación de Pines Analógicos y de Alarma
#define PIN_ZMPT101B      34  // Entrada ADC Voltaje (ADC1)[cite: 1]
#define PIN_SCT013        35  // Entrada ADC Corriente (ADC1)[cite: 1]
#define PIN_LED_ALARM     12  // Salida LED Indicador[cite: 1]
#define PIN_BUZZER_ALARM  13  // Salida Buzzer Activo[cite: 1]

// Pines del Módulo Ethernet LAN8720 (RMII)
#define ETH_ADDR          1
#define ETH_POWER_PIN     16  // Pin de control de alimentación LAN8720[cite: 1]
#define ETH_MDC_PIN       23  // Clock de gestión[cite: 1]
#define ETH_MDIO_PIN      18  // Datos de gestión[cite: 1]
#define ETH_TYPE          ETH_PHY_LAN8720
#define ETH_CLK_MODE      ETH_CLOCK_GPIO0_IN  // Reloj de 50MHz desde el módulo[cite: 1]

// Factores de Calibración de EmonLib[cite: 1]
#define CALIB_VOLTAJE     230.0f
#define CALIB_CORRIENTE   60.6f
#define FASE_VOLTAJE      1.7f

// Umbrales para Activación de Alerta
#define UMBRAL_MAX_VOLTAJE   240.0f
#define UMBRAL_MAX_CORRIENTE 15.0f

#endif