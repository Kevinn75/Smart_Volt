#include "sensors.h"
#include "config.h"
#include <EmonLib.h>  // Librería para muestreo RMS[cite: 1]

static EnergyMonitor emon;

void inicializarSensores() {
    // Configura los pines y factores de calibración en EmonLib[cite: 1]
    emon.voltage(PIN_ZMPT101B, CALIB_VOLTAJE, FASE_VOLTAJE);
    emon.current(PIN_SCT013, CALIB_CORRIENTE);
}

LecturasEnergia leerEnergiaAC() {
    // Muestrea 20 ciclos completos de onda senoidal AC (~333ms a 60Hz)[cite: 1]
    emon.calcVI(20, 2000);

    LecturasEnergia datos;
    datos.vRMS = emon.Vrms;
    datos.iRMS = emon.Irms;
    datos.potenciaActiva = emon.realPower;
    datos.factorPotencia = emon.powerFactor;

    // Filtro para pequeñas lecturas de ruido en vacío
    if (datos.iRMS < 0.10f) {
        datos.iRMS = 0.0f;
        datos.potenciaActiva = 0.0f;
    }

    return datos;
}