# Project Structure

Purpose: what lives where, and which files to touch for common changes.

## Layout

```
esp32-agent/
├── platformio.ini        # Boards, upload ports, libs, partitions
├── src/
│   ├── main.cpp          # setup()/loop() only (~50 lines)
│   ├── config.h/.cpp     # NVS settings + URL/key helpers
│   ├── net.h/.cpp        # WiFi STA/AP + shared server objects
│   ├── agent_tools.h/.cpp# shared tool glue (registry dispatch, memory/log, pins, prompt)
│   ├── tools/            # one file per device tool (tool_*.cpp + registry.cpp + _template.cpp.sample)
│   ├── llm.h/.cpp        # OpenAI-compatible bridge + tool loop
│   ├── routes.h/.cpp     # All pages + JSON APIs + SSE + OTA
│   └── web_ui.h          # Seven PROGMEM HTML pages (no logic)
├── docs/                 # Developer documentation (this folder)
├── LICENSE               # MIT
└── .gitignore             # .pio/, build artifacts, .env
```

Generated / ignored: `.pio/` (PlatformIO build + `firmware.bin`), `.playwright-mcp/`, `*.bin` / `*.elf` (see `.gitignore`). There is no `lib/`, `test/`, `data/`, Dockerfile, or CI config in this repo.

## File guide

- **`platformio.ini`** — two environments: `esp32dev` (USB default, `upload_port = COM8`, `upload_speed = 460800`) and `esp32dev-ota` (extends base, `espota` to `192.168.5.44:3232`). ArduinoJson pinned at `^7.4.2`, `monitor_speed = 115200`, `default.csv` partitions, `littlefs` filesystem. Edit this to change your COM port, OTA IP, or board variant.
- **`src/main.cpp:1-51`** — boot: `Serial.begin(115200)` → `loadConfig()` → `initAgentFS()` → `connectWiFi(15000)` or `startAP()` → mDNS (`esp32-agent.local`) → ArduinoOTA → `setupRoutes()` → `server.begin()`. Touch only for boot-order or OTA-hostname changes.
- **`src/config.h:1-29` / `src/config.cpp:1-85`** — constants `AP_SSID` (`ESP32-Agent-Setup`), `AP_PASS` (empty = open), `MDNS_NAME` (`esp32-agent`), `PREFS_NS` (`esp32agent`); globals `cfgSsid/cfgPass/cfgEndpoint/cfgApiKey/cfgModel`, `gToolsMask` (default `0x3FF` = 10 tools on). Helpers `chatCompletionsUrl()`, `maskedKey()`, `saveWifiOnly()`, `saveAgentOnly()`. This is the place for new persisted settings.
- **`src/net.h:1-14` / `src/net.cpp:1-38`** — shared `WebServer server(80)`, `DNSServer dnsServer`, flags `apMode` / `dnsRunning`, `DNS_PORT 53`. `connectWiFi()` blocks up to the given timeout; `startAP()` brings up `WIFI_AP_STA` + wildcard DNS. Touch for WiFi policy (e.g. static IP, hostname).
- **`src/agent_tools.h/.cpp`** — shared tool glue: `TOOL_REGISTRY` dispatch (`toolIndex/enabled`, `buildSystemPrompt()`, `addToolsArray()`, `execTool()`), LittleFS paths (`/chatlog.jsonl` 32 KB, `/memory.txt` 4 KB), pin allowlists (`pinCanWrite/Read/ADC`). Add a new device capability in `src/tools/` (and document it in `docs/agent-tools.md`).
- **`src/llm.h:1-9` / `src/llm.cpp:1-366`** — `postLLM()` (single upstream POST), `addHistoryCapped()`, `callLLMWithTools()` (agentic loop), `callLLM()` (legacy plain path). Touch for model parameters (`temperature`, `max_tokens`), timeouts, or new provider quirks.
- **`src/routes.h:1-4` / `src/routes.cpp`** — page handlers (`/` → chat or redirect, `/setup`, `/wifi`, `/agent`, `/update`, `/functions`, `/docs`), JSON APIs (`/api/status`, `/api/scan`, `/api/wifi`, `/api/agent`, `/api/models`, `/api/config`, `/api/chat`, `/api/chat-stream`, `/api/debug-raw`, `/api/tools`, `/api/memory`, `/api/reset`), browser OTA (`POST /update` via `Update.h`), captive-portal `onNotFound`. Add a new endpoint here and register it in `setupRoutes()`.
- **`src/web_ui.h:1-631`** — large `R"rawliteral(...)"` blobs: `CHAT_HTML`, `SETUP_HTML`, `WIFI_HTML`, `AGENT_HTML`, `UPDATE_HTML`, `FUNCS_HTML`, `DOCS_HTML`. Served with `server.send_P()` (flash-resident, no RAM copy). Skip unless editing UI — it is intentionally the biggest file and contains no C++ logic.

## Where to make common changes

| Goal | Edit |
|------|------|
| Change AP name, mDNS name, defaults | `src/config.cpp:3-16` |
| Add a device tool | Copy `src/tools/_template.cpp.sample` → `src/tools/tool_x.cpp`, append one line in `src/tools/registry.cpp` |
| Change system prompt / model params | `buildSystemPrompt()` (`src/agent_tools.cpp:149-173`), `callLLMWithTools()` request block (`src/llm.cpp:57-71`) |
| Add a page or API route | Handler + `server.on(...)` in `src/routes.cpp:629-689`, HTML in `src/web_ui.h` if needed |
| Change upload port / OTA target | `platformio.ini:15-16` (USB) and `platformio.ini:30-35` (OTA env) |
| Change flash layout / filesystem | `platformio.ini:26-28` (`board_build.partitions`, `board_build.filesystem`) |
