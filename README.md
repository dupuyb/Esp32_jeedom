# ESP32 Jeedom Water & Boiler Monitor

ESP32_Jeedom is an ESP32 application built with PlatformIO (Arduino framework) for home monitoring and automation.

It combines:
- water flow supervision (hall + IR flow sensors)
- boiler flame detection
- motorized valve control
- local web interface and configuration
- Jeedom virtual device reporting over HTTP
- SPIFFS-backed persistent configuration and logs
- OLED local status display

## Main Features

- Wi-Fi connectivity with captive portal fallback (WiFiManager)
- Embedded HTTP server and WebSocket endpoint
- mDNS discovery on local network
- OTA firmware update
- SPIFFS file hosting and upload
- Real-time valve safety logic based on flow duration
- Periodic and event-based Jeedom updates

## Project Structure

- include/Jeedom.h: Jeedom API client and local configuration model
- include/JFlux.h: flow sensor pulse processing and liters/min estimation
- include/JFlame.h: flame sensor filtering and duty-cycle estimation
- include/JKeyLedBuz.h: valve relay, RGB LED, buttons, buzzer handling
- include/HLog.h: in-memory log buffer with SPIFFS rotation
- include/eau.h: generated web page payload + runtime placeholder mapping
- src/main.cpp: system orchestration (network, sensors, valve logic, Jeedom)
- src/eau.html: source template for generated HTTP_EAU page
- Data/: SPIFFS web assets and JSON configuration files

## Build And Upload

Build:

```bash
platformio run --environment esp32dev
```

Upload:

```bash
platformio run --environment esp32dev --target upload
```

Serial monitor:

```bash
platformio device monitor --environment esp32dev
```

## HTML Generation Flow

This project uses the FrameWeb generator script referenced in platformio.ini:

- extra_scripts = pre:.pio/libdeps/esp32dev/Esp32_Framework/extra_script.py
- custom_in_html maps source HTML inputs
- custom_out_h maps generated C++ header/source targets

Current mapping:
1. .pio/libdeps/esp32dev/Esp32_Framework/src/FrameWeb.html -> .pio/libdeps/esp32dev/Esp32_Framework/src/FrameWeb.cpp
2. src/eau.html -> include/eau.h

Important:
- Do not manually edit content between generated markers in generated files.
- Update src/eau.html, then rebuild to regenerate include/eau.h.

## Runtime Endpoints

- /eau: water/boiler supervision page (GET/POST)
- /tail: in-memory latest logs
- /running.log and /previous.log: persisted logs in SPIFFS
- FrameWeb default endpoints are also available (/upload, /update, /explorer, ...)

## Dependencies

Key dependencies declared in platformio.ini:
- WiFiManager
- arduinoWebSockets
- ArduinoJson
- U8g2
- Esp32_Framework

## Notes

- The valve can be auto-closed when continuous flow exceeds the configured delay.
- Configuration is persisted in /cfJeedom.json.
- Generated files are intentionally overwritten at each pre-build generation.
