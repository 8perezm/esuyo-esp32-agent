# ESP32 Agent

> A chat harness on your ESP32 — you can ask it to do things connected to the device.

You can ask it to for example "Blink the light three times." "Is the soil dry?" "Remember that I like my workshop at 21 degrees." "What's in the news today?" It listens, figures out what you mean, uses its pins and sensors to check or do something, and answers back — like texting a handy friend who happens to live inside your electronics.

That's what this project is: it gives your ESP32 ears, hands, and a memory. The understanding part comes from whatever AI chat service you already use. 

Use it as a starting point for you to further build your ideas "what if my plant pot could answer questions?".

It's deliberately kept small so you can own it and make it yours. Flash it, play with it for ten minutes, then use it as the base for your own project, a greenhouse monitor, a garage door buddy, a classroom demo. The boring plumbing is already done, so you can spend your time on the fun part: what you want yours to do.

## Features

- **Phone-friendly setup:** first boot creates an open `ESP32-Agent-Setup` network with a captive portal. Pick your home WiFi and paste your AI endpoint without touching code.
- **Chat in your browser:** fast streaming replies with a "Thinking" view, chat history, and a model picker.
- **It can touch hardware:** ask it to report heap/uptime/WiFi, read or write GPIO, read analog sensors, scan nearby networks, or blink a pin.
- **It remembers:** tell it "my plant is a basil" and it saves the fact to onboard flash, recalls it next week, and shows it on the memory page.
- **It can look things up:** ask for fresh info and it fetches web pages as plain text (no JavaScript, no logins).
- **Works with your provider:** any endpoint that speaks `POST {base}/chat/completions` — OpenAI, Together, or a local gateway. The model picker lists whatever `GET {base}/models` on your endpoint returns.
- **No cable after day one:** update firmware from your browser or with one PlatformIO command. Toggle capabilities on the Functions page without reflashing.
- **Honest status:** `/api/status` and the on-device Docs page always tell you IP, model, heap, and what is configured.

## Prerequisites

