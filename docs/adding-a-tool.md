# Adding a New Tool

A "tool" is a superpower you give the chat: blink a light, read a sensor,
move a servo. If you can describe it in one sentence and write a few lines
of Arduino code, you can add it. No need to understand the whole project.

**Time:** ~10 minutes. **You will touch:** 1 new file + 1 line elsewhere.

## What you'll need

- The project building already (`pio run` succeeds).
- A copy of the starter file: `src/tools/_template.cpp.sample`.
- An idea, e.g. *"set my servo on pin 13 to an angle"*.

## Step 1 — Copy the starter

Copy the template to a new file. The name must start with `tool_` and end
with `.cpp`:

```
src/tools/_template.cpp.sample  →  src/tools/tool_servo.cpp
```

> The `.sample` file is invisible to the compiler on purpose — only real
> `.cpp` files get built.

## Step 2 — Fill in the 3 spots

Open your new file. There are 3 marked spots:

**Spot 1 — NAME.** One word, lowercase with underscores. This is what the
AI model sees and calls. For our example:

```
static const char* NAME = "servo";
```

**Spot 2 — `describe()`.** Tell the model what the tool does and what
arguments it takes, in plain words. Each argument needs a type
(`integer`, `string`, `boolean`) and you list which ones are required:

```
fn["description"] = "Set a servo to an angle (0-180). My servo is on pin 13.";
... pin (integer, required), angle (integer, required) ...
```

Tip: look at `src/tools/tool_blink.cpp` — it's the best example of a tool
with several arguments and safe limits.

**Spot 3 — `run()`.** The actual Arduino code. Read arguments with a
sensible default (`args["angle"] | 90` means "90 if the model didn't say"),
do the hardware thing, and return a tiny JSON answer:

```
int angle = args["angle"] | 90;
... move the servo ...
out["ok"] = true; out["angle"] = angle;
```

Three rules for `run()`:

1. **Fast** — under a few seconds. The chat waits while it runs.
2. **Small answer** — under ~1 KB of JSON.
3. **Dangerous things need a key** — like `restart_device` needs
   `confirm=true`, so the model can't do it by accident.

Shared helpers already exist — use them, don't copy them:

- `pinCanWrite(pin)` / `pinCanRead(pin)` / `pinCanADC(pin)` — safe-pin lists.
- `appendMemoryFact(text)` / `readMemoryFacts()` — long-term memory.

## Step 3 — Register it (one line)

Open `src/tools/registry.cpp` and do two tiny things:

1. Add one `extern` line at the top:
   `extern const ToolDef Tool_servo;`
2. Append `&Tool_servo,` at the **END** of the `TOOL_REGISTRY` list.

> Why the end? The position in the list is the tool's on/off bit in saved
> settings. Appending keeps everyone's `/functions` toggles intact.

## Step 4 — Build and try it

```powershell
pio run
```

Then:

1. Open `/functions` in the browser — your tool is there with its label.
   Tick it on (new tools default on for fresh "everything on" setups).
2. Chat: *"set my servo to 90 degrees"* — open the *Thinking* block to
   watch the model call your tool.
3. Check `GET /api/tools` to see your schema exactly as the model sees it.

## Step 5 (optional) — Tell the docs

Add one row to the tool catalogue table in `docs/agent-tools.md` so the
next person finds it.

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| `undefined reference to Tool_x` at link time | The last line of your file must say `extern const ToolDef Tool_x` (with `extern` — C++ hides plain `const` globals from other files). |
| Tool missing from `/api/tools` | Forgot the `registry.cpp` line, or the name collides with an existing tool. |
| Model never calls your tool | Is it ticked on `/functions`? Is the description concrete (pin numbers, ranges beat vague words)? |
| Chat hangs during your tool | `run()` blocks the web server — keep it short or clamp it like `blink` does (max 15 s). |
| Toggles scrambled after update | A tool was inserted in the middle of `registry.cpp` instead of appended. |

## Limits

- Max 32 tools (on/off bits fit in one number).
- New tools are auto-enabled for anyone who previously had "everything
  on" — nobody needs to touch settings after updating.

That's it. Copy, fill 3 spots, register, build. Happy tinkering!
