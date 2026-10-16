// gpio_write — set a GPIO output level. Copy me for int-arg tools.
#include "tool_api.h"
#include "agent_tools.h"

static void describe(JsonObject fn) {
  fn["description"] = "Set GPIO output level. Allowed pins 2,4,5,12-19,21-23,25-27,32,33.";
  JsonObject p = fn["parameters"].to<JsonObject>();
  p["type"] = "object";
  JsonObject pr = p["properties"].to<JsonObject>();
  pr["pin"].to<JsonObject>()["type"] = "integer";
  pr["level"].to<JsonObject>()["type"] = "integer";
  JsonArray r = p["required"].to<JsonArray>();
  r.add("pin");
  r.add("level");
}

static String run(JsonObject args) {
  JsonDocument out;
  int pin = args["pin"] | -1;
  int lvl = args["level"] | -1;
  if (!pinCanWrite(pin)) { out["error"] = "pin not allowed for output"; }
  else if (lvl != 0 && lvl != 1) { out["error"] = "level must be 0 or 1"; }
  else { pinMode(pin, OUTPUT); digitalWrite(pin, lvl); out["ok"] = true; out["pin"] = pin; out["level"] = lvl; }
  String s;
  serializeJson(out, s);
  return s;
}

extern const ToolDef Tool_gpio_write = { "gpio_write", "Digital out (safe pins)", describe, run, 1200 };
