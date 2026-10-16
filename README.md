# ESP32 Agent — WiFi portal + OpenAI-compatible chat + OTA

ESP32 firmware: captive WiFi setup via browser, agent endpoint/API-key setup,
LAN web server with chat UI that forwards to any OpenAI-compatible
`/chat/completions` endpoint, plus cable-free updates.

## What you get

- Boot with no WiFi → AP **`ESP32-Agent-Setup`** (open) at `http://192.168.4.1/wifi`.
  WiFi on `/wifi` (reboots), agent endpoint on `/agent` (no reboot, never touches WiFi).
  Hub at `/setup`.
- Boot with WiFi → chat at `http://<ESP_IP>/` and `http://esp32-agent.local/`.
- `POST /api/chat {"message":"hi","history":[...]}` → ESP32 calls
  `<endpoint>/chat/completions` with `Authorization: Bearer <key>` and returns `reply`.
- `GET /api/status`, `GET /api/scan`, `GET/POST /api/wifi`, `GET/POST /api/agent`,
  `GET/POST /api/config` (legacy), `POST /api/reset`.
- OTA: ArduinoOTA + browser `GET/POST /update` (upload `firmware.bin`).

## No boot button after the first flash

- First flash uses USB + auto-reset (DTR/RTS via CP210x). Boards **with** the
  auto-reset circuit flash with no button. Boards **without** it (or flaky
  ones — like the one on COM8 right now, which answers `Wrong boot mode 0x13`)
  need **one** manual step: hold `BOOT`, tap `EN`, release `BOOT`, then upload.
- Every flash after that is over WiFi:
  - `pio run --target upload --upload-port <ESP_IP>` (ArduinoOTA), or
  - browser → `http://<ESP_IP>/update` → upload `.pio/build/esp32dev/firmware.bin`.
- So the agent (me) can keep iterating without you touching the board.

## Build / flash

```powershell
pio run                                  # compile check
pio run --target upload                  # USB flash (uses COM8 per platformio.ini)
pio run --target upload --upload-port 192.168.1.50   # OTA flash, no cable/button
pio device monitor --baud 115200         # logs; shows IP + mDNS
```

## Setup (phone or laptop)

1. Power ESP32 → join WiFi `ESP32-Agent-Setup` → open `http://192.168.4.1/wifi`
   (if not, open it manually).
2. `/wifi`: Scan → pick hotspot → password → Save & Reboot. Blank password keeps saved one.
3. `/agent`: endpoint e.g. `https://api.openai.com/v1`, key `sk-…`, model → Save (no reboot, WiFi untouched).
4. Rejoin your normal WiFi → open `http://esp32-agent.local/` or the IP from serial logs.

## "From anywhere" — honest note

The ESP32 serves on your LAN. It is **not** reachable from the internet by default
(CGNAT/firewall). Options:

- Same network: `http://esp32-agent.local/` (mDNS) — works now.
- Outside: port-forward `:80` on your router (risky, add auth first), or run
  Tailscale on a PC/phone + reverse-proxy to the ESP IP, or `cloudflared`/ngrok
  TCP tunnel on a PC pointing at the ESP IP. Ask and I'll walk you through one.

## Files

- `platformio.ini` — esp32dev, Arduino, ArduinoJson, COM8 + OTA notes.
- `src/main.cpp` — slim entry: `setup()`/`loop()` only (~50 lines).
- `src/config.h/.cpp` — prefs, `cfg*`, tools mask, URL/key helpers.
- `src/agent_tools.h/.cpp` — GPIO/ADC/wifi-scan/memory/log + `blink` + `web_fetch` (10 tools).
- `src/web_ui.h` — PROGMEM HTML only (Chat/Setup/WiFi/Agent/Update/Functions/Docs). Agents: skip unless editing UI.
- `src/llm.h/.cpp` — OpenAI-compatible bridge + agentic tool loop.
- `src/net.h/.cpp` — `server`/`dnsServer`, `connectWiFi()`, `startAP()`.
- `src/routes.h/.cpp` — all `handle*` + `setupRoutes()` (`/`, `/docs`, `/functions`, `/api/*`, `/update`).
