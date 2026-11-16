# ESP32 Jeedom Notes

ESP32 development board project for PlatformIO or Arduino.

## Developer API Reference

This reference describes the current implementation in
[`src/main.cpp`](../src/main.cpp), [`include/eau.h`](../include/eau.h),
[`include/Jeedom.h`](../include/Jeedom.h), and the installed `Esp32_Framework`
dependency. FrameWeb behavior can change when that dependency is updated.

The HTTP server listens on port 80. Examples use `ESP_IP` as a placeholder for
the device IP; mDNS may also allow access using the configured hostname.

### Water and boiler HTTP interface

`GET /eau` returns the rendered supervision page. `GET` and `POST` also accept
query or form parameters. The response is always sent as HTTP 200 with
`Content-Type: text/html`, including for individual values; it is not JSON.

For either method, an argument with an empty value reads the corresponding
value, returning only its text instead of the page. Arguments are processed in
order: the first empty value ends processing immediately, so later arguments
are ignored and any earlier changes bypass the normal save check. Read values
one at a time. Unknown names return an empty response when read; unknown
nonempty parameters are ignored.

| Parameter | Read value | Nonempty value / effect |
|---|---|---|
| `Cmd` | `Fermé` or `Ouverte` | `open`, `close`, or `reset`; reset clears the flow-duration counter and opens the valve. |
| `TOE` | Hall pulse counter | Sets the Hall pulse counter from a number converted through a float. |
| `IPL` | Hall pulses per liter, one decimal | Sets `fluxReference`, the counter calibration. |
| `DFE` | Maximum flow duration in minutes, displayed without decimals | Sets `openDelay`; sustained flow can then close the valve. |
| `MT` | Water total in m³, three decimals | Sets the Hall counter using `(uint64_t) value * 1000`: fractional m³ are discarded before multiplication and `IPL` is not applied. Prefer `TOE` for counter initialization. |
| `FL` | `Allumée` or `Eteinte` | Read-only flame state. |
| `Tst` | `(Hall pulses, IR wheel pulses)` | Read-only diagnostic counters. |
| `DATE`, `REC`, `RB` | Current date/time, last save, last reboot | Read-only. |
| `VL`, `HOS`, `TITLE`, `IP`, `MAC`, `MFREE` | Version, hostname, page title, IP, MAC, total/used SPIFFS KiB | Read-only. |

The implementation does not validate numeric ranges or acknowledge invalid
commands; an HTTP 200 does not prove that an action succeeded. Nonempty
setters return the complete HTML page. Changes to configuration are flagged
for a later save, rather than synchronously confirmed by the HTTP response.
`Cmd`, `TOE` and `MT` trigger runtime actions; the main loop later updates the
stored valve state and water total.

```bash
# Read the water total or valve state
curl 'http://ESP_IP/eau?MT='
curl 'http://ESP_IP/eau?Cmd='

# These requests affect a physical valve; use only on a safe test installation
curl 'http://ESP_IP/eau?Cmd=close'
curl 'http://ESP_IP/eau?Cmd=open'

# Update calibration and flow timeout
curl -X POST 'http://ESP_IP/eau' -d 'IPL=1&DFE=15'
```

### Logs and FrameWeb endpoints

| Endpoint | Behavior |
|---|---|
| `GET /tail` | HTTP 200, `text/plain`: current RAM log buffer, not a complete historical log. |
| `/running.log`, `/previous.log` | SPIFFS log files, served if present. Logs are flushed from RAM and rotated by `HLog`. |
| `/ls` | Plain-text SPIFFS listing. |
| `GET /upload`, `POST /upload` | File upload page and multipart SPIFFS upload. |
| `GET /update`, `POST /update` | Firmware upload page and multipart OTA update; the POST replies `OK` or `FAIL` and restarts the ESP32. |
| `/explorer` | File explorer; `cmd=remove&file=...` or `cmd=download&file=...` selects an operation. |
| `POST /post` | Form field `cmd`: `save-config`, `reset-wifi`, `restart` or `restore`. Redirects to `/` with HTTP 303. `save-config` concerns FrameWeb configuration, not the Jeedom configuration. |
| Other paths | FrameWeb attempts to serve a SPIFFS file, preferring a `.gz` version; otherwise it returns 404 or a built-in page when enabled. |

FrameWeb's built-in index and update pages require HTTP Basic authentication
when served as fallbacks. The upload page, explorer, and nonempty `/post`
commands also check `LoginName` and `LoginPassword` from `/config.json`.
However, in the installed framework, **POST `/upload` and POST `/update` do
not check authentication**; `/ls` and directly served SPIFFS files are also
unauthenticated. The project's `/eau` and `/tail` routes do not authenticate
either, and `/eau` can operate the valve via GET. Configuration files may
therefore be readable over HTTP, including the Jeedom API key. HTTP is not
encrypted. Use a trusted, isolated network; do not expose this service to the
Internet or rely on Basic authentication to protect these routes.

