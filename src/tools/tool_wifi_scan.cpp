// wifi_scan — blocking scan (~2s), top 10.
#include "tool_api.h"
#include <WiFi.h>

static void describe(JsonObject fn) {
  fn["description"] = "Scan nearby WiFi networks (takes ~2s).";
  JsonObject p = fn["parameters"].to<JsonObject>();
  p["type"] = "object";
  p["properties"].to<JsonObject>();
}

static String run(JsonObject args) {
  (void)args;
  JsonDocument out;
  int n = WiFi.scanNetworks();
  JsonArray arr = out["networks"].to<JsonArray>();
  for (int i = 0; i < n && i < 10; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = WiFi.SSID(i);
    o["rssi"] = WiFi.RSSI(i);
  }
  out["count"] = n;
  WiFi.scanDelete();
  String s;
  serializeJson(out, s);
  return s;
}

extern const ToolDef Tool_wifi_scan = { "wifi_scan", "Scan networks (~2s)", describe, run, 1200 };
