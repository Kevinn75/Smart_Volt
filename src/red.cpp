#include "red.h"
#include "config.h"
#include <ETH.h>
#include <WiFi.h>  // Se agrega para el manejo de eventos en el Core 2.x de ESP32

static bool estadoConexion = false;

static void manejarEventosWiFi(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_ETH_START:
            ETH.setHostname("esp32-medidor-energia");
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            Serial.println("[ETH] Enlace físico establecido (Link UP)");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            Serial.print("[ETH] Dirección IP asignada: ");
            Serial.println(ETH.localIP());
            estadoConexion = true;
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            Serial.println("[ETH] Enlace físico desconectado");
            estadoConexion = false;
            break;
        default:
            break;
    }
}

void inicializarEthernet() {
    // Se utiliza WiFi.onEvent para registrar callbacks del sistema de red
    WiFi.onEvent(manejarEventosWiFi);
    
    // Inicialización del PHY LAN8720 con el bus RMII
    ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
}

bool estaLanConectada() {
    return estadoConexion;
}