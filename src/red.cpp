#include "red.h"
#include "config.h"
#include <ETH.h>  // Librería nativa del core de Arduino para ESP32[cite: 1]

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
    Network.onEvent(manejarEventosWiFi);
    ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);//[cite: 1]
}

bool estaLanConectada() {
    return estadoConexion;
}