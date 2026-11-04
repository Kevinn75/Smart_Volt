#ifndef SENSORES_H
#define SENSORES_H

#include <Arduino.h>

struct LecturasEnergia {
    float vRMS;
    float iRMS;
    float potenciaActiva;
    float factorPotencia;
};

void inicializarSensores();
LecturasEnergia leerEnergiaAC();

#endif