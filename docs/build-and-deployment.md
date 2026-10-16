# Build and Deployment

Purpose: how to compile, flash (USB + OTA), and understand the flash layout for this firmware.

## Environments (`platformio.ini`)

| Env | Upload protocol | Target | Use |
|-----|----------------|--------|-----|
| `esp32dev` (default) | `esptool` (USB serial) | `upload_port = COM8`, `upload_speed = 460800` | First flash, recovery |
| `esp32dev-ota` | `espota --port=3232` | `upload_port = 192.168.5.44` | Cable-free updates (update the IP or override with `--upload-port`) |

Shared settings: `platform = espressif32`, `board = esp32dev`, `framework = arduino`, `monitor_speed = 115200` (+ `monitor_rts/dtr = 1`), `lib_deps = bblanchon/ArduinoJson@^7.4.2`, `build_flags = -DCORE_DEBUG_LEVEL=0`, `board_build.partitions = default.csv`, `board_build.filesystem = littlefs`.

## Build

```powershell
pio run                 # build default env → .pio/build/esp32dev/firmware.bin
pio run -e esp32dev-ota # build OTA env (same sources, espota upload config)
pio run --target clean  # wipe build outputs
```

`firmware.bin` is git-ignored (see `.gitignore:29-35`) — releases are produced by rebuilding, not by committing binaries.

## First flash (USB)

```powershell
pio run --target upload                          # uses COM8 from platformio.ini
pio run --target upload --upload-port COM5       # override for your machine
pio device monitor --baud 115200                 # verify: === Agent boot ===, WiFi OK / AP started
```

- Most CP210x boards auto-reset via DTR/RTS — no button needed.
- `Wrong boot mode 0x13` (or a silent board) means the auto-reset circuit is missing/flaky: hold `BOOT`, tap `EN`, release `BOOT`, then upload. One manual step; never needed again once OTA works.

## OTA updates (the normal path)

Prerequisite: device is on your WiFi (see `README.md` setup) and ArduinoOTA is running (started in `src/main.cpp:30-35`, hostname `esp32-agent`, port 3232).

```powershell
# A — PlatformIO (recommended for developers)
pio run --target upload --upload-port 192.168.1.50   # your ESP's LAN IP
```

```text
# B — Browser (for operators without PlatformIO)
# 1. pio run → take .pio/build/esp32dev/firmware.bin
# 2. Open http://<ESP_IP>/update → choose firmware.bin → Upload
# 3. Device flashes via Update.h (routes.cpp:661-679) and reboots
```

OTA progress/failure appears on serial (`OTA start` / `OTA end` / `OTA error %u`). If OTA is unreachable, fall back to USB — then re-check WiFi (`GET /api/status`).

## Flash layout and filesystem

- **Partitions:** `default.csv` (app + SPIFFS/LittleFS region). `board_build.filesystem = littlefs` means any `data/` upload would use LittleFS — this project currently uses LittleFS only at runtime (`LittleFS.begin(true)` in `src/agent_tools.cpp:33-36`), no pre-baked `data/` dir needed.
- **Runtime files:** `/memory.txt` (4 KB cap) and `/chatlog.jsonl` (32 KB cap). Both survive firmware OTA (OTA replaces the app partition, not LittleFS/NVS). `POST /api/reset` is the only operation that wipes NVS + LittleFS together.
- **Web UI:** served from flash (`PROGMEM` + `send_P`, `src/routes.cpp:630-643`) — no filesystem space consumed by HTML.

## Hardening before exposing beyond your LAN

The ESP serves plain HTTP on port 80 with no auth. Before any port-forward or tunnel:

1. Set `ArduinoOTA.setPassword("…")` in `src/main.cpp:31` so OTA can't be hijacked.
2. Consider adding HTTP auth in `setupRoutes()` (`src/routes.cpp:629-689`) — not present today.
3. Prefer a tailnet (e.g. Tailscale on a PC/phone + reverse proxy) or an SSH/cloud tunnel over a raw port-forward. The stock `README.md` troubleshooting section explains the LAN-only default.
