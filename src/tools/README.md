# Tools — one file per function

Copy `_template.cpp.sample` to `tool_mything.cpp`, fill in the 3 marked
spots, add one line to `registry.cpp`. Done.

## Files

| File | What |
|------|------|
| `tool_api.h` | `ToolDef` struct + `TOOL_REGISTRY`. Only shared file. |
| `registry.cpp` | Ordered list of tools. **Append-only** — order = bit in `gToolsMask` (saved `/functions` toggles). |
| `tool_*.cpp` | One tool each: `describe()` (LLM schema) + `run()` (executor). |
| `_template.cpp.sample` | Starter, not compiled (`.sample` so PlatformIO ignores it). |

## Add a tool

1. Copy template to `tool_mything.cpp`.
2. Set name, schema, run logic.
3. `registry.cpp`: add `extern` + append `&Tool_mything` at the END.
4. `pio run`. Toggle on `/functions`.
5. (Optional) add a row to `docs/agent-tools.md` catalogue table.

## Edit a tool

Just open its `tool_*.cpp` — schema and behavior live together. Nothing
else to touch unless you change shared policy (`pinCanWrite()` etc. in
`src/agent_tools.cpp:137-146`).

## Limits

- `uint32_t` mask → max 32 tools.
- Keep `run()` fast and results < ~1KB (`maxResultChars`, 2200 for web text).
