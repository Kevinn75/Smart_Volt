#ifndef METER_H
#define METER_H

#include <Arduino.h>

// Última lectura del medidor (valores ya promediados con la media móvil)
struct MeterSnapshot {
  uint32_t t;        // segundos desde el arranque en que se tomó la lectura
  float v;           // tensión RMS (V)
  float i;           // corriente RMS (A)
  float p;           // potencia activa (W)
  float s;           // potencia aparente (VA)
  float fp;          // factor de potencia
  double kwh;        // energía acumulada (kWh)
  float v_bias_mv;   // centro DC del pin de voltaje
  float i_bias_mv;   // centro DC del pin de corriente
  bool v_ok;         // centro de voltaje en rango
  bool v_signal;     // hay señal AC de red
  bool v_clip;       // la onda de voltaje está recortada (pico + y pico − desiguales)
  float v_pk_pos_mv; // pico positivo respecto al centro (mV en el pin)
  float v_pk_neg_mv; // pico negativo respecto al centro (mV en el pin)
  bool i_ok;         // centro de corriente en rango
  uint8_t avg_n;     // segundos de la media móvil
  float i_limit;     // límite de corriente en A (0 = desactivado)
  float v_min;       // límite inferior de voltaje en V (0 = desactivado)
  float v_max;       // límite superior de voltaje en V (0 = desactivado)
  bool buzzer_en;    // activar buzzer automáticamente al superar algún límite
  bool led_en;       // activar LED automáticamente al superar algún límite
  bool buzzer_manual;// activación manual del buzzer desde la web
  bool led_manual;   // activación manual del LED desde la web
  bool buzzer_on;    // estado actual real del buzzer
  bool led_on;       // estado actual real del LED
  bool overcurrent;  // true si la corriente supera el límite
  bool undervolt;    // true si el voltaje está por debajo de v_min
  bool overvolt;     // true si el voltaje supera v_max
  bool valid;        // ya hay al menos una lectura
};

// Un punto del historial para las gráficas
struct HistPoint {
  uint32_t t;
  float v, i, p;
};

#define HIST_LEN 600   // 10 minutos a 1 punto por segundo
#define WAVE_LEN 134   // ~2 ciclos a 60 Hz (4 kHz -> 33.5 ms)

// Lectura segura desde otra tarea (servidor web)
MeterSnapshot meter_get_snapshot();
size_t meter_copy_history(HistPoint *dst, size_t max_points);  // del más antiguo al más reciente
size_t meter_copy_wave(float *v_dst, float *i_dst, size_t max_points);

// Pedidos desde la página web (se aplican en la tarea de medición o de inmediato)
void meter_request_avg(uint8_t n);
void meter_request_energy_reset();
void meter_set_limit(float limit_a, float v_min, float v_max, bool buzzer_en, bool led_en);
void meter_set_manual_outputs(bool buzzer_on, bool led_on);

#endif // METER_H
