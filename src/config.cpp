#include "config.h"

const char* AP_SSID = "ESP32-Agent-Setup";
const char* AP_PASS = "";  // open AP for easy onboarding
const char* MDNS_NAME = "esp32-agent";
const char* PREFS_NS = "esp32agent";

Preferences prefs;

String cfgSsid = "";
String cfgPass = "";
String cfgEndpoint = "";   // base e.g. https://api.openai.com/v1  (or full .../chat/completions)
String cfgApiKey = "";
String cfgModel = "gpt-4o-mini";

uint32_t gToolsMask = 0x3FF;  // default all on (10 tools); restart still needs confirm=true

void loadConfig() {
  prefs.begin(PREFS_NS, true);
  cfgSsid = prefs.getString("wifi_ssid", "");
  cfgPass = prefs.getString("wifi_pass", "");
  cfgEndpoint = prefs.getString("api_endpoint", "");
  cfgApiKey = prefs.getString("api_key", "");
  cfgModel = prefs.getString("api_model", "gpt-4o-mini");
  if (cfgModel.length() == 0) cfgModel = "gpt-4o-mini";
  gToolsMask = prefs.getUInt("tools_mask", 0x3FF);
  if (gToolsMask == 0) gToolsMask = 0x3FF;
  if (gToolsMask == 0xFF || gToolsMask == 0x1FF) gToolsMask = 0x3FF;  // pre-web_fetch defaults: enable new tools too
  prefs.end();
}

void saveToolsMask(uint32_t m) {
  gToolsMask = m;
  prefs.begin(PREFS_NS, false);
  prefs.putUInt("tools_mask", m);
  prefs.end();
}

void saveConfig(const String& ssid, const String& pass,
                const String& endpoint, const String& key, const String& model) {
  prefs.begin(PREFS_NS, false);
  prefs.putString("wifi_ssid", ssid);
  prefs.putString("wifi_pass", pass);
  prefs.putString("api_endpoint", endpoint);
  prefs.putString("api_key", key);
  prefs.putString("api_model", model.length() ? model : "gpt-4o-mini");
  prefs.end();
  cfgSsid = ssid; cfgPass = pass;
  cfgEndpoint = endpoint; cfgApiKey = key;
  cfgModel = model.length() ? model : "gpt-4o-mini";
}

// Normalize user-supplied endpoint to full .../chat/completions URL.
String chatCompletionsUrl(const String& base) {
  String u = base;
  u.trim();
  // strip trailing slashes
  while (u.endsWith("/")) u.remove(u.length() - 1);
  if (u.endsWith("/chat/completions")) return u;
  if (u.endsWith("/chat/completion")) return u + "s";
  return u + "/chat/completions";
}

// Same normalization, but for the OpenAI-style model list (GET {base}/models).
String modelsUrl(const String& base) {
  String u = base;
  u.trim();
  while (u.endsWith("/")) u.remove(u.length() - 1);
  const String suffix = "/chat/completions";
  if (u.endsWith(suffix)) u.remove(u.length() - suffix.length());
  return u + "/models";
}

String maskedKey(const String& k) {
  if (k.length() <= 8) return "****";
  return k.substring(0, 4) + "****" + k.substring(k.length() - 2);
}

void saveWifiOnly(const String& ssid, const String& pass) {
  prefs.begin(PREFS_NS, false);
  prefs.putString("wifi_ssid", ssid);
  prefs.putString("wifi_pass", pass);
  prefs.end();
  cfgSsid = ssid; cfgPass = pass;
}

void saveAgentOnly(const String& endpoint, const String& key, const String& model) {
  prefs.begin(PREFS_NS, false);
  prefs.putString("api_endpoint", endpoint);
  prefs.putString("api_key", key);
  prefs.putString("api_model", model.length() ? model : "gpt-4o-mini");
  prefs.end();
  cfgEndpoint = endpoint; cfgApiKey = key;
  cfgModel = model.length() ? model : "gpt-4o-mini";
}
