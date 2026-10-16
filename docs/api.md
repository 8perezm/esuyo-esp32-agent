# API Reference

Purpose: complete contract for every page and JSON endpoint served by the ESP32, derived from `src/routes.cpp:629-689`.

Base URL is `http://<ESP_IP>` in STA mode or `http://192.168.4.1` in AP setup mode. `http://esp32-agent.local` works wherever mDNS resolves. All JSON APIs use `application/json`; POST bodies are JSON in the request `plain` arg.

## Auth (optional token, `src/routes.cpp`)

Empty token = open (first-boot UX). Set a token via `POST /api/auth` or Setup → Step 3; the browser stores it in `localStorage['esp32_token']` and sends `Authorization: Bearer <token>` (`X-Auth-Token` also accepted). When set (STA mode only; AP setup stays open):

- Every `/api/*` except `GET /api/auth` and `GET /api/status` requires the header, else `401 {"error":"auth required"}`.
- `GET /api/status` without auth returns minimal `401 { ap_mode, wifi_connected, auth_required:true, endpoint_configured }` (no ssid/ip/endpoint/model/heap). With auth it returns the full snapshot plus `auth_required:true`.
- `POST /update` (browser OTA) and ArduinoOTA require the same password.
- Token rules: min 8 chars, max 128 (truncated), blank clears. Still plaintext HTTP on LAN — stops casual abuse, not sniffing.

## Pages (HTML)

| Page | Handler | Notes |
|------|---------|-------|
| `GET /` | `CHAT_HTML` | Chat UI. Fresh device with no saved SSID 302-redirects to `/wifi` (`src/routes.cpp:630-637`). Header warns when WiFi or endpoint is missing; footer shows IP · model · heap. |
| `GET /setup` | `SETUP_HTML` | Hub linking to WiFi / agent / functions / firmware pages. |
| `GET /wifi` | `WIFI_HTML` | SSID scan + password form. Blank password keeps the saved one when SSID is unchanged. |
| `GET /agent` | `AGENT_HTML` | Endpoint + key + model form. Blank endpoint/key keeps saved values. Saves without rebooting. |
| `GET /functions` | `FUNCS_HTML` | Tool enable toggles (posts `enabled[]` to `/api/tools`) + live system-prompt preview. |
| `GET /docs` | `DOCS_HTML` | On-device quick reference. |
| `GET /update` | `UPDATE_HTML` | Browser firmware upload (`firmware.bin`). |
| `GET /api/debug-raw?message=...` | `handleDebugRaw` | Upstream probe with the saved key; returns `{ url, model, http_code, len, raw, choices, msg_keys, content_len, ... }` truncated to ~2.5 KB. Never returns the key. |

Unknown paths 404, except in AP mode where any unknown host redirects to `http://192.168.4.1/wifi` (captive portal, `src/routes.cpp:680-688`).

## `GET /api/status` → device snapshot

No body. Returns (see `src/routes.cpp:16-40`):

```json
{
  "ap_mode": false,
  "wifi_connected": true,
  "ssid": "MyHome",
  "ip": "192.168.1.50",
  "mdns": "http://esp32-agent.local",
  "endpoint_configured": true,
  "endpoint": "https://api.openai.com/v1",
  "model": "gpt-4o-mini",
  "free_heap": 180000,
  "uptime_s": 1234,
  "fs_ok": true,
  "tools_mask": 1023,
  "log_bytes": 1200,
  "mem_bytes": 80
}
```

## WiFi

- `GET /api/wifi` → `{ "wifi_ssid": "..." }`.
- `GET /api/scan` → `{ "count": 12, "networks": [{ "ssid": "...", "rssi": -55, "secure": true }] }` (up to 30 networks).
- `POST /api/wifi` `{ "ssid": "...", "password": "..." }` → `{ "ok": true, "rebooting": true, "ssid": "..." }` then reboots after ~800 ms. Empty SSID falls back to the saved one; blank password keeps the saved password for the same SSID; missing SSID → `400 { "error": "SSID required" }`.

## Agent (LLM provider)

