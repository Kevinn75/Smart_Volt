#include "web_server.h"
#include <WiFi.h>
#include <WebServer.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

static const char *TAG = "WEB_SERVER";

static WebServer s_http_server(80);
static MeteringReport s_current_metrics;
static SemaphoreHandle_t s_metrics_mutex = NULL;
static bool s_is_ap_mode = false;
static String s_network_status = "Iniciando...";

// Tarea del servidor web en Core 0
static void web_task(void *pvParameters);

// ----------------------------------------------------------------------------
// INTERFAZ WEB EMBEBIDA EN FLASH (PROGMEM)
// ----------------------------------------------------------------------------
static const char HTML_INDEX[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Volt - Monitor de Energía</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <div class="container">
        <header>
            <div class="logo">
                <span class="icon">⚡</span>
                <div>
                    <h1>Smart Volt</h1>
                    <span class="user-subtitle" id="userNameLabel">Mi Hogar</span>
                </div>
            </div>
            <div class="header-right">
                <span class="status-badge" id="netStatus">Conectando...</span>
                <button class="btn-settings" onclick="openConfigModal()">⚙️ Configuración</button>
            </div>
        </header>

        <!-- Tarjeta Principal (Hero) -->
        <section class="card hero-card">
            <div class="hero-header">
                <h2>MÓDULO PRINCIPAL</h2>
                <span class="badge-tag">Red AC 120V / 60 Hz</span>
            </div>
            <div class="hero-body">
                <div class="voltage-display">
                    <span class="label">Tensión de Red</span>
                    <div class="val-unit">
                        <span class="big-val" id="valV">--.-</span>
                        <span class="unit">V</span>
                    </div>
                </div>
                <div class="hero-summary">
                    <div class="sum-item">
                        <span class="label">Potencia Total:</span>
                        <strong id="valTotP">-- W</strong>
                    </div>
                    <div class="sum-item">
                        <span class="label">Corriente Total:</span>
                        <strong id="valTotI">-- A</strong>
                    </div>
                    <div class="sum-item">
                        <span class="label">Energía Total:</span>
                        <strong id="valTotE">-- kWh</strong>
                    </div>
                </div>
            </div>
        </section>

        <!-- Habitaciones / Circuitos Conectados -->
        <section>
            <div class="section-title">
                <h2>Circuitos y Habitaciones Monitoreadas</h2>
            </div>
            <div class="channels-grid" id="channelsGrid">
                <!-- Se genera dinámicamente según las habitaciones configuradas -->
            </div>
        </section>

        <footer>
            <span>Smart Volt © 2026 | ESP32-S3 DMA Continuous ADC (16 kS/s) | IP: <span id="ipLabel">--</span></span>
        </footer>
    </div>

    <!-- Modal de Configuración -->
    <div class="modal-overlay" id="configModal">
        <div class="modal-box">
            <div class="modal-header">
                <h3>⚙️ Configuración del Sistema</h3>
                <button class="btn-close" onclick="closeConfigModal()">✕</button>
            </div>
            <form id="configForm" onsubmit="saveConfiguration(event)">
                <div class="form-group">
                    <label>Nombre de Usuario / Instalación:</label>
                    <input type="text" id="cfgUser" maxlength="30" required placeholder="Ej: Casa Principal">
                </div>

                <div class="form-group">
                    <label>Cantidad de Habitaciones / Circuitos:</label>
                    <select id="cfgRooms" onchange="updateRoomInputs()">
                        <option value="1">1 Circuito (Solo Principal)</option>
                        <option value="2">2 Circuitos (Principal + 1 Habitación)</option>
                        <option value="3" selected>3 Circuitos (Principal + 2 Habitaciones)</option>
                    </select>
                </div>

                <div class="form-group" id="groupRoom1">
                    <label>Nombre Canal 1 (Principal):</label>
                    <input type="text" id="cfgR1" maxlength="30" placeholder="Ej: Tablero General">
                </div>

                <div class="form-group" id="groupRoom2">
                    <label>Nombre Canal 2 (Habitación 1):</label>
                    <input type="text" id="cfgR2" maxlength="30" placeholder="Ej: Habitación Principal">
                </div>

                <div class="form-group" id="groupRoom3">
                    <label>Nombre Canal 3 (Habitación 2 / Cocina):</label>
                    <input type="text" id="cfgR3" maxlength="30" placeholder="Ej: Cocina">
                </div>

                <hr class="divider">

                <h4>Conectividad Wi-Fi a Internet</h4>
                <div class="form-group">
                    <label>Nombre de Red Wi-Fi (SSID):</label>
                    <input type="text" id="cfgSSID" maxlength="30" placeholder="Nombre de tu Wi-Fi">
                </div>

                <div class="form-group">
                    <label>Contraseña Wi-Fi:</label>
                    <input type="password" id="cfgPass" maxlength="60" placeholder="Dejar en blanco si no cambia">
                </div>

                <div class="modal-footer">
                    <button type="button" class="btn-cancel" onclick="closeConfigModal()">Cancelar</button>
                    <button type="submit" class="btn-save">Guardar Cambios</button>
                </div>
            </form>
        </div>
    </div>

    <script src="/app.js"></script>
</body>
</html>
)rawliteral";

