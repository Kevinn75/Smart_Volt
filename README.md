# Smart Volt

Medidor de energía no invasivo para redes de 120 VAC / 60 Hz con ESP32-S3, Wi-Fi y Ethernet.
Universidad Nacional de Colombia – Sede Manizales · Diseño Electrónico.

Cada segundo mide tensión, corriente, potencia activa y aparente, factor de potencia y energía, y
muestra en una página web cuánto se va a pagar según el estrato.

## Documentos

- [Presentación](documentacion/Smart_Volt_Presentacion.pdf)
- [Nota de aplicación](documentacion/Smart_Volt_Nota_de_Aplicacion.pdf)

## Firmware

| Acción | Comando |
|---|---|
| Compilar y cargar | `pio run -e devkit-uart -t upload` |
| Monitor serie (115 200 baud) | `pio device monitor -e devkit-uart` |

- La página web se edita en [docs/index.html](docs/index.html); al compilar se copia a `src/web_page.h`.
- Acceso local: red **SmartVolt** (clave `smartvolt123`) → http://192.168.4.1, o http://smartvolt.local en la red de la casa.
- Acceso desde internet: comando `nube <servidor> <usuario> <clave>` y la página publicada en GitHub Pages.

Autor: Kevinn Alexander Ordoñez
