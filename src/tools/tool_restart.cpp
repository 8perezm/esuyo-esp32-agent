// restart_device — reboot. Requires confirm=true; reboot happens after reply.
#include "tool_api.h"
#include "agent_tools.h"

static void describe(JsonObject fn) {
  fn["description"] = "Restart ESP32. Only when user explicitly asks. Requires confirm=true.";
  JsonObject p = fn["parameters"].to<JsonObject>();
  p["type"] = "object";
  JsonObject pr = p["properties"].to<JsonObject>();
  pr["confirm"].to<JsonObject>()["type"] = "boolean";
  JsonArray r = p["required"].to<JsonArray>();
  r.add("confirm");
}

static String run(JsonObject args) {
  JsonDocument out;
  bool c = args["confirm"] | false;
  if (!c) out["error"] = "needs confirm=true";
  else { out["ok"] = true; out["restarting"] = true; gRestartPending = true; }
  String s;
  serializeJson(out, s);
  return s;
}

extern const ToolDef Tool_restart_device = { "restart_device", "Restart (needs confirm)", describe, run, 1200 };
