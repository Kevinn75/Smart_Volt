#ifndef CLOUD_H
#define CLOUD_H

// ============================================================================
// Acceso desde internet por MQTT (tarea en el Core 0)
//
// El medidor SALE a un servidor MQTT en la nube (no hay que abrir puertos en el
// router y funciona aunque el operador use CGNAT). La página publicada en
// GitHub Pages se conecta al mismo servidor y muestra los datos.
//
// Temas (id = código del medidor, p. ej. "sv1a2b3c"):
//   smartvolt/<id>/status   "online" / "offline" (retenido)
//   smartvolt/<id>/data     JSON de la lectura (igual que /api/data)
//   smartvolt/<id>/wave     JSON de las formas de onda (solo si alguien mira)
//   smartvolt/<id>/hist     historial en partes {"pts":[[t,v,i,p],...]}
//   smartvolt/<id>/cmd      órdenes: watch | history | avg?n=5 | limit?... | outputs?... | energy_reset
// ============================================================================

#include <Arduino.h>

// Página remota (GitHub Pages del repositorio, carpeta /docs)
#define CLOUD_PAGE_URL "https://kevinn75.github.io/Smart_Volt/"

struct CloudInfo {
  bool enabled;
  bool connected;
  char host[96];
  uint16_t port;
  char user[64];
  char id[16];
  const char *page;
};

void cloud_init();
void cloud_configure(const char *host, uint16_t port, const char *user, const char *pass);
void cloud_disable();
CloudInfo cloud_get_info();
void cloud_request_status();   // imprime el estado en la consola

#endif // CLOUD_H
