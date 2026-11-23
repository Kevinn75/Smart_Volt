#ifndef SCT013_H
#define SCT013_H

#include <Arduino.h>

/**
 * ============================================================================
 * SENSOR DE CORRIENTE AC: SCT-013-000 (Transformador de Corriente No Invasivo)
 * ============================================================================
 * - Relación de transformación: 100 A : 50 mA (N = 2000 espiras)
 * - Resistencia de carga (Burden): 33.3 Ω (3 resistencias de 100 Ω en paralelo
 * de precisión)
 * - Tensión secundaria a 100 A: 50 mA * 33.33 Ω = 1.666 V RMS
 * - Relación de conversión teórica:
 *   I_primaria = V_burden * (N / R_burden) = V_burden * (2000 / 33.333) ≈
 * V_burden * 60.06 A/V
 * - Circuito de polarización (Bias): Divisor resistivo 10 kΩ / 10 kΩ +
 * condensador 10 µF Tensión de reposo DC: 3.3 V / 2 = 1.65 V (1650 mV)
 * - Rango seguro en el ADC de 12 dB: 100 mV a 3000 mV
 */

#define SCT013_BIAS_NOMINAL_MV 1650 // Centro DC esperado
#define SCT013_MIN_SAFE_MV 100      // Límite inferior seguro
#define SCT013_MAX_SAFE_MV 3000     // Límite superior seguro
#define SCT013_TURNS_RATIO 2000.0f
#define SCT013_BURDEN_RESISTANCE_OHM 33.333f
#define SCT013_THEORETICAL_KI 60.06f // Constante teórica en A/V

class Sct013Sensor {
public:
  Sct013Sensor() {}

  /**
   * Calcula la corriente RMS real en Amperes
   * @param iadc_rms_mv Tensión RMS en el pin ADC en milivoltios
   * @param ki Constante de escala (A reales por V en ADC)
   * @param noise_threshold Umbral en Amperes para descartar ruido en vacío (ej.
   * 0.05 A)
   * @return Corriente RMS real en Amperes (o 0.0 si está bajo el umbral)
   */
  static float calculateIrms(float iadc_rms_mv, float ki,
                             float noise_threshold) {
    float iadc_volts = iadc_rms_mv / 1000.0f;
    float i_real = iadc_volts * ki;
    if (i_real < noise_threshold) {
      return 0.0f;
    }
    return i_real;
  }

  /**
   * Comprueba si la lectura del sensor satura los límites dinámicos del ADC
   */
  static bool isSaturated(uint16_t min_mv, uint16_t max_mv) {
    return (min_mv <= SCT013_MIN_SAFE_MV) || (max_mv >= SCT013_MAX_SAFE_MV);
  }

  /**
   * Verifica si el bias DC en reposo está dentro del rango admisible (±150 mV
   * de 1650 mV)
   */
  static bool isBiasValid(uint16_t avg_mv) {
    return (avg_mv >= (SCT013_BIAS_NOMINAL_MV - 150)) &&
           (avg_mv <= (SCT013_BIAS_NOMINAL_MV + 150));
  }
};

#endif // SCT013_H
