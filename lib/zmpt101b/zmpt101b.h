#ifndef ZMPT101B_H
#define ZMPT101B_H

#include <Arduino.h>

/**
 * ============================================================================
 * SENSOR DE TENSIÓN AC: ZMPT101B
 * ============================================================================
 * - Alimentación del módulo: 5 V
 * - Salida del módulo centrada en: ~2.5 V
 * - Divisor de acondicionamiento: 3.3 kΩ (R1) / 4.7 kΩ (R2)
 *   Factor de atenuación: 4.7 / (3.3 + 4.7) ≈ 0.5875 (0.588)
 * - Centro DC en el pin ADC: ~1.47 V (1470 mV)
 * - Tensión nominal de red: 120 VAC RMS / 60 Hz (Colombia)
 * - Tensión de pico nominal: 120 * √2 ≈ 169.7 Vpk
 * - Rango seguro en el ADC de 12 dB: 100 mV a 3000 mV (evita saturación en rieles)
 */

#define ZMPT101B_BIAS_NOMINAL_MV        1470    // Centro DC esperado
#define ZMPT101B_MIN_SAFE_MV            100     // Límite inferior seguro
#define ZMPT101B_MAX_SAFE_MV            3000    // Límite superior seguro
#define ZMPT101B_DIVIDER_FACTOR         0.5875f // Divisor 3.3k / 4.7k

class Zmpt101bSensor {
public:
    Zmpt101bSensor() {}

    /**
     * Calcula la tensión real AC en Volts a partir del valor medido por el ADC (en mV) y el factor KV
     * @param vadc_rms_mv Tensión RMS en el pin ADC en milivoltios
     * @param kv Constante de calibración (V reales por V en ADC)
     * @return Tensión RMS de la red en Volts
     */
    static float calculateVrms(float vadc_rms_mv, float kv) {
        float vadc_volts = vadc_rms_mv / 1000.0f;
        return vadc_volts * kv;
    }

    /**
     * Comprueba si la lectura del sensor satura los límites dinámicos del ADC
     */
    static bool isSaturated(uint16_t min_mv, uint16_t max_mv) {
        return (min_mv <= ZMPT101B_MIN_SAFE_MV) || (max_mv >= ZMPT101B_MAX_SAFE_MV);
    }

    /**
     * Verifica si el bias DC en reposo está dentro del rango admisible (±150 mV del centro)
     */
    static bool isBiasValid(uint16_t avg_mv) {
        return (avg_mv >= (ZMPT101B_BIAS_NOMINAL_MV - 150)) && 
               (avg_mv <= (ZMPT101B_BIAS_NOMINAL_MV + 150));
    }
};

#endif // ZMPT101B_H
