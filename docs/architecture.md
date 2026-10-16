# Architecture

Purpose: technical overview of the ESP32-Agent firmware — stack, runtime model, and how chat, tools, and storage fit together.

## Tech stack

- **MCU / framework:** ESP32 (`esp32dev` board) on Arduino core (`platformio.ini:4-7`).
- **Build system:** PlatformIO. Single dependency: `bblanchon/ArduinoJson@^7.4.2` (`platformio.ini:23-24`). Everything else (WiFi, WebServer, DNSServer, mDNS, ArduinoOTA, LittleFS, HTTPClient, Update, Preferences) comes from the Arduino-ESP32 core.
- **Language:** C++ (Arduino-style `setup()` / `loop()` in `src/main.cpp:13-50`).
- **Persistence:** NVS via `Preferences` (namespace `esp32agent`, `src/config.cpp:6`) for settings + LittleFS (`/memory.txt`, `/chatlog.jsonl`) for agent memory.
- **Networking:** `WebServer` on port 80 + `DNSServer` on port 53 for captive portal (`src/net.cpp:5-9`). mDNS name `esp32-agent` (`src/config.cpp:5`), OTA via ArduinoOTA (`src/main.cpp:30-35`).

## Runtime modes

The firmware boots into exactly one of two modes (`src/main.cpp:20-22`):

1. **STA mode (normal):** `connectWiFi(15000)` succeeds → serves chat UI and APIs on the LAN IP + `http://esp32-agent.local`.
2. **AP setup mode:** no saved SSID or connect times out after 15 s → `startAP()` creates open AP `ESP32-Agent-Setup` at `192.168.4.1` with wildcard DNS (`src/net.cpp:31-38`). Any unknown hostname redirects to `/wifi` (`src/routes.cpp:680-688`). Fresh devices redirect `/` → `/wifi` (`src/routes.cpp:630-637`).

`loop()` is intentionally thin (`src/main.cpp:46-50`): pump DNS (AP mode only), pump HTTP, pump OTA.

## Module map

| File | Responsibility |
|------|----------------|
| `src/main.cpp` | Boot sequence, mDNS, OTA init, `setupRoutes()` + `server.begin()`. No business logic. |
| `src/config.h/.cpp` | NVS settings (`cfgSsid`, `cfgPass`, `cfgEndpoint`, `cfgApiKey`, `cfgModel`, `gToolsMask`), `chatCompletionsUrl()` normalizer, `maskedKey()`. |
| `src/net.h/.cpp` | `WebServer server(80)`, `DNSServer dnsServer`, `connectWiFi()`, `startAP()`, `apMode` / `dnsRunning` flags. |
| `src/agent_tools.h/.cpp` | 10 device tools, LittleFS memory/log, system-prompt builder, OpenAI `tools` array builder. See `docs/agent-tools.md`. |
| `src/llm.h/.cpp` | OpenAI-compatible HTTPS bridge: `postLLM()`, `callLLM()` (plain), `callLLMWithTools()` (3-iteration agentic loop). |
| `src/routes.h/.cpp` | All HTTP pages + JSON APIs + SSE streaming + browser OTA. See `docs/api.md`. |
| `src/web_ui.h` | Seven PROGMEM HTML pages (`CHAT_HTML`, `SETUP_HTML`, `WIFI_HTML`, `AGENT_HTML`, `UPDATE_HTML`, `FUNCS_HTML`, `DOCS_HTML`). No C++ logic. |

## Request / data flow

### Plain chat turn (`POST /api/chat`)

```
Browser → handleChat (routes.cpp:323) → callLLMWithTools (llm.cpp:55)
  → buildSystemPrompt + injectRecentLog (last 8 persisted turns) + history (last 8 turns)
  → addToolsArray (only enabled tools) → POST <endpoint>/chat/completions
  → model returns text OR tool_calls → execTool locally → append tool results → POST again (max 3 iterations)
  → appendChatLog (one LittleFS write per turn) → { reply, reasoning, tools_used }
```

Key budgets (ESP32 RAM is ~220 KB free): user message capped at 1500–2000 chars, history capped at 8–10 turns, `max_tokens: 800`, `temperature: 0.7`, tool results capped at 1200 chars (2200 for `web_fetch`), final reply capped at 3500 chars (`src/llm.cpp:55-235`).

### Streaming chat turn (`POST /api/chat-stream`)

Same context construction, but `stream: true` over a raw socket with manual chunked decoding (`src/routes.cpp:359-617`). Text deltas are forwarded as SSE `{"delta": "..."}` events; reasoning deltas as `{"reasoning": "..."}`. If the stream contains `tool_calls` — or yields zero content chars (reasoning-only models) — the ESP sends `{"need_tools": true}` and the browser falls back to `/api/chat`, which runs the full tool loop. This keeps streaming fast for pure Q&A while never losing tool capability.

### Tool execution

`execTool(name, args)` (`src/agent_tools.cpp:405-489`) is a local switch — no subprocess, no sandbox beyond pin allowlists. `restart_device` only sets `gRestartPending = true`; the actual `ESP.restart()` happens after the HTTP response is sent (`src/routes.cpp:354`).

### Storage flow

- **Settings (NVS):** `wifi_ssid`, `wifi_pass`, `api_endpoint`, `api_key`, `api_model`, `tools_mask`. Written by `saveWifiOnly()` / `saveAgentOnly()` / `saveConfig()` / `saveToolsMask()` (`src/config.cpp:32-85`).
- **Long-term memory:** `/memory.txt`, max 4096 bytes, append-one-line + drop-oldest-lines (`src/agent_tools.cpp:48-68`). Injected into every system prompt (first 1200 chars, `src/agent_tools.cpp:161-164`).
- **Chat log:** `/chatlog.jsonl`, max 32768 bytes, one open/close per turn, rotation keeps last ~16 KB (`src/agent_tools.cpp:72-100`). Last 8 messages re-injected as context (`src/agent_tools.cpp:104-135`).

## External dependencies

- **LLM provider:** any HTTP(S) endpoint exposing `POST {base}/chat/completions` with OpenAI-style `messages` / `tools` / `tool_calls`, plus `GET {base}/models` for the picker (`modelsUrl()`, `src/config.cpp`). The firmware normalizes `https://api.openai.com/v1` → `https://api.openai.com/v1/chat/completions`. TLS uses `setInsecure()` (no cert store on ESP32). Tested shape includes OpenAI, Together, and other OpenAI-compatible gateways; reasoning-field variants (`reasoning_content`, `reasoning`, `reasoning_details[0].text`) are all parsed (`src/llm.cpp`).
- **`web_fetch` target:** arbitrary `http://` / `https://` URL, 15 s timeout, max 8 KB body → 2000 chars of cleaned text (`src/agent_tools.cpp:363-402`).

## Constraints that shape the design

- **HTTP/1.0 for upstream POSTs** (`http.useHTTP10(true)`) so the server returns `Content-Length` instead of chunked encoding, which `HTTPClient::getString()` handles reliably on ESP32 (`src/llm.cpp:289`).
- **No per-token writes.** Flash wear is minimized: exactly one LittleFS append per completed turn (`src/agent_tools.cpp:70-71`).
- **Tool results degrade gracefully.** If a follow-up POST fails after tools ran, the firmware answers from the raw tool results instead of returning an error (`src/llm.cpp:218-224`).
- **OTA + HTTP share `loop()`.** Long tools (`wifi_scan` ~2 s, `blink` up to 15 s) run inside the HTTP handler; a 25 s time budget skips remaining calls rather than wedging the server (`src/llm.cpp:193-196`).
