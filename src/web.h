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

#endif // WEB_H
