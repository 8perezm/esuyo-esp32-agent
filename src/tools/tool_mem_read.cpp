// mem_read — read saved memory facts.
#include "tool_api.h"
#include "agent_tools.h"

static void describe(JsonObject fn) {
  fn["description"] = "Read saved memory facts.";
  JsonObject p = fn["parameters"].to<JsonObject>();
  p["type"] = "object";
  p["properties"].to<JsonObject>();
}

static String run(JsonObject args) {
  (void)args;
  JsonDocument out;
  out["memory"] = readMemoryFacts().substring(0, 2000);
  String s;
  serializeJson(out, s);
  return s;
}

extern const ToolDef Tool_mem_read = { "mem_read", "Read memory facts", describe, run, 1200 };