static const char CSS_STYLES[] PROGMEM = R"rawliteral(
:root {
    --bg-color: #0b1120;
    --card-bg: #1e293b;
    --card-border: #334155;
    --accent: #38bdf8;
    --text-primary: #f8fafc;
    --text-secondary: #94a3b8;
    --green: #22c55e;
    --yellow: #f59e0b;
}

* { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
body { background: var(--bg-color); color: var(--text-primary); display: flex; justify-content: center; padding: 20px; min-height: 100vh; }
.container { width: 100%; max-width: 960px; display: flex; flex-direction: column; gap: 20px; }

header { display: flex; justify-content: space-between; align-items: center; padding-bottom: 14px; border-bottom: 1px solid var(--card-border); }
.logo { display: flex; align-items: center; gap: 12px; }
.logo .icon { font-size: 32px; }
.logo h1 { font-size: 22px; color: var(--accent); }
.user-subtitle { font-size: 13px; color: var(--text-secondary); }

.header-right { display: flex; align-items: center; gap: 10px; }
.status-badge { background: rgba(34, 197, 94, 0.15); color: var(--green); border: 1px solid var(--green); padding: 5px 12px; border-radius: 20px; font-size: 12px; font-weight: 600; }
.status-badge.ap { background: rgba(245, 158, 11, 0.15); color: var(--yellow); border-color: var(--yellow); }

.btn-settings { background: #334155; color: var(--text-primary); border: 1px solid #475569; padding: 7px 14px; border-radius: 8px; cursor: pointer; font-size: 13px; font-weight: 600; }
.btn-settings:hover { background: #475569; }

.card { background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 12px; padding: 20px; box-shadow: 0 4px 16px rgba(0, 0, 0, 0.3); }
.hero-card { border-top: 4px solid var(--accent); }
.hero-header { display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid var(--card-border); padding-bottom: 10px; margin-bottom: 16px; }
.hero-header h2 { font-size: 14px; color: var(--text-secondary); letter-spacing: 1px; }
.badge-tag { font-size: 12px; background: rgba(56, 189, 248, 0.15); color: var(--accent); padding: 3px 8px; border-radius: 6px; }

.hero-body { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; align-items: center; }
@media(max-width: 600px) { .hero-body { grid-template-columns: 1fr; text-align: center; } }

.voltage-display .label { font-size: 13px; color: var(--text-secondary); }
.val-unit { display: flex; align-items: baseline; gap: 6px; margin-top: 4px; }
.big-val { font-size: 56px; font-weight: 700; color: #fff; }
.val-unit .unit { font-size: 24px; color: var(--accent); font-weight: 600; }

.hero-summary { display: flex; flex-direction: column; gap: 10px; background: rgba(0, 0, 0, 0.25); padding: 14px; border-radius: 8px; }
.sum-item { display: flex; justify-content: space-between; font-size: 14px; }
.sum-item strong { color: var(--accent); }

.section-title { margin-top: 8px; margin-bottom: 12px; }
.section-title h2 { font-size: 16px; color: var(--text-secondary); font-weight: 600; }

.channels-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 16px; }
.channel-card { display: flex; flex-direction: column; gap: 12px; border-top: 3px solid #64748b; }
.channel-card.main-room { border-top-color: var(--accent); }

.ch-header { display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid var(--card-border); padding-bottom: 6px; }
.ch-header h3 { font-size: 16px; color: var(--text-primary); }
.ch-badge { font-size: 11px; color: var(--text-secondary); background: rgba(255, 255, 255, 0.05); padding: 2px 6px; border-radius: 4px; }

.metric-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
.metric { display: flex; flex-direction: column; gap: 2px; }
.metric .lbl { font-size: 11px; color: var(--text-secondary); }
.metric .val { font-size: 18px; font-weight: 600; }
.metric .val.accent { color: var(--accent); }

.energy-footer { margin-top: 4px; padding: 10px; background: rgba(0, 0, 0, 0.25); border-radius: 6px; display: flex; justify-content: space-between; align-items: center; font-size: 13px; }
.energy-footer strong { color: var(--green); }

footer { text-align: center; color: var(--text-secondary); font-size: 12px; padding: 16px 0; border-top: 1px solid var(--card-border); }

/* Modal Styles */
.modal-overlay { position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: rgba(0, 0, 0, 0.75); display: none; justify-content: center; align-items: center; z-index: 100; backdrop-filter: blur(4px); }
.modal-box { background: var(--card-bg); border: 1px solid var(--card-border); width: 90%; max-width: 480px; border-radius: 12px; padding: 22px; max-height: 90vh; overflow-y: auto; }
.modal-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 16px; }
.modal-header h3 { font-size: 18px; color: var(--accent); }
.btn-close { background: transparent; border: none; color: var(--text-secondary); font-size: 20px; cursor: pointer; }

.form-group { display: flex; flex-direction: column; gap: 6px; margin-bottom: 14px; }
.form-group label { font-size: 13px; color: var(--text-secondary); }
.form-group input, .form-group select { background: #0f172a; border: 1px solid var(--card-border); border-radius: 6px; padding: 10px; color: #fff; font-size: 14px; outline: none; }
.form-group input:focus, .form-group select:focus { border-color: var(--accent); }

.divider { border: 0; border-top: 1px solid var(--card-border); margin: 16px 0; }
h4 { color: var(--accent); font-size: 14px; margin-bottom: 10px; }

.modal-footer { display: flex; justify-content: flex-end; gap: 10px; margin-top: 16px; }
.btn-cancel { background: #334155; color: #fff; border: none; padding: 9px 16px; border-radius: 6px; cursor: pointer; }
.btn-save { background: #0284c7; color: #fff; border: none; padding: 9px 18px; border-radius: 6px; cursor: pointer; font-weight: 600; }
.btn-save:hover { background: #0369a1; }
)rawliteral";

static const char JS_SCRIPT[] PROGMEM = R"rawliteral(
let g_config = null;

async function fetchMetrics() {
    try {
        const res = await fetch('/api/data');
        if (!res.ok) throw new Error();
        const d = await res.json();

        document.getElementById('valV').textContent = d.v.toFixed(1);
        document.getElementById('userNameLabel').textContent = d.user;
        document.getElementById('valTotP').textContent = Math.round(d.tot_p) + ' W';
        document.getElementById('valTotI').textContent = d.tot_i.toFixed(2) + ' A';
        document.getElementById('valTotE').textContent = d.tot_e.toFixed(3) + ' kWh';
        document.getElementById('ipLabel').textContent = d.ip;

        const badge = document.getElementById('netStatus');
        badge.textContent = d.status;
        if (d.status.includes('Punto de Acceso')) {
            badge.classList.add('ap');
        } else {
            badge.classList.remove('ap');
        }

        renderChannels(d.channels);
    } catch (e) {
        document.getElementById('netStatus').textContent = 'Desconectado';
    }
}

function renderChannels(channels) {
    const grid = document.getElementById('channelsGrid');
    grid.innerHTML = '';

    channels.forEach((ch, idx) => {
        const isMain = (idx === 0);
        const card = document.createElement('div');
        card.className = 'card channel-card' + (isMain ? ' main-room' : '');
        card.innerHTML = `
            <div class="ch-header">
                <h3>${ch.name}</h3>
                <span class="ch-badge">${isMain ? 'Circuito Principal' : 'Habitación ' + idx}</span>
            </div>
            <div class="metric-grid">
                <div class="metric">
                    <span class="lbl">Corriente</span>
                    <span class="val">${ch.i.toFixed(2)} A</span>
                </div>
                <div class="metric">
                    <span class="lbl">Pot. Activa</span>
                    <span class="val accent">${Math.round(ch.p)} W</span>
                </div>
                <div class="metric">
                    <span class="lbl">Pot. Aparente</span>
                    <span class="val">${Math.round(ch.s)} VA</span>
                </div>
                <div class="metric">
                    <span class="lbl">F.P.</span>
                    <span class="val">${ch.fp.toFixed(2)}</span>
                </div>
            </div>
            <div class="energy-footer">
                <span>Energía Acumulada:</span>
                <strong>${ch.e.toFixed(3)} kWh</strong>
            </div>
        `;
        grid.appendChild(card);
    });
}

async function openConfigModal() {
    try {
        const res = await fetch('/api/config');
        if (res.ok) {
            g_config = await res.json();
            document.getElementById('cfgUser').value = g_config.user || '';
            document.getElementById('cfgRooms').value = g_config.rooms || 3;
            document.getElementById('cfgR1').value = g_config.r0 || '';
            document.getElementById('cfgR2').value = g_config.r1 || '';
            document.getElementById('cfgR3').value = g_config.r2 || '';
            document.getElementById('cfgSSID').value = g_config.ssid || '';
            document.getElementById('cfgPass').value = '';
            updateRoomInputs();
        }
    } catch (e) {}
    document.getElementById('configModal').style.display = 'flex';
}

function closeConfigModal() {
    document.getElementById('configModal').style.display = 'none';
}

function updateRoomInputs() {
    const rooms = parseInt(document.getElementById('cfgRooms').value);
    document.getElementById('groupRoom2').style.display = (rooms >= 2) ? 'flex' : 'none';
    document.getElementById('groupRoom3').style.display = (rooms >= 3) ? 'flex' : 'none';
}

async function saveConfiguration(e) {
    e.preventDefault();
    const data = {
        user: document.getElementById('cfgUser').value,
        rooms: parseInt(document.getElementById('cfgRooms').value),
        r0: document.getElementById('cfgR1').value,
        r1: document.getElementById('cfgR2').value,
        r2: document.getElementById('cfgR3').value,
        ssid: document.getElementById('cfgSSID').value,
        pass: document.getElementById('cfgPass').value
    };

    try {
        const res = await fetch('/api/config', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify(data)
        });
        if (res.ok) {
            alert('¡Configuración guardada exitosamente!');
            closeConfigModal();
            fetchMetrics();
        } else {
            alert('Error al guardar configuración.');
        }
    } catch (e) {
        alert('Error de conexión con el medidor.');
    }
}

setInterval(fetchMetrics, 1000);
fetchMetrics();
)rawliteral";

// ----------------------------------------------------------------------------
// HANDLERS HTTP
// ----------------------------------------------------------------------------
static void handle_root() {
    s_http_server.send_P(200, "text/html", HTML_INDEX);
}

static void handle_css() {
    s_http_server.send_P(200, "text/css", CSS_STYLES);
}

static void handle_js() {
    s_http_server.send_P(200, "application/javascript", JS_SCRIPT);
}

static void handle_api_data() {
    MeteringReport rep;
    if (s_metrics_mutex) xSemaphoreTake(s_metrics_mutex, portMAX_DELAY);
    rep = s_current_metrics;
    if (s_metrics_mutex) xSemaphoreGive(s_metrics_mutex);

    SystemConfig sys = system_config_get();

    float tot_p = 0.0f;
    float tot_i = 0.0f;
    double tot_e = 0.0;
    for (int k = 0; k < sys.rooms_count; k++) {
        tot_p += rep.p_active[k];
        tot_i += rep.i_rms[k];
        tot_e += rep.energy_kwh[k];
    }

    String json = "{";
    json += "\"v\":" + String(rep.v_rms, 1) + ",";
    json += "\"user\":\"" + String(sys.user_name) + "\",";
    json += "\"status\":\"" + s_network_status + "\",";
    json += "\"ip\":\"" + SmartVoltWebServer::getIPAddress() + "\",";
    json += "\"rooms\":" + String(sys.rooms_count) + ",";
    json += "\"tot_p\":" + String(tot_p, 1) + ",";
    json += "\"tot_i\":" + String(tot_i, 2) + ",";
    json += "\"tot_e\":" + String(tot_e, 3) + ",";
    json += "\"channels\":[";

    for (int k = 0; k < sys.rooms_count; k++) {
        if (k > 0) json += ",";
        json += "{";
        json += "\"name\":\"" + String(sys.room_names[k]) + "\",";
        json += "\"i\":" + String(rep.i_rms[k], 2) + ",";
        json += "\"p\":" + String(rep.p_active[k], 1) + ",";
        json += "\"s\":" + String(rep.s_apparent[k], 1) + ",";
        json += "\"q\":" + String(rep.q_reactive[k], 1) + ",";
        json += "\"fp\":" + String(rep.power_factor[k], 2) + ",";
        json += "\"e\":" + String(rep.energy_kwh[k], 3);
        json += "}";
    }

    json += "]}";
    s_http_server.send(200, "application/json", json);
}

static void handle_api_config_get() {
    SystemConfig sys = system_config_get();
    String json = "{";
    json += "\"user\":\"" + String(sys.user_name) + "\",";
    json += "\"rooms\":" + String(sys.rooms_count) + ",";
    json += "\"r0\":\"" + String(sys.room_names[0]) + "\",";
    json += "\"r1\":\"" + String(sys.room_names[1]) + "\",";
    json += "\"r2\":\"" + String(sys.room_names[2]) + "\",";
    json += "\"ssid\":\"" + String(sys.wifi_ssid) + "\"";
    json += "}";
    s_http_server.send(200, "application/json", json);
}

static void handle_api_config_post() {
    if (!s_http_server.hasArg("plain")) {
        s_http_server.send(400, "application/json", "{\"error\":\"Missing body\"}");
        return;
    }

    String body = s_http_server.arg("plain");
    SystemConfig sys = system_config_get();

    // Parser JSON simple y eficiente sin requerir dependencias pesadas
    auto extract = [&](const char *key) -> String {
        String k = String("\"") + key + "\":";
        int idx = body.indexOf(k);
        if (idx == -1) return "";
        idx += k.length();
        while (idx < body.length() && (body[idx] == ' ' || body[idx] == '\"')) idx++;
        int end = idx;
        while (end < body.length() && body[end] != '\"' && body[end] != ',' && body[end] != '}') end++;
        return body.substring(idx, end);
    };

    String user = extract("user");
    String rooms = extract("rooms");
    String r0 = extract("r0");
    String r1 = extract("r1");
    String r2 = extract("r2");
    String ssid = extract("ssid");
    String pass = extract("pass");

    if (user.length() > 0) strncpy(sys.user_name, user.c_str(), sizeof(sys.user_name) - 1);
    if (rooms.length() > 0) {
        int r = rooms.toInt();
        if (r >= 1 && r <= 3) sys.rooms_count = r;
    }
    if (r0.length() > 0) strncpy(sys.room_names[0], r0.c_str(), sizeof(sys.room_names[0]) - 1);
    if (r1.length() > 0) strncpy(sys.room_names[1], r1.c_str(), sizeof(sys.room_names[1]) - 1);
    if (r2.length() > 0) strncpy(sys.room_names[2], r2.c_str(), sizeof(sys.room_names[2]) - 1);
    if (ssid.length() > 0) strncpy(sys.wifi_ssid, ssid.c_str(), sizeof(sys.wifi_ssid) - 1);
    if (pass.length() > 0) strncpy(sys.wifi_pass, pass.c_str(), sizeof(sys.wifi_pass) - 1);

    system_config_set(sys);
    system_config_save();

    s_http_server.send(200, "application/json", "{\"status\":\"ok\"}");

    // Si se actualizó el Wi-Fi, intentar reconectar en segundo plano
    if (ssid.length() > 0) {
        ESP_LOGI(TAG, "Nuevas credenciales Wi-Fi recibidas: %s", sys.wifi_ssid);
        WiFi.disconnect();
        WiFi.begin(sys.wifi_ssid, sys.wifi_pass);
    }
}

// ----------------------------------------------------------------------------
// INICIALIZACIÓN Y BUCLE DE TAREA
// ----------------------------------------------------------------------------
void SmartVoltWebServer::init() {
    if (s_metrics_mutex == NULL) {
        s_metrics_mutex = xSemaphoreCreateMutex();
    }

    SystemConfig sys = system_config_get();

    // Configurar Wi-Fi
    WiFi.persistent(false);
    WiFi.mode(WIFI_AP_STA);

    bool connected = false;
    if (strlen(sys.wifi_ssid) > 0) {
        ESP_LOGI(TAG, "Conectando a Wi-Fi: %s ...", sys.wifi_ssid);
        WiFi.begin(sys.wifi_ssid, sys.wifi_pass);

        // Esperar hasta 8 segundos
        int retries = 16;
        while (retries-- > 0 && WiFi.status() != WL_CONNECTED) {
            delay(500);
        }

        if (WiFi.status() == WL_CONNECTED) {
            connected = true;
            s_is_ap_mode = false;
            s_network_status = "Wi-Fi: " + String(sys.wifi_ssid);
            ESP_LOGI(TAG, "Wi-Fi conectado! IP: %s", WiFi.localIP().toString().c_str());
        } else {
            ESP_LOGW(TAG, "No se pudo conectar a la red Wi-Fi");
        }
    }

    if (!connected) {
        // Iniciar Punto de Acceso de Rescate / Configuración
        WiFi.softAP("SmartVolt_AP", "12345678");
        s_is_ap_mode = true;
        s_network_status = "Punto de Acceso (AP)";
        ESP_LOGI(TAG, "Punto de Acceso activo. SSID: SmartVolt_AP, IP: %s", WiFi.softAPIP().toString().c_str());
    }

    // Configurar rutas del servidor HTTP
    s_http_server.on("/", HTTP_GET, handle_root);
    s_http_server.on("/style.css", HTTP_GET, handle_css);
    s_http_server.on("/app.js", HTTP_GET, handle_js);
    s_http_server.on("/api/data", HTTP_GET, handle_api_data);
    s_http_server.on("/api/config", HTTP_GET, handle_api_config_get);
    s_http_server.on("/api/config", HTTP_POST, handle_api_config_post);

    s_http_server.begin();
    ESP_LOGI(TAG, "Servidor HTTP iniciado en puerto 80");

    // Iniciar tarea del servidor HTTP en Core 0
    xTaskCreatePinnedToCore(
        web_task,
        "web_task",
        4096,
        NULL,
        1,
        NULL,
        0 // Core 0
    );
}

void SmartVoltWebServer::updateMetrics(const MeteringReport &report) {
    if (s_metrics_mutex) {
        xSemaphoreTake(s_metrics_mutex, portMAX_DELAY);
        s_current_metrics = report;
        xSemaphoreGive(s_metrics_mutex);
    }
}

bool SmartVoltWebServer::isConnected() {
    return WiFi.status() == WL_CONNECTED || s_is_ap_mode;
}

String SmartVoltWebServer::getIPAddress() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return WiFi.softAPIP().toString();
}

String SmartVoltWebServer::getNetworkStatus() {
    return s_network_status;
}

static void web_task(void *pvParameters) {
    (void)pvParameters;
    while (1) {
        s_http_server.handleClient();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