- `GET /api/agent` → `{ "endpoint": "...", "model": "...", "api_key_masked": "sk-****42" }`.
- `POST /api/agent` `{ "endpoint": "https://api.openai.com/v1", "apiKey": "sk-…", "model": "gpt-4o-mini" }` (also accepts `api_key`) → `{ "ok": true, "rebooting": false }`. **No reboot, WiFi untouched.** Blank fields keep saved values; empty endpoint with nothing saved → `400`.
- `GET /api/models` → `{ "models": [{ "id": "llama3.1" }], "count": 1 }`. Proxies `GET {base}/models` on the saved endpoint (saved API key forwarded, up to 60 IDs). Feeds the chat composer picker. Errors: `400` (endpoint not configured), `503` (no WiFi), `502` (upstream fetch/parse failed).
- Legacy `GET /api/config` → `{ "wifi_ssid", "endpoint", "model", "api_key_masked" }`. Legacy `POST /api/config` accepts either WiFi fields (`ssid`, `password`) or agent fields (`endpoint`, `apiKey`/`api_key`, `model`) and delegates to the matching handler without wiping the other side; a fully combined save reboots (`src/routes.cpp:226-258`).

## Chat

- `POST /api/chat` `{ "message": "hi", "history": [{ "role": "user", "content": "..." }] }` → `200 { "reply": "...", "model": "...", "reasoning": "...", "tools_used": "gpio_read " }`. `reasoning` / `tools_used` are omitted when empty. Errors: `400` (empty message / endpoint not configured), `503` (no WiFi), `500/502` (`{ "error": "ERROR: …" }` from upstream). `history` is optional; max ~10 turns are used. Messages are capped at 2000 chars inbound.
- `POST /api/chat-stream` — same request shape, returns `text/event-stream` SSE: `data: {"delta": "…"}` (text), `data: {"reasoning": "…"}` (thinking), terminal `data: {"need_tools": true}` when the browser must fall back to `/api/chat` for the full tool loop, then `data: [DONE]`. Non-200 upstream statuses are returned as JSON `{ "error": "upstream HTTP …" }` instead of a stream.

Example:

```powershell
curl -X POST http://esp32-agent.local/api/chat `
  -H "Content-Type: application/json" `
  -H "Authorization: Bearer <device-password>" `
  -d '{"message":"what is your heap and IP?"}'
```

## Auth endpoints

- `GET /api/auth` (always open) → `{ "auth_required": false, "ap_mode": false }`.
- `POST /api/auth` `{ "token": "new-password-min-8" }` → `{ "ok": true, "auth_required": true }`. First set is open; changing/clearing requires current `Authorization`. Blank `token` removes the password (device becomes open).

## Tools and memory

- `GET /api/tools` → `{ "mask": 1023, "tools": [{ "name": "get_status", "enabled": true }, …], "prompt_preview": "You are ESP32-Agent …" }`.
- `POST /api/tools` `{ "enabled": ["get_status","gpio_read","blink"] }` (or `{ "mask": 1023 }`) → `{ "ok": true, "mask": 1023 }`. Takes effect immediately, no reboot. Tool schemas are in `docs/agent-tools.md`.
- `GET /api/memory` → `{ "memory": "- prefers …\n", "fs_ok": true, "log_bytes": 1234, "log_tail": [{ "role": "user", "content": "…" }] }` (last 10 lines, content truncated to 300 chars).
- `POST /api/memory` `{ "text": "my plant is a basil" }` saves one fact (≤500 chars); `{ "clear_log": true }` deletes the chat log; `{ "clear_memory": true }` deletes saved facts. Returns `{ "ok": true, "saved": true, "memory": "…" }`.

## Maintenance

- `POST /api/reset` → `{ "ok": true, "rebooting": true }`, wipes NVS settings **and** LittleFS log/memory, then reboots. The device comes back as a fresh unit (AP setup mode).
- `POST /update` (multipart file upload, field consumed via `server.upload()`) → `OK - rebooting...` then reboots. Upload the `firmware.bin` produced by `pio run` (see `docs/build-and-deployment.md`).