### WebSocket

FrameWeb starts a WebSocket server on port 81. The example
[`Data/websocket.js`](../Data/websocket.js) connects to
`ws://ESP_IP:81/` with the `arduino` subprotocol. The application's
`webSocketEvent(...)` callback is empty: no application messages, commands or
responses are implemented. The example client is not a valve-control API.

### Jeedom outbound API

`Jeedom::setup()` loads `/cfJeedom.json` from SPIFFS and builds the virtual
event URL prefix. `Jeedom::sendVirtual(int id, float val)` sends an HTTP GET
to the configured host and port, formats the value with three decimals, and
returns the HTTP status or HTTPClient error code. `getErrorCounter()` counts
consecutive non-200 results and resets on HTTP 200.

The intended Jeedom event path has this shape:

```text
/core/api/jeeApi.php?apikey=<API_KEY>&type=event&plugin=virtual&id=<ID>&value=<VALUE>
```

**Current implementation caveat:** `sendVirtual` appends the partially built
URL to itself before appending `&value=...`. Consequently, the actual request
path contains the prefix and ID twice, not the intended path above. Check
[`Jeedom.h`](../include/Jeedom.h) when debugging Jeedom updates; this
documentation does not change that behavior. The API key is in the URL, so
avoid recording or sharing request URLs.

The IDs are hard-coded in [`main.cpp`](../src/main.cpp):

| ID | Reported value |
|---|---|
| 1786 | Flame state |
| 1816 | Flame duty percentage |
| 1811 | Flow in L/min |
| 1812 | Water total in m³ |
| 1823 | Inverse of `flux.getState()` |
| 1825 | Valve open state |

IDs 1784 and 1785 are declared but unused. Updates occur on changes and in
the daily/reporting section. A send requires connected Wi-Fi and a previous
HTTP 200 in the current reporting cycle; a failure suppresses subsequent sends
in that cycle.

The JSON configuration fields are `host`, `port`, `apiKey`, `fluxReference`
(pulses/liter), `openDelay` (minutes), `waterM3`, `valveOpen`, and
`daylightOffset` (seconds). Loading uses per-field defaults when a file is
present; a missing file is reported on serial output. The URL prefix is
built during `setup()`, so changing the API key requires restarting to rebuild
it. Saving opens the file for writing before checking `waterM3`; a zero total
returns failure and can leave the file truncated. Back up configuration before
testing persistence.

### C++ integration APIs

| Component | Main methods and contract |
|---|---|
| `Jeedom` | `setup()`, `loadConfigurationJeedom()`, `saveConfigurationJeedom()`, `isCcrChanged()`, `sendVirtual(id, value)`, `getErrorCounter()`; `config` holds the active values. |
| `JFlux` | `setup(totalM3, pulsesPerLiter)` restores the Hall count; `irq(pin, level)` processes queued edges; call `loop()` regularly and `isChanged(time, pulsesPerLiter)` periodically. Getters expose flow state, L/min, Hall and IR counts; `setMagnetHallPluse()` sets the Hall count. |
| `JFlame` | `isChanged(time, limit)` samples and filters the analog sensor; read `state`, `value` and `flamePerCent`. The application calls it approximately once per second with limit 1000. |
| `JKeyLedBuz` | `initValve(closed)` initializes the remembered state; `setValve(closed)` applies it and returns whether it changed. `getKey(repeatMs)` returns 0, 1, 2 or 3 for none, up, down or both, and services LED/buzzer. |
| `HLog` | `append(...)` buffers a line, `getTail()` returns the RAM buffer, and `flush()` writes it to SPIFFS and returns success. |
| `FrameWeb` | `setup()` initializes networking and services; `loop()` services them. Add application routes through `frame.server.on(...)`; implement the callbacks declared by FrameWeb. |

For the page, edit [`src/eau.html`](../src/eau.html), then rebuild to regenerate
the marked HTML block in `include/eau.h`. The getters, setters and `dico`
mapping outside that block implement the HTTP parameter interface.

## Tested Features

- Network protocols
- File upload
- Wi-Fi connectivity
- mDNS
- Web server
- SPIFFS
- OTA update
- WebSocket

## Dependencies and Adapted Libraries

Some libraries were adapted for ESP32.

Reference source:
- https://github.com/tzapu/WiFiManager

Private or project-scoped libraries used:

### WiFiManager Stack

- DNSServer---esp32-master
- simplenetworkscanner-master
- WebServer-esp32-master
- WIFIMANAGER-ESP32-master

### WebSocket

- arduinoWebSockets-master