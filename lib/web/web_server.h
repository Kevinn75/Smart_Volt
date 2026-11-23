#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include "metering.h"
#include "calibration.h"

/**
 * ============================================================================
 * SERVIDOR WEB Y GESTOR DE CONECTIVIDAD SMART VOLT
 * ============================================================================
 * - Gestiona Wi-Fi en modo Estación (STA) o Punto de Acceso de Rescate (AP).
 * - Entrega la aplicación web responsiva (HTML5, CSS3, JS).
 * - Provee API REST JSON para datos en vivo y configuración de habitaciones/Wi-Fi.
 * - Ejecuta su bucle en Core 0 para no interferir con la medición en Core 1.
 */

class SmartVoltWebServer {
public:
    static void init();
    static void updateMetrics(const MeteringReport &report);
    static bool isConnected();
    static String getIPAddress();
    static String getNetworkStatus();
};

#endif // WEB_SERVER_H
