#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "red.h"

void setup() {
    Serial.begin(115200);

    // Configuración de pines de alarma
    pinMode(PIN_LED_ALARM, OUTPUT);
    pinMode(PIN_BUZZER_ALARM, OUTPUT);
    digitalWrite(PIN_LED_ALARM, LOW);
    digitalWrite(PIN_BUZZER_ALARM, LOW);

    // Autocomprobación inicial de alarmas (200ms)[cite: 1]
    digitalWrite(PIN_LED_ALARM, HIGH);
    digitalWrite(PIN_BUZZER_ALARM, HIGH);
    delay(200);
    digitalWrite(PIN_LED_ALARM, LOW);
    digitalWrite(PIN_BUZZER_ALARM, LOW);

    // Inicializar subsistemas
    inicializarSensores();
    inicializarEthernet();

    Serial.println(">>> Sistema Medidor de Energía Inicializado <<<");
}

void loop() {
    LecturasEnergia lecturas = leerEnergiaAC();

    // Verificación de umbrales de alarma[cite: 1]
    if (lecturas.vRMS > UMBRAL_MAX_VOLTAJE || lecturas.iRMS > UMBRAL_MAX_CORRIENTE) {
        digitalWrite(PIN_LED_ALARM, HIGH);
        digitalWrite(PIN_BUZZER_ALARM, HIGH);
    } else {
        digitalWrite(PIN_LED_ALARM, LOW);
        digitalWrite(PIN_BUZZER_ALARM, LOW);
    }

    // Reporte de mediciones por el Puerto Serie[cite: 1]
    Serial.printf("Vrms: %.2f V | Irms: %.2f A | P: %.2f W | FP: %.2f | LAN: %s\n",
    lecturas.vRMS,
    lecturas.iRMS,
    lecturas.potenciaActiva,
    lecturas.factorPotencia,
    estaLanConectada() ? "OK" : "DESCONECTADO");

    delay(1000);
}