- **ESP32 dev board** (ESP32-WROOM / `esp32dev` style with USB serial) + USB cable. The onboard LED (usually GPIO 2) is enough to try blinking.
- **A computer** with [PlatformIO](https://platformio.org/) installed (CLI or VS Code extension) for the first flash, plus the USB driver for your board (CP210x or CH340).
- **2.4 GHz WiFi** credentials the ESP32 can join.
- **A phone or laptop** with a browser for setup and chat.
- **An AI endpoint + key + model name** — for example `https://api.openai.com/v1` with `gpt-4o-mini`, or any OpenAI-compatible provider. You paste these into a web form; they are stored only on the ESP32.

## Installation

One USB flash, then everything is wireless.

```powershell
# From the project folder:

pio run                                  # compile check — no hardware needed

pio run --target upload                  # first flash over USB
# If your board is not on COM8, override it:
# pio run --target upload --upload-port COM5

pio device monitor --baud 115200         # watch boot logs, note the IP address
```

What to expect:

1. The log prints `=== Agent boot ===`, then either `WiFi OK: 192.168.x.x` or `AP started: ESP32-Agent-Setup -> http://192.168.4.1/setup`.
2. Note the IP address, or open `http://esp32-agent.local/` if your network resolves mDNS.
3. Leave the monitor running while you configure — it confirms every save.

Only the very first flash needs USB and possibly the BOOT button (see Troubleshooting). After that, flash with either of these — no cable, no button:

```powershell
pio run --target upload --upload-port 192.168.1.50   # your ESP's IP from the logs
```

Or in a browser: open `http://<ESP_IP>/update`, choose `.pio/build/esp32dev/firmware.bin`, Upload. The device reboots itself.

## Configuration

All settings are entered in the browser and stored on the device. Blank fields keep whatever was already saved.

1. **Join WiFi:** power the ESP32. On your phone/laptop join the open network **`ESP32-Agent-Setup`**, then open **`http://192.168.4.1/wifi`** (most devices redirect there automatically).
2. **Save WiFi:** press Scan, pick your network, enter the password, Save & Reboot. Blank password keeps the saved one when the network name is unchanged.
3. **Rejoin your normal WiFi** on your phone/laptop — the setup network disappears after the reboot.
4. **Point at your AI:** open `http://esp32-agent.local/` (or the IP from the serial log), go to **Setup → Agent** (`/agent`), and enter:
   - Endpoint base, e.g. `https://api.openai.com/v1` (`/chat/completions` is added automatically)
   - API key (`sk-…`) — shown masked after saving
   - Model, e.g. `gpt-4o-mini`
   
   Save applies instantly — no reboot, WiFi stays connected.
5. **Verify:** the chat header warning disappears. If it still shows a warning, click through to `/wifi` or `/agent`.

Handy pages (also linked from `/setup`):

| Page | Address | Use |
|------|---------|-----|
| Chat | `/` | Talk to the device |
| Setup hub | `/setup` | Links to everything below |
| WiFi | `/wifi` | Change networks (reboots) |
| Agent | `/agent` | Change endpoint / key / model (no reboot) |
| Functions | `/functions` | Turn capabilities on/off |
| Docs | `/docs` | On-device quick reference |
| Firmware | `/update` | Upload `firmware.bin` from a browser |

## Quick Start / Usage

Open the chat page and just type. History is kept on the device, so follow-ups work ("and blink it twice").

Try these in order:

```text
what is your heap, uptime and IP?
scan for nearby wifi networks
blink GPIO 2 five times
read GPIO 34 as analog
remember that my plant is a basil
what did I tell you about my plant?
fetch https://example.com and summarize it in one line
```

A few things worth knowing:

- **Streaming + fallback:** most answers stream word-by-word. When the model wants to use a tool (pins, WiFi, web), the page automatically runs the full agent pass and shows what ran.
- **Thinking:** open the *Thinking* block under a reply to see the model's reasoning when the provider shares it.
- **Functions page:** untick anything you don't want the model to touch (e.g. `restart_device`, `gpio_write`). Changes apply immediately.
- **Memory:** view or clear saved facts and recent chat on the memory panel. One fact per line, kept in onboard flash across reboots and firmware updates.
- **Stop / start:** there is no power button in software — unplug to stop, plug in to start. To start over completely (wipe WiFi, keys, memory), send `POST /api/reset` or reflash and reset from the UI. The device comes back as a fresh `ESP32-Agent-Setup` unit.

When you are ready to make it yours — new tools, new prompt, new pages — the developer guides below walk through exactly where each piece lives.

## Troubleshooting / FAQ

**Upload fails with `Wrong boot mode 0x13`?**
Your board's auto-reset circuit is missing or flaky. Hold `BOOT`, tap `EN`, release `BOOT`, then retry `pio run --target upload`. One manual step — OTA takes over after that.

**I joined `ESP32-Agent-Setup` but nothing loads?**
Open `http://192.168.4.1/wifi` manually. Some phones need mobile data turned off to stay on a network without internet.

**`http://esp32-agent.local/` doesn't resolve?**
Use the IP from `pio device monitor` (the `WiFi OK:` line) instead, e.g. `http://192.168.1.50/`. mDNS works on most home networks but not all Windows/VPN setups.

**Chat says "no WiFi" or "endpoint not configured"?**
Follow the banner links to `/wifi` and `/agent`. Remember: saving WiFi reboots the device (rejoin your home network afterwards); saving the agent endpoint does not.

**Upstream HTTP 401 / 404 / "empty reply"?**
Recheck endpoint, key, and exact model name on `/agent`. The on-device probe `GET /api/debug-raw?message=Reply%20with%20just%20OK` shows the raw provider response without leaking your key.

**Can I reach it from outside my home?**
Not by default — it serves on your LAN only (port 80), which is the safe choice behind home routers and CGNAT. On the same network, just use the `.local` name or IP. For remote access, either port-forward `:80` on your router (add a password first — there is none today), or run Tailscale / `cloudflared` on a PC that can see the ESP's IP and proxy to it.

**How do I wipe everything?**
`POST /api/reset` clears WiFi, endpoint, key, model, memory, and chat log, then reboots into setup mode.

## License

MIT — Copyright (c) 2026 Miguel Perez. See [LICENSE](./LICENSE).

## Developer Documentation

- [Architecture](./docs/architecture.md) — Tech stack, runtime modes, and request/data flow
- [Project Structure](./docs/project-structure.md) — Folder layout and where to change things
- [Development Guide](./docs/development.md) — Local setup, flashing, OTA, and debugging
- [API Reference](./docs/api.md) — Pages and JSON endpoints with request/response shapes
- [Agent Tools and LLM Bridge](./docs/agent-tools.md) — The 10 tools, memory, and how to add a tool
- [Build and Deployment](./docs/build-and-deployment.md) — Environments, USB/OTA flashing, flash layout
