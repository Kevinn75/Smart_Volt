#ifndef WEB_H
#define WEB_H

// Wi-Fi + servidor web (tarea en el Core 0, la medición sigue en el Core 1)
//  - Siempre crea la red "SmartVolt" (clave smartvolt123) -> http://192.168.4.1
//  - Si hay una red guardada, también se conecta a ella   -> http://smartvolt.local
void web_init();

// Guarda una red Wi-Fi nueva y se conecta (se puede llamar desde la consola)
void web_set_wifi(const char *ssid, const char *pass);

// Pide imprimir en la consola las direcciones de la página
void web_request_print_ip();

#include <Arduino.h>

// JSON de la última lectura y de las formas de onda (los usa también la nube)
String web_data_json();
String web_wave_json();

// Ejecuta una acción de la página: avg, energy_reset, limit, outputs.
// query = "clave=valor&clave=valor". Devuelve false y llena err si no es válida.
bool web_apply_action(const char *action, const char *query, String &err);

#endif // WEB_H
