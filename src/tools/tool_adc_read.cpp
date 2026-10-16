// adc_read — analog read (ADC1 only, WiFi-safe).
#include "tool_api.h"
#include "agent_tools.h"

static void describe(JsonObject fn) {
  fn["description"] = "Analog read. Best pins 32,33,34,35,36,39.";
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
  if (!pinCanADC(pin)) { out["error"] = "use ADC1 pins 32,33,34,35,36,39"; }
  else { out["pin"] = pin; out["value"] = analogRead(pin); }
  String s;
  serializeJson(out, s);
  return s;
}

extern const ToolDef Tool_adc_read = { "adc_read", "Analog (32,33,34,35,36,39)", describe, run, 1200 };
