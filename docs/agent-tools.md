# Agent Tools and LLM Bridge

Purpose: how the 10 device tools, memory, and the OpenAI-compatible tool loop work — and how to add a new tool.

## Tool catalogue

Registered in `src/tools/registry.cpp` (one file per tool: `src/tools/tool_*.cpp`). Default mask is all-on (`allToolsMask()`, `src/agent_tools.h`).

| Tool | Args | What it does (see its `src/tools/tool_*.cpp`) |
|------|------|-----------------------------------------------|
| `get_status` | — | `{ heap, uptime_s, wifi, rssi, ip, model }`. No pins touched. |
| `gpio_write` | `pin: int`, `level: 0\|1` | `pinMode(pin, OUTPUT)` + `digitalWrite`. Allowlist: `2,4,5,12,13,14,15,16,17,18,19,21,22,23,25,26,27,32,33`. |
| `gpio_read` | `pin: int` | `pinMode(pin, INPUT)` + `digitalRead`. Extends the write list with input-only `34,35,36,39`. |
| `adc_read` | `pin: int` | `analogRead`. Only `32,33,34,35,36,39` (ADC1); anything else returns `use ADC1 pins …`. |
| `wifi_scan` | — | Blocking `WiFi.scanNetworks()` (~2 s), top 10 as `{ ssid, rssi }` + `count`. |
| `mem_save` | `text: string` | Appends one `"- …"` line (≤500 chars, newlines flattened) to `/memory.txt`. |
| `mem_read` | — | Returns `/memory.txt` (first 2000 chars). |
| `restart_device` | `confirm: bool` | Requires `confirm=true`; sets `gRestartPending` — reboot happens after the HTTP reply (`src/routes.cpp:354`). |
| `blink` | `pin` (default 2), `count` (default 5), `on_ms`/`off_ms` (default 200) | Blocking blink ending LOW. Clamped: 1–20 blinks, 50–1000 ms each, ≤15 s total. Preferred over repeated `gpio_write` for blinking. |
| `web_fetch` | `url: string` | GETs `http(s)://…` (≤512 chars), 15 s timeout, follows redirects, strips scripts/styles/tags → ≤2000 chars of plain text (`fetchWebText` + `stripHtmlToText` in `src/tools/tool_web_fetch.cpp`). Rejects non-HTTP URLs and empty extraction with `{ error }`. |

Errors are always JSON `{ "error": "…" }` (unknown tool, disallowed pin, disabled tool, over-budget skip). Tool payloads are capped per tool (`maxResultChars`: 1200, 2200 for `web_fetch`) before being sent back to the model (`execTool()`, `src/agent_tools.cpp`).

## Pin safety

`pinCanWrite()` / `pinCanRead()` / `pinCanADC()` (`src/agent_tools.cpp:137-146`) encode the ESP32 safe set: strapping and flash pins are excluded, and ADC is limited to ADC1 because ADC2 conflicts with WiFi. The LLM only ever sees the allowlist in the function descriptions (`src/agent_tools.cpp:188-227`), and `execTool()` re-validates every call — a model cannot talk its way onto a blocked pin.

## Memory and log (LittleFS)

- `initAgentFS()` mounts LittleFS with format-on-first-boot (`src/agent_tools.cpp:33-36`).
- `/memory.txt` (cap 4096 B): durable facts. `appendMemoryFact()` appends one line and drops oldest lines past the cap (`src/agent_tools.cpp:48-68`). Surfaced to the model in two ways: injected into the system prompt (first 1200 chars, `src/agent_tools.cpp:161-164`) and via `mem_read`.
- `/chatlog.jsonl` (cap 32768 B): `{"r":"user"|"assistant","c":"…"}` per line, one append per completed turn (`appendChatLog`, `src/agent_tools.cpp:72-100`). Rotation keeps the last ~16 KB aligned to a line start. `injectRecentLog()` replays the last 8 entries into every request while skipping an exact duplicate of the current message (`src/agent_tools.cpp:104-135`).

## System prompt and tool schema

`buildSystemPrompt()` (`src/agent_tools.cpp:149-173`) lists **only enabled tools** (saves tokens), instructs the model to use tools instead of guessing hardware state, demands a closing plain-text summary, appends long-term memory, and appends a live snapshot (`ip`, `uptime`, `heap`, `rssi`).

`addToolsArray()` (`src/agent_tools.cpp:176-304`) emits the OpenAI `tools` array with only enabled functions and sets `tool_choice: auto` when at least one tool exists. Disabling everything is valid — the model then answers from knowledge only.

## The agentic loop (`callLLMWithTools`)

Full implementation in `src/llm.cpp:55-235`:

1. Build `{ model, temperature: 0.7, max_tokens: 800, stream: false, messages: [system + recent-log(8) + history(8) + user], tools }`.
2. `POST {endpoint}/chat/completions` via `postLLM()` (`src/llm.cpp:8-35`): HTTP/1.0, 45 s timeout, `Authorization: Bearer <key>`, `HTTP-Referer`/`X-Title` headers.
3. Parse `choices[0].message.tool_calls` (plus legacy `function_call`). Collect up to 20 calls; execute each via `execTool()` (25 s total time budget — over-budget calls return a `skipped` error so `tool_call_id` pairing stays valid); append `role: tool` results.
4. Repeat up to 3 iterations. Preserve any same-message narration (`pendingPrefix`) and accumulate reasoning fields (`reasoning_content` / `reasoning` / `reasoning_details[0].text`).
5. If no final text arrives but tool results exist, synthesize a fallback (`Ran <tools> … Results: …`) rather than returning empty. Persist the turn with `appendChatLog()`.

`callLLM()` (`src/llm.cpp:237-366`) is the older non-tool path (still used as a reference for provider-variant parsing); production chat goes through `callLLMWithTools()`.

## Adding a new tool (one file, one line)

> New here? Start with the friendly walkthrough: [`adding-a-tool.md`](./adding-a-tool.md).

1. Copy `src/tools/_template.cpp.sample` to `src/tools/tool_mything.cpp` and fill in the 3 marked spots (name, `describe()` schema, `run()` executor returning a small JSON string).
2. In `src/tools/registry.cpp`: add the `extern const ToolDef Tool_mything;` line and append `&Tool_mything` at the END of `TOOL_REGISTRY` (order = bit in `gToolsMask`; never reorder).
3. `pio run`. Verify via `GET /api/tools` and toggle on the `/functions` page — no reboot needed. Old "all on" masks auto-enable new tools (`src/config.cpp`); no mask edit needed.

Keep executors fast (< a few seconds), small (< 1 KB result), and side-effect-explicit: anything destructive (like `restart_device`) must require an explicit confirmation arg. Shared policy (pin allowlists) stays in `src/agent_tools.cpp` — don't copy it per tool.
