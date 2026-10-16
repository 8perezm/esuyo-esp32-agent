// blink — blink a pin N times with visible timing, ends LOW.
// Blocking (up to 15s total). Preferred over repeated gpio_write.
#include "tool_api.h"
#include "agent_tools.h"

static void describe(JsonObject fn) {
  fn["description"] = "Blink a GPIO pin N times with visible timing (preferred over repeated gpio_write for blinking). Ends LOW. Allowed pins 2,4,5,12-19,21-23,25-27,32,33.";
  JsonObject p = fn["parameters"].to<JsonObject>();
  p["type"] = "object";
  JsonObject pr = p["properties"].to<JsonObject>();
  pr["pin"].to<JsonObject>()["type"] = "integer";
  pr["count"].to<JsonObject>()["type"] = "integer";
  pr["on_ms"].to<JsonObject>()["type"] = "integer";
  pr["off_ms"].to<JsonObject>()["type"] = "integer";
  JsonArray r = p["required"].to<JsonArray>();
  r.add("pin");
}

static String run(JsonObject args) {
  JsonDocument out;
  int pin = args["pin"] | 2;
  int count = args["count"] | 5;
  int onms = args["on_ms"] | 200;
  int offms = args["off_ms"] | 200;
  if (!pinCanWrite(pin)) { out["error"] = "pin not allowed for output"; }
  else {
    if (count < 1) count = 1; if (count > 20) count = 20;
    if (onms < 50) onms = 50; if (onms > 1000) onms = 1000;
    if (offms < 50) offms = 50; if (offms > 1000) offms = 1000;
    if ((long)count * (onms + offms) > 15000) count = 15000 / (onms + offms);  // <=15s block
    if (count < 1) count = 1;
    pinMode(pin, OUTPUT);
    for (int i = 0; i < count; i++) {
      digitalWrite(pin, HIGH); delay(onms);
      digitalWrite(pin, LOW); delay(offms);
    }
    out["ok"] = true; out["pin"] = pin; out["blinks"] = count;
  }
  String s;
  serializeJson(out, s);
  return s;
}

extern const ToolDef Tool_blink = { "blink", "Blink a pin N times", describe, run, 1200 };
