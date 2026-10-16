// get_status — read-only device snapshot. Copy me for no-arg tools.
#include "tool_api.h"
#include "config.h"
#include <WiFi.h>

static void describe(JsonObject fn) {
  fn["description"] = "Get ESP32 status: heap, uptime, wifi, ip, model";
  JsonObject p = fn["parameters"].to<JsonObject>();
  p["type"] = "object";
  p["properties"].to<JsonObject>();
}

static String run(JsonObject args) {
  (void)args;
  JsonDocument out;
  out["heap"] = ESP.getFreeHeap();
  out["uptime_s"] = (int)(millis() / 1000);
  out["wifi"] = WiFi.SSID();
  out["rssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  out["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  out["model"] = cfgModel;
  String s;
  serializeJson(out, s);
  return s;
}

extern const ToolDef Tool_get_status = { "get_status", "Heap, uptime, WiFi, IP", describe, run, 1200 };
