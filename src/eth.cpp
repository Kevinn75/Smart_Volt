#include "eth.h"

#if !ENABLE_ETHERNET
// ---------------------------------------------------------------------------
// Ethernet desactivado: funciones vacías para que el resto del código no cambie
// ---------------------------------------------------------------------------
void eth_init() {}
bool eth_connected() { return false; }
void eth_request_status() { Serial.println(">> Ethernet desactivado (ENABLE_ETHERNET = 0 en eth.h)"); }
void eth_request_internet_test() { eth_request_status(); }

#else
// ---------------------------------------------------------------------------
// Ethernet ENC28J60 con la librería EthernetENC (API igual a la Ethernet de Arduino)
// ---------------------------------------------------------------------------
#include <SPI.h>
#include <EthernetENC.h>
#include <esp_mac.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "board_pins.h"

static const uint32_t DHCP_TIMEOUT_MS = 10000;   // espera de respuesta del router
static const uint32_t RETRY_MS = 15000;          // reintento si no hay IP
static const char *TEST_HOST = "example.com";    // para la prueba de internet

static volatile bool s_connected = false;
static volatile bool s_req_status = false;
static volatile bool s_req_test = false;
static uint8_t s_mac[6];

bool eth_connected() { return s_connected; }
void eth_request_status() { s_req_status = true; }
void eth_request_internet_test() { s_req_test = true; }

static void print_status() {
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("[ETH] No se encuentra el ENC28J60: revisa SPI, CS, 3,3 V y GND.");
    return;
  }
  const char *link = (Ethernet.linkStatus() == LinkON) ? "cable conectado" : "sin cable";
  if (s_connected) {
    Serial.printf("[ETH] %s | IP %s | puerta %s | DNS %s\n", link,
                  Ethernet.localIP().toString().c_str(),
                  Ethernet.gatewayIP().toString().c_str(),
                  Ethernet.dnsServerIP().toString().c_str());
  } else {
    Serial.printf("[ETH] %s | sin IP todavía\n", link);
  }
  Serial.printf("[ETH] MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
                s_mac[0], s_mac[1], s_mac[2], s_mac[3], s_mac[4], s_mac[5]);
}

// Abre una conexión TCP a un servidor de internet: si funciona, hay salida a internet
static void internet_test() {
  if (!s_connected) {
    Serial.println("[ETH] Prueba de internet: no hay IP por Ethernet.");
    return;
  }
  EthernetClient c;
  uint32_t t0 = millis();
  if (c.connect(TEST_HOST, 80)) {
    c.print("HEAD / HTTP/1.1\r\nHost: ");
    c.print(TEST_HOST);
    c.print("\r\nConnection: close\r\n\r\n");
    uint32_t t1 = millis();
    while (!c.available() && c.connected() && millis() - t1 < 3000) delay(5);
    String line = c.available() ? c.readStringUntil('\n') : String("(sin respuesta)");
    line.trim();
    Serial.printf("[ETH] Internet OK: %s respondió en %lu ms -> %s\n", TEST_HOST,
                  (unsigned long)(millis() - t0), line.c_str());
    c.stop();
  } else {
    Serial.printf("[ETH] Sin internet: no se pudo conectar a %s (revisa router/DNS).\n", TEST_HOST);
  }
}

static bool start_dhcp() {
  Serial.println("[ETH] Pidiendo IP por DHCP...");
  if (Ethernet.begin(s_mac, DHCP_TIMEOUT_MS) == 0) {
    Serial.println("[ETH] El router no dio IP (¿cable conectado? ¿DHCP activo?).");
    return false;
  }
  Serial.printf("[ETH] Conectado -> IP %s\n", Ethernet.localIP().toString().c_str());
  return true;
}

static void eth_task(void *) {
  // Reset por hardware del módulo
  pinMode(PIN_ENC_RESET, OUTPUT);
  digitalWrite(PIN_ENC_RESET, LOW);
  vTaskDelay(pdMS_TO_TICKS(10));
  digitalWrite(PIN_ENC_RESET, HIGH);
  vTaskDelay(pdMS_TO_TICKS(50));

  // Bus SPI en los pines del ENC28J60 (la librería usa el objeto SPI global)
  SPI.begin(PIN_ENC_SCK, PIN_ENC_MISO, PIN_ENC_MOSI, PIN_ENC_CS);
  Ethernet.init(PIN_ENC_CS);

  // MAC única del chip (la reservada por Espressif para Ethernet)
  esp_read_mac(s_mac, ESP_MAC_ETH);

  bool have_ip = start_dhcp();
  s_connected = have_ip;
  if (Ethernet.hardwareStatus() == EthernetNoHardware) print_status();

  uint32_t last_try = millis();
  bool was_link = (Ethernet.linkStatus() == LinkON);

  while (true) {
    // Mantiene la concesión DHCP y procesa paquetes pendientes
    if (have_ip) Ethernet.maintain();

    bool link = (Ethernet.linkStatus() == LinkON);
    if (link != was_link) {
      Serial.println(link ? "[ETH] Cable conectado" : "[ETH] Cable desconectado");
      was_link = link;
      if (!link) have_ip = false;
      last_try = 0;  // reintentar ya
    }

    if (link && !have_ip && millis() - last_try >= RETRY_MS) {
      last_try = millis();
      have_ip = start_dhcp();
    }
    s_connected = link && have_ip;

    if (s_req_status) { s_req_status = false; print_status(); }
    if (s_req_test)   { s_req_test = false;   internet_test(); }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void eth_init() {
  xTaskCreatePinnedToCore(eth_task, "eth_task", 6144, NULL, 1, NULL, 0);  // Core 0
}

#endif // ENABLE_ETHERNET
