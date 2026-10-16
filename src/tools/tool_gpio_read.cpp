// gpio_read — read a GPIO level.
#include "tool_api.h"
#include "agent_tools.h"

static void describe(JsonObject fn) {
  fn["description"] = "Read GPIO level.";
  JsonObject p = fn["parameters"].to<JsonObject>();
  p["type"] = "object";
  JsonObject pr = p["properties"].to<JsonObject>();
  pr["pin"].to<JsonObject>()["type"] = "integer";
  JsonArray r = p["required"].to<JsonArray>();
  r.add("pin");
}

static String run(JsonObject args) {
  JsonDocument out;
  int pin = args["pin"] | -1;
  if (!pinCanRead(pin)) { out["error"] = "pin not allowed"; }
  else { pinMode(pin, INPUT); out["pin"] = pin; out["level"] = digitalRead(pin); }
  String s;
  serializeJson(out, s);
  return s;
}

extern const ToolDef Tool_gpio_read = { "gpio_read", "Digital in", describe, run, 1200 };
