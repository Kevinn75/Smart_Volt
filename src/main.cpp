#include <Arduino.h>
#include "board_pins.h"
#include "adc_dma.h"
#include "calibration.h"
#include "metering.h"
#include "console.h"
#include "web_server.h"

void setup() {
    // 1. Inicializar USB CDC Serial
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n==================================================");
    Serial.println("      SMART VOLT - MEDIDOR DE ENERGÍA AC          ");
    Serial.println("      ESP32-S3-WROOM-1-N16R8 (16MB Flash, 8MB PSRAM)");
    Serial.println("      Red: 120 VAC / 60 Hz monofásica             ");
    Serial.println("==================================================");

    // 2. Configurar salidas de alarma y dejarlas en LOW
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    pinMode(PIN_LED_ALARM, OUTPUT);
    digitalWrite(PIN_LED_ALARM, LOW);

    // 3. Inicializar constantes de calibración y NVS
    Serial.println("[INIT] Inicializando NVS y parámetros de calibración...");
    calibration_init();

    // 4. Inicializar driver ADC continuo por DMA (16 kS/s)
    Serial.println("[INIT] Inicializando ADC continuo por DMA (ADC1, 4 canales)...");
    esp_err_t err = adc_dma_init();
    if (err != ESP_OK) {
        Serial.printf("[ERROR] Falló la inicialización del ADC DMA: 0x%x\n", err);
    } else {
        Serial.println("[INIT] ADC DMA continuo iniciado correctamente.");
    }

    // 5. Inicializar subsistema de medición (metering_task en Core 1)
    Serial.println("[INIT] Iniciando tarea de medición en Core 1...");
    metering_init();

    // 6. Inicializar subsistema de consola y USB CDC (console_task en Core 0)
    Serial.println("[INIT] Iniciando consola en Core 0...");
    console_init();

    // 7. Registrar subsistema Web (preparado para fase de conectividad)
    Serial.println("[INIT] Registrando componente Web...");
    SmartVoltWebServer::init();

    Serial.println("[SISTEMA] Inicialización completa. Escribe 'raw' o 'show' para comenzar.");
    Serial.println("--------------------------------------------------");
}

void loop() {
    // Las tareas FreeRTOS (Core 1 para medición, Core 0 para consola) gestionan el sistema.
    vTaskDelay(pdMS_TO_TICKS(1000));
}
