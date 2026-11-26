#ifndef ETH_H
#define ETH_H

// ============================================================================
// Ethernet por módulo ENC28J60 (SPI dedicado, pines en board_pins.h)
//
// Para activarlo: poner ENABLE_ETHERNET en 1 (aquí o con -D ENABLE_ETHERNET=1 en
// platformio.ini). Con 0 no se compila nada y el medidor funciona solo por Wi-Fi.
//
// Qué hace: arranca el módulo, pide IP por DHCP al router, vigila el cable y
// reconecta solo. Corre en su propia tarea (Core 0), sin tocar la medición.
// La página web sigue saliendo por Wi-Fi; este módulo da salida a internet
// (EthernetClient) para enviar datos a un servidor más adelante.
// ============================================================================
#ifndef ENABLE_ETHERNET
#define ENABLE_ETHERNET 0
#endif

#include <Arduino.h>

void eth_init();                 // arranca la tarea de Ethernet (no hace nada si está desactivado)
bool eth_connected();            // cable conectado y con IP
void eth_request_status();       // imprime estado e IP en la consola
void eth_request_internet_test();// prueba de conexión a internet (DNS + TCP)

#endif // ETH_H
