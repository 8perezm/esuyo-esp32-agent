# Development Guide

Purpose: set up a contributor workstation, build, flash, monitor, and debug this firmware.

## Prerequisites

- **Hardware:** ESP32 dev board (`esp32dev` target — ESP32-WROOM style boards with CP210x USB serial work out of the box). USB cable + one GPIO LED or wire for tool testing (onboard LED is usually GPIO 2).
- **Software:** [PlatformIO Core](https://platformio.org/) (CLI) or PlatformIO in VS Code, plus a serial driver for your USB chip (CP210x / CH340 depending on board). No Node, Python venv, or Docker needed — PlatformIO manages toolchains.
- **Network:** 2.4 GHz WiFi credentials for the ESP32, plus an OpenAI-compatible API endpoint + key for agent replies (any provider exposing `POST {base}/chat/completions`).

Verify the toolchain:

```powershell
pio --version
pio run --target clean   # optional sanity check from the repo root
```

## First checkout → build → flash

```powershell
# 1. Compile check (no hardware needed)
pio run

# 2. First flash over USB (auto-reset via DTR/RTS on most boards)
pio run --target upload

# 3. Watch boot logs — note the IP and mDNS line
pio device monitor --baud 115200
```

Notes verified from `platformio.ini`:

- `upload_port = COM8` is pre-set for the original author's machine. If your board enumerates elsewhere, override on the command line instead of editing the file: `pio run --target upload --upload-port COM5`.
- `upload_speed = 460800`, `monitor_speed = 115200` with `monitor_rts = 1` / `monitor_dtr = 1`.
- If the upload fails with `Wrong boot mode`, your board lacks (or has a flaky) auto-reset circuit — hold `BOOT`, tap `EN`, release `BOOT`, then retry the upload. This is only ever needed for the first USB flash; afterwards use OTA.

## OTA workflow (no cable after the first flash)

ArduinoOTA is started unconditionally in `src/main.cpp:30-35` with hostname `esp32-agent`. Two equivalent paths:

```powershell
# Option A — PlatformIO OTA (ArduinoOTA on port 3232)
pio run --target upload --upload-port 192.168.1.50
# replace 192.168.1.50 with the IP shown in `pio device monitor`
# or resolve via mDNS: http://esp32-agent.local
```

```text
# Option B — browser upload (no PlatformIO needed on the operator machine)
# 1. pio run                          → builds .pio/build/esp32dev/firmware.bin
# 2. Open http://<ESP_IP>/update      → choose firmware.bin → Upload
# 3. ESP reboots automatically on success (routes.cpp:661-679)
```

The `esp32dev-ota` env (`platformio.ini:30-35`) presets `upload_protocol = espota`, `upload_port = 192.168.5.44`, `--port=3232`. Either update that IP or pass `--upload-port` explicitly as above.

To lock OTA with a password, uncomment `ArduinoOTA.setPassword(...)` in `src/main.cpp:31` before building.

## Configuration for development

All runtime settings persist in NVS (namespace `esp32agent`) — there is no `.env` file and no compile-time secrets:

| Key (NVS) | Set via | Notes |
|-----------|---------|-------|
| `wifi_ssid` / `wifi_pass` | `POST /api/wifi` or `/wifi` page | Saving WiFi reboots (`src/routes.cpp:162-183`) |
| `api_endpoint` | `POST /api/agent` or `/agent` page | Base URL, e.g. `https://api.openai.com/v1`; `/chat/completions` appended automatically |
| `api_key` | `POST /api/agent` | Blank keeps the saved key (`src/routes.cpp:194-214`) |
| `api_model` | `POST /api/agent` | Default `gpt-4o-mini` |
| `tools_mask` | `POST /api/tools` or `/functions` page | Bitmask over the 10 tools |

For a factory reset during development: `POST /api/reset` clears NVS + LittleFS and reboots (`src/routes.cpp:619-627`).

## Debugging

- **Serial logs** are the primary debugger: `pio device monitor --baud 115200`. Look for `=== Agent boot ===`, `WiFi OK:` / `AP started:`, `mDNS:`, `[agent] iter…`, `[tool] …`, `[llm] POST …`, `[stream] …`.
- **Device state without the UI:** `GET http://<ESP_IP>/api/status` returns WiFi, heap, uptime, endpoint, model, tools mask, and FS usage (`src/routes.cpp:16-40`).
- **Upstream shape problems:** `GET http://<ESP_IP>/api/debug-raw?message=Reply%20with%20just%20OK` returns the raw provider body (truncated) plus parsed `msg_keys` / `content_len` — without exposing the API key (`src/routes.cpp:260-321`).
- **RAM:** watch `free_heap` in `/api/status`. If you add features, keep JSON documents small — `llm.cpp` caps history, message sizes, and tool payloads deliberately (see `docs/architecture.md`).
- **Filesystem:** `GET /api/memory` shows saved facts + last 10 log lines + byte counts (`src/routes.cpp:81-114`).

## Coding conventions

- Keep `main.cpp` slim — put logic in `config` / `net` / `agent_tools` / `llm` / `routes` (the header comment in `src/main.cpp:1-2` says the same).
- `config.h` is the place for prefs / `cfg*` changes (per `src/config.h:2-3`); HTML blobs live only in `web_ui.h` — skip reading it unless editing UI (per `src/web_ui.h:2` and `src/agent_tools.h:2-3`).
- `routes.h` stays tiny; handler bodies live in `routes.cpp` (per `src/routes.h:1-3`).
- Never log or return the raw API key — use `maskedKey()` (`src/config.cpp:64-67`).
- Pin access must go through `pinCanWrite()` / `pinCanRead()` / `pinCanADC()` (`src/agent_tools.cpp:137-146`).
- Persist sparingly: one LittleFS write per chat turn, never per token.
