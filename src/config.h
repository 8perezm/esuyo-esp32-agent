#pragma once
// config.h — persisted settings (WiFi, endpoint, model, tools mask) + small helpers.
// Agents: this is the place for prefs / cfg* changes. Small file, safe to read.
#include <Arduino.h>
#include <Preferences.h>

extern const char* AP_SSID;
extern const char* AP_PASS;
extern const char* MDNS_NAME;
extern const char* PREFS_NS;

extern Preferences prefs;

extern String cfgSsid;
extern String cfgPass;
extern String cfgEndpoint;
extern String cfgApiKey;
extern String cfgModel;

extern uint32_t gToolsMask;

void loadConfig();
void saveToolsMask(uint32_t m);
void saveConfig(const String& ssid, const String& pass,
                const String& endpoint, const String& key, const String& model);
void saveWifiOnly(const String& ssid, const String& pass);
void saveAgentOnly(const String& endpoint, const String& key, const String& model);
String chatCompletionsUrl(const String& base);
String modelsUrl(const String& base);
String maskedKey(const String& k);
