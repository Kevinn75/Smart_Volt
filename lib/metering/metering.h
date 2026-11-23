#ifndef METERING_H
#define METERING_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "board_pins.h"
#include "adc_dma.h"
#include "calibration.h"
#include "zmpt101b.h"
#include "sct013.h"

#define METERING_WINDOW_CYCLES          10
#define METERING_WINDOW_SAMPLES         667
#define METERING_WINDOWS_PER_SEC        6

struct MeteringReport {
    // Magnitudes AC calculadas
    float v_rms;                // Tensión RMS en Volts
    float i_rms[3];             // Corrientes RMS en Amperes (I1, I2, I3)
    float p_active[3];          // Potencia activa en Watts (P1, P2, P3)
    float s_apparent[3];        // Potencia aparente en VA (S1, S2, S3)
    float q_reactive[3];        // Potencia reactiva en VAR (Q1, Q2, Q3)
    float power_factor[3];      // Factor de potencia [-1, 1] (FP1, FP2, FP3)
    double energy_kwh[3];       // Energía activa acumulada en kWh

    // Tensiones ADC directas (en Volts) para calibración sin carga
    float v_adc_raw_rms;        // Vrms medido en el pin ADC (en Volts)
    float i_adc_raw_rms[3];     // Irms medido en los pines ADC (en Volts)

    // Estadísticas RAW en mV (para modo 'raw' y verificar centros ~1470 mV y ~1650 mV)
    ChannelRawStats stats[ADC_NUM_CHANNELS];

    uint32_t timestamp_ms;
};

// Cola de comunicación entre Core 1 (metering) y Core 0 (console/web)
extern QueueHandle_t g_metering_queue;

/**
 * Inicializa la tarea de medición en Core 1 y la cola FreeRTOS
 */
void metering_init();

#endif // METERING_H
