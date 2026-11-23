# Smart Volt Mobile (Flutter App) 📱⚡

Aplicación móvil dedicada para el medidor eléctrico no invasivo **Smart Volt (ESP32-S3)**.

---

## Características de la Aplicación
1. **Módulo Principal (Dashboard)**:
   - Medición en vivo de la Tensión de Red AC (120 V / 60 Hz).
   - Resumen total de la vivienda: Potencia Activa (W), Potencia Aparente (VA), Corriente (A) y Energía (kWh).
   - Estimación de costos en **Pesos Colombianos (\$ COP)** con tarifa configurable ($/kWh) y proyección de factura mensual.
2. **Desglose por Habitaciones / Circuitos**:
   - Tarjetas individuales para 1, 2 o 3 habitaciones con nombres personalizables (ej. *Tablero General*, *Habitación 1*, *Cocina / Aire Acondicionado*).
   - Barra de porcentaje de carga sobre el disyuntor/breaker (15 A / 40 A).
   - Semáforo de Factor de Potencia (Verde: Excelente/Resistivo, Amarillo: Aceptable, Rojo: Bajo/Inductivo).
3. **Gráficos en Tiempo Real (Live Chart)**:
   - Curva de potencia activa total en vivo con muestreo a 1 Hz (`fl_chart`).
4. **Panel de Ajustes y Conectividad**:
   - Configuración del nombre del usuario/instalación.
   - Selección de circuitos activos (1, 2 o 3).
   - Configuración de la red Wi-Fi (SSID y clave) para conectarse a Internet.
   - Ajuste de IP del ESP32 con cambio rápido entre Punto de Acceso (`192.168.4.1`) o red local Wi-Fi (`192.168.1.xxx`).

---

## Cómo compilar y generar el APK

### Requisitos previos:
1. Instalar [Flutter SDK](https://flutter.dev/docs/get-started/install/windows) (versión >= 3.0.0).
2. Tener configurado Android Studio o Android SDK.

### Comandos de Compilación:
Abre una terminal en esta carpeta (`mobile_app/smart_volt_app`):

```bash
# 1. Descargar paquetes y dependencias
flutter pub get

# 2. Probar la app en tu celular conectado por USB o emulador
flutter run

# 3. Generar el archivo instalable APK para Android
flutter build apk --release
```

El archivo `.apk` final se generará en:
`build/app/outputs/flutter-apk/app-release.apk`
¡Solo lo copias a tu celular Android e instalas!
