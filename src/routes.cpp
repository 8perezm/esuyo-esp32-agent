#include "routes.h"
#include "config.h"
#include "agent_tools.h"
#include "llm.h"
#include "net.h"
#include "web_ui.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <Preferences.h>

void handleStatus() {
  JsonDocument d;
  d["ap_mode"] = apMode;
  d["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
  d["ssid"] = WiFi.SSID();
  d["ip"] = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  d["mdns"] = String("http://") + MDNS_NAME + ".local";
  d["endpoint_configured"] = (cfgEndpoint.length() > 0);
  d["endpoint"] = cfgEndpoint;
  d["model"] = cfgModel;
  d["free_heap"] = ESP.getFreeHeap();
  d["uptime_s"] = millis() / 1000;
  d["fs_ok"] = gFsOk;
  d["tools_mask"] = (int)gToolsMask;
  if (gFsOk) {
    File lf = LittleFS.open(LOG_PATH, "r");
    d["log_bytes"] = lf ? (int)lf.size() : 0;
    if (lf) lf.close();
    File mf = LittleFS.open(MEM_PATH, "r");
    d["mem_bytes"] = mf ? (int)mf.size() : 0;
    if (mf) mf.close();
  }
  String s; serializeJson(d, s);
  server.send(200, "application/json", s);
}

void handleGetTools() {
  JsonDocument d;
  JsonArray arr = d["tools"].to<JsonArray>();
  for (int i = 0; i < TOOL_COUNT; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["name"] = TOOL_NAMES[i];
    o["enabled"] = toolEnabledIdx(i);
  }
  d["mask"] = (int)gToolsMask;
  d["prompt_preview"] = buildSystemPrompt().substring(0, 600);
  String s; serializeJson(d, s);
  server.send(200, "application/json", s);
}

void handleSaveTools() {
  if (!server.hasArg("plain")) { server.send(400, "application/json", "{\"error\":\"empty body\"}"); return; }
  JsonDocument d;
  if (deserializeJson(d, server.arg("plain"))) {
    server.send(400, "application/json", "{\"error\":\"invalid JSON\"}"); return;
  }
  uint32_t mask = 0;
  JsonArray en = d["enabled"].as<JsonArray>();
  if (!en.isNull()) {
    for (const char* n : en) {
      int i = toolIndex(String(n));
      if (i >= 0) mask |= (1UL << i);
    }
  } else {
    // allow {mask:123} or per-tool bools
    if (!d["mask"].isNull()) mask = d["mask"].as<uint32_t>();
    else { server.send(400, "application/json", "{\"error\":\"enabled[] required\"}"); return; }
  }
  saveToolsMask(mask);
  Serial.printf("[tools] mask=%lu\n", (unsigned long)mask);
  JsonDocument out; out["ok"] = true; out["mask"] = (int)mask;
  String s; serializeJson(out, s);
  server.send(200, "application/json", s);
}

void handleGetMemory() {
  JsonDocument d;
  d["memory"] = readMemoryFacts().substring(0, 3000);
  d["fs_ok"] = gFsOk;
  JsonArray tail = d["log_tail"].to<JsonArray>();
  if (gFsOk && LittleFS.exists(LOG_PATH)) {
    File f = LittleFS.open(LOG_PATH, "r");
    if (f) {
      d["log_bytes"] = (int)f.size();
      String all = f.readString();
      f.close();
      // last 10 lines
      int lines = 0;
      for (unsigned i = 0; i < all.length(); i++) if (all[i]=='\n') lines++;
      int skip = lines > 10 ? lines - 10 : 0;
      int pos = 0, ln = 0;
      while (pos < (int)all.length() && (int)tail.size() < 10) {
        int nl = all.indexOf('\n', pos);
        String l = nl<0 ? all.substring(pos) : all.substring(pos, nl);
        if (nl<0) pos = all.length(); else pos = nl+1;
        if (ln++ < skip) continue;
        l.trim(); if (!l.length()) continue;
        JsonDocument e;
        if (!deserializeJson(e, l)) {
          JsonObject o = tail.add<JsonObject>();
          o["role"] = String((const char*)(e["r"]|""));
          o["content"] = String((const char*)(e["c"]|"")).substring(0,300);
        }
      }
    }
  }
  String s; serializeJson(d, s);
  server.send(200, "application/json", s);
}

void handleSaveMemory() {
  if (!server.hasArg("plain")) { server.send(400, "application/json", "{\"error\":\"empty body\"}"); return; }
  JsonDocument d;
  if (deserializeJson(d, server.arg("plain"))) {
    server.send(400, "application/json", "{\"error\":\"invalid JSON\"}"); return;
  }
  JsonDocument out;
  if (d["clear_log"] | false) {
    if (gFsOk) LittleFS.remove(LOG_PATH);
    out["cleared_log"] = true;
  }
  if (d["clear_memory"] | false) {
    if (gFsOk) LittleFS.remove(MEM_PATH);
    out["cleared_memory"] = true;
  }
  String t = String((const char*)(d["text"] | ""));
  if (t.length()) out["saved"] = appendMemoryFact(t);
  out["ok"] = true;
  out["memory"] = readMemoryFacts().substring(0, 2000);
  String s; serializeJson(out, s);
  server.send(200, "application/json", s);
}

void handleScan() {
  // Non-blocking: async scan + 30s cache. A blocking scanNetworks() here
  // wedged server.handleClient() for ~7s (measured), stalling every other
  // page. Now we never block: return cache or 202 while scanning.
  static String sCache = "";
  static unsigned long sCacheAt = 0;
  unsigned long now = millis();
  server.sendHeader("Connection", "close");
  server.sendHeader("Cache-Control", "no-store");
  if (sCache.length() && (now - sCacheAt) < 30000) {
    server.send(200, "application/json", sCache);
    return;
  }
  int st = WiFi.scanComplete();
  if (st == WIFI_SCAN_RUNNING) {
    if (sCache.length()) { server.send(200, "application/json", sCache); return; }
    server.send(202, "application/json", "{\"scanning\":true,\"networks\":[]}");
    return;
  }
  if (st >= 0) {
    // Scan finished — build response from results, cache it, free buffers.
    JsonDocument d;
    JsonArray arr = d["networks"].to<JsonArray>();
    for (int i = 0; i < st && i < 30; i++) {
      JsonObject o = arr.add<JsonObject>();
      o["ssid"] = WiFi.SSID(i);
      o["rssi"] = WiFi.RSSI(i);
      o["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    }
    d["count"] = st;
    WiFi.scanDelete();
    String s; serializeJson(d, s);
    sCache = s; sCacheAt = now;
    server.send(200, "application/json", s);
    return;
  }
  // st == WIFI_SCAN_FAILED (no scan yet, or expired) — kick off async scan.
  WiFi.scanNetworks(true);
  if (sCache.length()) { server.send(200, "application/json", sCache); return; }
  server.send(202, "application/json", "{\"scanning\":true,\"networks\":[]}");
}

void handleGetWifi() {
  JsonDocument d;
  d["wifi_ssid"] = cfgSsid;
  String s; serializeJson(d, s);
  server.send(200, "application/json", s);
}

void handleSaveWifi() {
  if (!server.hasArg("plain")) { server.send(400, "application/json", "{\"error\":\"empty body\"}"); return; }
  JsonDocument d;
  if (deserializeJson(d, server.arg("plain"))) {
    server.send(400, "application/json", "{\"error\":\"invalid JSON\"}"); return;
  }
  String ssid = String((const char*)(d["ssid"] | ""));
  String pass = String((const char*)(d["password"] | ""));
  ssid.trim();
  // Blank password keeps the saved one when SSID is unchanged.
  // This fixes the old bug where an endpoint-only save wiped WiFi.
  if (!ssid.length()) ssid = cfgSsid;
  if (!ssid.length()) { server.send(400, "application/json", "{\"error\":\"SSID required\"}"); return; }
  if (!pass.length() && ssid == cfgSsid) pass = cfgPass;
  saveWifiOnly(ssid, pass);
  Serial.printf("Saved WiFi: ssid=%s (pass %u chars)\n", ssid.c_str(), pass.length());
  JsonDocument out; out["ok"] = true; out["rebooting"] = true; out["ssid"] = ssid;
  String s; serializeJson(out, s);
  server.send(200, "application/json", s);
  delay(800);
  ESP.restart();
}

void handleGetAgent() {
  JsonDocument d;
  d["endpoint"] = cfgEndpoint;
  d["model"] = cfgModel;
  d["api_key_masked"] = maskedKey(cfgApiKey);
  String s; serializeJson(d, s);
  server.send(200, "application/json", s);
}

void handleSaveAgent() {
  if (!server.hasArg("plain")) { server.send(400, "application/json", "{\"error\":\"empty body\"}"); return; }
  JsonDocument d;
  if (deserializeJson(d, server.arg("plain"))) {
    server.send(400, "application/json", "{\"error\":\"invalid JSON\"}"); return;
  }
  String ep = String((const char*)(d["endpoint"] | ""));
  String key = String((const char*)(d["apiKey"] | d["api_key"] | ""));
  String model = String((const char*)(d["model"] | ""));
  ep.trim(); key.trim(); model.trim();
  if (!ep.length()) ep = cfgEndpoint;          // blank keeps saved
  if (!key.length()) key = cfgApiKey;          // blank keeps saved key
  if (!model.length()) model = cfgModel.length() ? cfgModel : "gpt-4o-mini";
  if (!ep.length()) { server.send(400, "application/json", "{\"error\":\"endpoint required\"}"); return; }
  saveAgentOnly(ep, key, model);
  Serial.printf("Saved agent: endpoint=%s model=%s (no reboot, WiFi untouched)\n", ep.c_str(), model.c_str());
  JsonDocument out; out["ok"] = true; out["rebooting"] = false;
  String s; serializeJson(out, s);
  server.send(200, "application/json", s);
  // No reboot: WiFi stays connected, / keeps working.
}

// Proxies the configured endpoint's OpenAI-style model list so the browser
// picker shows what YOUR endpoint serves (works with any provider/app).
void handleGetModels() {
  // Cache 5min: upstream fetch blocks loop ~4s (measured). Picker opens
  // repeatedly while clicking pages, so serve cache instantly.
  static String sCache = "";
  static unsigned long sAt = 0;
  static String sEp = "";
  unsigned long nowM = millis();
  server.sendHeader("Connection", "close");
  server.sendHeader("Cache-Control", "no-store");
  if (sCache.length() && sEp == cfgEndpoint && (nowM - sAt) < 300000) {
    server.send(200, "application/json", sCache);
    return;
  }
  if (!cfgEndpoint.length()) {
    server.send(400, "application/json", "{\"error\":\"agent endpoint not configured. Open /agent.\"}");
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    server.send(503, "application/json", "{\"error\":\"ESP32 has no WiFi. Open /wifi.\"}");
    return;
  }
  String url = modelsUrl(cfgEndpoint);
  bool https = url.startsWith("https://");
  HTTPClient http;
  WiFiClientSecure secure;
  WiFiClient plain;
  http.setTimeout(15000);
  if (https) {
    secure.setInsecure();
    secure.setTimeout(15000);
    if (!http.begin(secure, url)) { server.send(502, "application/json", "{\"error\":\"upstream begin failed\"}"); return; }
  } else {
    plain.setTimeout(15000);
    if (!http.begin(plain, url)) { server.send(502, "application/json", "{\"error\":\"upstream begin failed\"}"); return; }
  }
  http.addHeader("Accept", "application/json");
  if (cfgApiKey.length()) http.addHeader("Authorization", "Bearer " + cfgApiKey);
  http.useHTTP10(true);
  http.setReuse(false);
  int code = http.GET();
  if (code <= 0) {
    String e = http.errorToString(code);
    http.end();
    server.send(502, "application/json", "{\"error\":\"upstream request failed: " + e + "\"}");
    return;
  }
  if (code < 200 || code >= 300) {
    String resp = http.getString();
    http.end();
    resp.replace("\"", "'");
    if (resp.length() > 300) resp = resp.substring(0, 300);
    server.send(502, "application/json", "{\"error\":\"upstream HTTP " + String(code) + ": " + resp + "\"}");
    return;
  }
  JsonDocument out;
  JsonArray arr = out["models"].to<JsonArray>();
  // Model catalogs (400+ entries with pricing) are far too big to hold in RAM,
  // so scan the stream for "id" values and stop after 60 — flat ~1KB RAM.
  WiFiClient* st = http.getStreamPtr();
  String win;
  win.reserve(320);
  bool inData = false;
  size_t total = 0;
  unsigned long deadline = millis() + 15000;
  uint8_t buf[256];
  while ((int)arr.size() < 60 && total < 98304 && (long)(millis() - deadline) < 0) {
    if (!st->available()) {
      if (!http.connected()) break;
      delay(5);
      continue;
    }
    int n = st->read(buf, sizeof(buf));
    if (n <= 0) { delay(5); continue; }
    total += (size_t)n;
    for (int i = 0; i < n; i++) {
      win += (char)buf[i];
      if (win.length() > 300) win.remove(0, win.length() - 300);
    }
    if (!inData) {
      if (win.indexOf("\"data\"") < 0) continue;
      inData = true;
    }
    int p;
    while ((p = win.indexOf("\"id\"")) >= 0) {
      int q = p + 4;
      while (q < (int)win.length() && (win[q] == ' ' || win[q] == '\t')) q++;
      if (q >= (int)win.length() || win[q] != ':') break;  // need more bytes
      q++;
      while (q < (int)win.length() && (win[q] == ' ' || win[q] == '\t')) q++;
      if (q >= (int)win.length() || win[q] != '"') break;  // need more bytes
      int e = win.indexOf('"', q + 1);
      if (e < 0) break;  // value split across chunks, wait for more
      String id = win.substring(q + 1, e);
      win.remove(0, e + 1);
      if (id.length() && id.length() <= 160) {
        JsonObject o = arr.add<JsonObject>();
        o["id"] = id;
        if ((int)arr.size() >= 60) break;
      }
    }
  }
  http.end();
  if (arr.size() == 0) {
    server.send(502, "application/json", "{\"error\":\"no models parsed from endpoint\"}");
    return;
  }
  out["count"] = (int)arr.size();
  String s; serializeJson(out, s);
  sCache = s; sAt = millis(); sEp = cfgEndpoint;
  server.send(200, "application/json", s);
}

void handleGetConfig() {
  JsonDocument d;
  d["wifi_ssid"] = cfgSsid;
  d["endpoint"] = cfgEndpoint;
  d["model"] = cfgModel;
  d["api_key_masked"] = maskedKey(cfgApiKey);
  String s; serializeJson(d, s);
  server.send(200, "application/json", s);
}

void handleSaveConfig() {
  // Legacy combined endpoint — kept for compat, now safe:
  // blank password keeps saved WiFi password, blank key keeps saved API key, no WiFi wipe.
  if (!server.hasArg("plain")) { server.send(400, "application/json", "{\"error\":\"empty body\"}"); return; }
  JsonDocument d;
  if (deserializeJson(d, server.arg("plain"))) {
    server.send(400, "application/json", "{\"error\":\"invalid JSON\"}"); return;
  }
  bool hasWifi = d["ssid"].is<const char*>() || d["password"].is<const char*>();
  bool hasAgent = d["endpoint"].is<const char*>() || d["apiKey"].is<const char*>() || d["api_key"].is<const char*>() || d["model"].is<const char*>();
  if (hasWifi && !hasAgent) { handleSaveWifi(); return; }
  if (hasAgent && !hasWifi) { handleSaveAgent(); return; }
  String ssid = String((const char*)(d["ssid"] | ""));
  String pass = String((const char*)(d["password"] | ""));
  String ep = String((const char*)(d["endpoint"] | ""));
  String key = String((const char*)(d["apiKey"] | d["api_key"] | ""));
  String model = String((const char*)(d["model"] | ""));
  ssid.trim(); ep.trim(); key.trim(); model.trim();
  if (!ssid.length()) ssid = cfgSsid;
  if (!pass.length() && ssid == cfgSsid) pass = cfgPass;
  if (!key.length()) key = cfgApiKey;
  if (!ep.length()) ep = cfgEndpoint;
  if (!model.length()) model = cfgModel;
  if (!ssid.length() && !ep.length()) {
    server.send(400, "application/json", "{\"error\":\"SSID or endpoint required\"}"); return;
  }
  if (!ssid.length()) ssid = cfgSsid, pass = cfgPass;
  saveConfig(ssid, pass, ep, key, model);
  Serial.printf("Saved (legacy): ssid=%s endpoint=%s model=%s\n", ssid.c_str(), ep.c_str(), model.c_str());
  server.send(200, "application/json", "{\"ok\":true,\"rebooting\":true}");
  delay(800);
  ESP.restart();
}

void handleDebugRaw() {
  // Uses the ESP's saved key to hit the saved endpoint and returns the RAW upstream
  // body (truncated) so we can see the exact JSON shape without exposing the key.
  String msg = server.hasArg("message") ? server.arg("message") : "Reply with just OK";
  if (msg.length() > 500) msg = msg.substring(0, 500);
  String url = chatCompletionsUrl(cfgEndpoint);
  JsonDocument req;
  req["model"] = cfgModel;
  req["temperature"] = 0.7;
  req["max_tokens"] = 800;  // bound reply size for ESP32 RAM + avoids chunked giant bodies
  req["stream"] = false;
  JsonArray msgs = req["messages"].to<JsonArray>();
  JsonObject sys = msgs.add<JsonObject>();
  sys["role"] = "system";
  sys["content"] = "You are a helpful, friendly assistant. Respond in the level of detail the user requests. Be clear, accurate, and natural. Ask clarifying questions when needed. If you don't know something, say so.";
  JsonObject u = msgs.add<JsonObject>();
  u["role"] = "user"; u["content"] = msg;
  String body; serializeJson(req, body);
  bool https = url.startsWith("https://");
  HTTPClient http; WiFiClientSecure secure; WiFiClient plain;
  http.setTimeout(60000);
  if (https) { secure.setInsecure(); secure.setTimeout(60000); http.begin(secure, url); }
  else { plain.setTimeout(60000); http.begin(plain, url); }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  http.addHeader("HTTP-Referer", "http://esp32-agent.local");
  http.addHeader("X-Title", "ESP32-Agent");
  if (cfgApiKey.length()) http.addHeader("Authorization", "Bearer " + cfgApiKey);
  http.useHTTP10(true);
  http.setReuse(false);
  int code = http.POST(body);
  String resp = (code > 0) ? http.getString() : http.errorToString(code);
  int len = resp.length();
  http.end();
  JsonDocument out;
  out["url"] = url;
  out["model"] = cfgModel;
  out["http_code"] = code;
  out["len"] = len;
  out["raw"] = resp.substring(0, 2500);
  // Also report what the parser sees:
  JsonDocument res;
  if (deserializeJson(res, resp)) out["parse"] = "json-error";
  else {
    out["choices"] = res["choices"].size();
    JsonObject m = res["choices"][0]["message"].as<JsonObject>();
    String keys;
    for (JsonPair kv : m) { keys += String(kv.key().c_str()) + ","; }
    out["msg_keys"] = keys;
    const char* c = m["content"] | "";
    out["content_len"] = c ? strlen(c) : -1;
    out["content_preview"] = String(c ? c : "").substring(0, 300);
    const char* r1 = m["reasoning_content"] | "";
    const char* r2 = m["reasoning"] | "";
    out["reasoning_content_len"] = r1 ? strlen(r1) : -1;
    out["reasoning_len"] = r2 ? strlen(r2) : -1;
    out["reasoning_preview"] = String(r2 && r2[0] ? r2 : (r1 && r1[0] ? r1 : "")).substring(0, 300);
    out["raw2"] = resp.substring(2500, 5000);
  }
  String s; serializeJson(out, s);
  server.send(200, "application/json", s);
}

void handleChat() {
  if (!server.hasArg("plain")) { server.send(400, "application/json", "{\"error\":\"empty body\"}"); return; }
  JsonDocument d;
  if (deserializeJson(d, server.arg("plain"))) {
    server.send(400, "application/json", "{\"error\":\"invalid JSON\"}"); return;
  }
  String msg = String((const char*)(d["message"] | ""));
  msg.trim();
  if (!msg.length()) { server.send(400, "application/json", "{\"error\":\"message required\"}"); return; }
  if (msg.length() > 2000) msg = msg.substring(0, 2000);
  if (WiFi.status() != WL_CONNECTED) {
    server.send(503, "application/json", "{\"error\":\"ESP32 has no WiFi. Open /wifi.\"}"); return;
  }
  JsonArray hist = d["history"].as<JsonArray>();  // may be null -> handled in callLLM
  JsonDocument empty;
  String reasoning, toolsUsed;
  String reply = callLLMWithTools(msg, hist.isNull() ? empty.to<JsonArray>() : hist, &reasoning, &toolsUsed);
  JsonDocument out;
  if (reply.startsWith("ERROR:")) {
    out["error"] = reply;
    String s; serializeJson(out, s);
    int code = reply.indexOf("HTTP 4") > 0 ? 502 : 500;
    if (reply.indexOf("not configured") > 0) code = 400;
    server.send(code, "application/json", s);
  } else {
    out["reply"] = reply;
    out["model"] = cfgModel;
    if (reasoning.length()) out["reasoning"] = reasoning.substring(0, 3000);
    if (toolsUsed.length()) out["tools_used"] = toolsUsed;
    String s; serializeJson(out, s);
    server.send(200, "application/json", s);
    if (gRestartPending) { delay(600); ESP.restart(); }
  }
  Serial.printf("[chat] q=%d chars -> a=%d chars tools=[%s]\n", msg.length(), reply.length(), toolsUsed.c_str());
}

// True SSE streaming: browser POSTs here, ESP opens upstream with stream:true
// over a raw socket (HTTP/1.1, manual chunked decode) and forwards each delta
// to the browser as text/event-stream. Low RAM: deltas forwarded, never buffered.
void handleChatStream() {
  if (!server.hasArg("plain")) { server.send(400, "application/json", "{\"error\":\"empty body\"}"); return; }
  JsonDocument d;
  if (deserializeJson(d, server.arg("plain"))) {
    server.send(400, "application/json", "{\"error\":\"invalid JSON\"}"); return;
  }
  String msg = String((const char*)(d["message"] | ""));
  msg.trim();
  if (!msg.length()) { server.send(400, "application/json", "{\"error\":\"message required\"}"); return; }
  if (msg.length() > 2000) msg = msg.substring(0, 2000);
  if (WiFi.status() != WL_CONNECTED) {
    server.send(503, "application/json", "{\"error\":\"ESP32 has no WiFi. Open /wifi.\"}"); return;
  }
  if (!cfgEndpoint.length()) {
    server.send(400, "application/json", "{\"error\":\"agent endpoint not configured. Open /agent.\"}"); return;
  }
  JsonArray hist = d["history"].as<JsonArray>();

  // Build upstream request with stream:true (same context + enabled tools).
  JsonDocument req;
  req["model"] = cfgModel;
  req["temperature"] = 0.7;
  req["max_tokens"] = 800;
  req["stream"] = true;
  JsonArray msgs = req["messages"].to<JsonArray>();
  JsonObject sys = msgs.add<JsonObject>();
  sys["role"] = "system";
  sys["content"] = buildSystemPrompt();
  injectRecentLog(msgs, msg, 8);
  if (!hist.isNull()) {
    int count = 0;
    for (JsonObject m : hist) {
      if (count++ >= 16) break;
      const char* r = m["role"] | "";
      const char* c = m["content"] | "";
      if (!r || !c || !c[0]) continue;
      String role = String(r), content = String(c);
      if (role != "user" && role != "assistant" && role != "system") continue;
      if (content.length() > 1500) content = content.substring(0, 1500);
      JsonObject o = msgs.add<JsonObject>();
      o["role"] = role; o["content"] = content;
    }
  }
  JsonObject u = msgs.add<JsonObject>();
  u["role"] = "user";
  u["content"] = msg.substring(0,1500);
  addToolsArray(req);
  String body;
  serializeJson(req, body);

  String full = chatCompletionsUrl(cfgEndpoint);
  bool isHttps = full.startsWith("https://");
  String rest = full.substring(isHttps ? 8 : 7);
  int slash = rest.indexOf('/');
  String host = slash > 0 ? rest.substring(0, slash) : rest;
  String path = slash > 0 ? rest.substring(slash) : "/";
  int port = isHttps ? 443 : 80;
  int colon = host.indexOf(':');
  if (colon > 0) { port = host.substring(colon + 1).toInt(); host = host.substring(0, colon); }

  WiFiClientSecure secure;
  WiFiClient plain;
  Client* up = isHttps ? (Client*)&secure : (Client*)&plain;
  if (isHttps) { secure.setInsecure(); secure.setTimeout(15000); }
  else { plain.setTimeout(15000); }
  Serial.printf("[stream] %s%s model=%s q=%dB\n", host.c_str(), path.c_str(), cfgModel.c_str(), msg.length());
  if (!up->connect(host.c_str(), port)) {
    server.send(502, "application/json", "{\"error\":\"upstream connect failed\"}");
    return;
  }
  up->printf("POST %s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/json\r\nAccept: text/event-stream\r\nAuthorization: Bearer %s\r\nHTTP-Referer: http://esp32-agent.local\r\nX-Title: Agent\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",
    path.c_str(), host.c_str(), cfgApiKey.c_str(), body.length());
  up->print(body);

  up->setTimeout(20000);
  String status = up->readStringUntil('\n');
  int scode = (status.startsWith("HTTP/")) ? status.substring(9, 12).toInt() : 0;
  bool chunked = false;
  while (true) {
    String h = up->readStringUntil('\n');
    h.trim();
    if (!h.length()) break;
    String hl = h;
    hl.toLowerCase();
    if (hl.startsWith("transfer-encoding:") && hl.indexOf("chunked") >= 0) chunked = true;
  }
  if (scode < 200 || scode >= 300) {
    String eb;
    unsigned long t0 = millis();
    if (chunked) {
      // Dechunk the error body (usually a single small chunk).
      while ((up->connected() || up->available()) && eb.length() < 1200 && millis() - t0 < 8000) {
        String szLine = up->readStringUntil('\n');
        szLine.trim();
        if (!szLine.length()) continue;
        int sc2 = szLine.indexOf(';');
        if (sc2 >= 0) szLine = szLine.substring(0, sc2);
        long sz = strtol(szLine.c_str(), nullptr, 16);
        if (sz <= 0) break;
        if (sz > 1200) sz = 1200;
        long got = 0;
        while (got < sz && millis() - t0 < 8000) {
          while (up->available() && got < sz) { eb += (char)up->read(); got++; }
          if (got < sz) delay(2);
        }
        up->readStringUntil('\n');
        break;
      }
    } else {
      while ((up->connected() || up->available()) && eb.length() < 1200 && millis() - t0 < 8000) {
        while (up->available() && eb.length() < 1200) eb += (char)up->read();
        delay(2);
      }
    }
    up->stop();
    eb.replace("\"", "'");
    if (eb.length() > 600) eb = eb.substring(0, 600);
    server.send(502, "application/json", "{\"error\":\"upstream HTTP " + String(scode) + ": " + eb + "\"}");
    return;
  }

  // Upstream OK: start SSE to the browser.
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/event-stream", "");
  auto sendEvt = [&](const String& payload) {
    server.sendContent("data: " + payload + "\n\n");
  };

  String sseBuf;
  sseBuf.reserve(2048);
  size_t totalChars = 0;
  size_t contentChars = 0;  // text deltas only (reasoning doesn't count)
  unsigned long start = millis();
  unsigned long lastBytes = millis();
  bool done = false;
  bool sawTools = false;
  int dbgChunks = 0, dbgEvents = 0;

  // Feed one raw byte-run into the SSE line buffer; returns false to abort.
  auto feed = [&](const String& chunk) -> bool {
    sseBuf += chunk;
    if (sseBuf.length() > 4096) sseBuf = sseBuf.substring(sseBuf.length() - 4096);  // safety trim
    int idx;
    while ((idx = sseBuf.indexOf("\n\n")) >= 0) {
      String evt = sseBuf.substring(0, idx);
      sseBuf = sseBuf.substring(idx + 2);
      int pos = 0;
      while (pos < (int)evt.length()) {
        int nl = evt.indexOf('\n', pos);
        String ln = (nl < 0) ? evt.substring(pos) : evt.substring(pos, nl);
        if (nl < 0) pos = evt.length(); else pos = nl + 1;
        ln.trim();
        if (!ln.startsWith("data:")) continue;
        String payload = ln.substring(5);
        payload.trim();
        if (payload == "[DONE]") { done = true; return false; }
        dbgEvents++;
        JsonDocument sd;
        if (deserializeJson(sd, payload)) { if (dbgEvents <= 3) Serial.printf("[stream] evt%d parse-fail len=%d\n", dbgEvents, payload.length()); continue; }
        if (dbgEvents <= 4) {
          String pv = payload.substring(0, 160);
          pv.replace("\n", " ");
          Serial.printf("[stream] evt%d: %s\n", dbgEvents, pv.c_str());
        }
        // Debug: report delta shape (first 6 events only to save log spam).
        if (dbgEvents <= 6) {
          JsonObject dl = sd["choices"][0]["delta"].as<JsonObject>();
          String dkeys;
          for (JsonPair kv : dl) dkeys += String(kv.key().c_str()) + ",";
          const char* dc = dl["content"] | "";
          const char* dr = dl["reasoning"] | "";
          const char* drc = dl["reasoning_content"] | "";
          Serial.printf("[stream] evt%d delta keys=[%s] content=%d reasoning=%d rcontent=%d\n",
            dbgEvents, dkeys.c_str(), dc ? (int)strlen(dc) : -1, dr ? (int)strlen(dr) : -1, drc ? (int)strlen(drc) : -1);
        }
        const char* delta = sd["choices"][0]["delta"]["content"] | (const char*)nullptr;
        if (!delta || !delta[0]) delta = sd["choices"][0]["message"]["content"] | (const char*)nullptr;
        const char* rDelta = sd["choices"][0]["delta"]["reasoning"] | (const char*)nullptr;
        if (!rDelta || !rDelta[0]) rDelta = sd["choices"][0]["delta"]["reasoning_content"] | (const char*)nullptr;
        // tool_calls in streaming deltas -> signal browser to fallback to /api/chat (full agent loop)
        if (!sd["choices"][0]["delta"]["tool_calls"].isNull()) sawTools = true;
        if (!sd["choices"][0]["delta"]["function_call"].isNull()) sawTools = true;
        if (delta && delta[0]) {
          JsonDocument o;
          o["delta"] = delta;
          String js;
          serializeJson(o, js);
          sendEvt(js);
          totalChars += strlen(delta);
          contentChars += strlen(delta);
        }
        if (rDelta && rDelta[0]) {
          JsonDocument o;
          o["reasoning"] = rDelta;
          String js;
          serializeJson(o, js);
          sendEvt(js);
          totalChars += strlen(rDelta);
        }
      }
      if (!server.client().connected()) return false;
    }
    return true;
  };

  if (chunked) {
    while (!done && millis() - start < 120000 && millis() - lastBytes < 30000) {
      if (!server.client().connected()) break;
      String szLine = up->readStringUntil('\n');
      if (!szLine.length() && !up->connected() && !up->available()) break;
      szLine.trim();
      if (!szLine.length()) { delay(2); continue; }
      int sc = szLine.indexOf(';');
      if (sc >= 0) szLine = szLine.substring(0, sc);
      long sz = strtol(szLine.c_str(), nullptr, 16);
      if (sz <= 0) {  // final chunk: consume trailers then finish
        String trailer;
        while (true) { trailer = up->readStringUntil('\n'); trailer.trim(); if (!trailer.length()) break; }
        break;
      }
      if (sz > 8192) sz = 8192;
      String chunk;
      chunk.reserve(sz);
      long got = 0;
      while (got < sz && millis() - lastBytes < 30000) {
        while (up->available() && got < sz) { chunk += (char)up->read(); got++; }
        if (got < sz) delay(2); else break;
      }
      up->readStringUntil('\n');  // trailing CRLF
      lastBytes = millis();
      if (chunk.length()) { dbgChunks++; if (!feed(chunk)) break; }
      delay(1);
    }
  } else {
    while (!done && millis() - start < 120000 && millis() - lastBytes < 30000) {
      if (!server.client().connected()) break;
      bool progress = false;
      while (up->available()) {
        String chunk;
        int n = 0;
        while (up->available() && n < 1024) { chunk += (char)up->read(); n++; progress = true; }
        if (chunk.length()) { if (!feed(chunk)) { progress = true; break; } }
      }
      if (progress) lastBytes = millis();
      if (!up->connected() && !up->available()) break;
      delay(2);
    }
  }
  up->stop();
  // Harness rule: stream is only "done" if real text arrived. Reasoning-only
  // (tool intent, no content — some providers don't stream tool_calls) means
  // the browser must fall back to /api/chat which runs the full agent loop.
  if (sawTools || contentChars == 0) sendEvt("{\"need_tools\":true}");
  sendEvt("[DONE]");
  Serial.printf("[stream] done, ~%u chars forwarded, chunks=%d events=%d tools=%d\n", (unsigned)totalChars, dbgChunks, dbgEvents, sawTools ? 1 : 0);
}

void handleReset() {
  server.send(200, "application/json", "{\"ok\":true,\"rebooting\":true}");
  delay(500);
  if (gFsOk) { LittleFS.remove(LOG_PATH); LittleFS.remove(MEM_PATH); }
  prefs.begin(PREFS_NS, false);
  prefs.clear();
  prefs.end();
  ESP.restart();
}

void setupRoutes() {
  server.on("/", HTTP_GET, [] {
    if (apMode && cfgSsid.length() == 0) {  // fresh device -> straight to WiFi screen
      server.sendHeader("Location", "/wifi");
      server.send(302, "text/plain", "go to /wifi");
      return;
    }
    server.send_P(200, "text/html", CHAT_HTML);
  });
  server.on("/setup", HTTP_GET, [] { server.send_P(200, "text/html", SETUP_HTML); });
  server.on("/wifi", HTTP_GET, [] { server.send_P(200, "text/html", WIFI_HTML); });
  server.on("/agent", HTTP_GET, [] { server.send_P(200, "text/html", AGENT_HTML); });
  server.on("/update", HTTP_GET, [] { server.send_P(200, "text/html", UPDATE_HTML); });
  server.on("/functions", HTTP_GET, [] { server.send_P(200, "text/html", FUNCS_HTML); });
  server.on("/docs", HTTP_GET, [] { server.send_P(200, "text/html", DOCS_HTML); });
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/scan", HTTP_GET, handleScan);
  server.on("/api/wifi", HTTP_GET, handleGetWifi);
  server.on("/api/wifi", HTTP_POST, handleSaveWifi);
  server.on("/api/agent", HTTP_GET, handleGetAgent);
  server.on("/api/agent", HTTP_POST, handleSaveAgent);
  server.on("/api/models", HTTP_GET, handleGetModels);
  server.on("/api/config", HTTP_GET, handleGetConfig);
  server.on("/api/config", HTTP_POST, handleSaveConfig);
  server.on("/api/chat", HTTP_POST, handleChat);
  server.on("/api/chat-stream", HTTP_POST, handleChatStream);
  server.on("/api/debug-raw", HTTP_GET, handleDebugRaw);
  server.on("/api/tools", HTTP_GET, handleGetTools);
  server.on("/api/tools", HTTP_POST, handleSaveTools);
  server.on("/api/memory", HTTP_GET, handleGetMemory);
  server.on("/api/memory", HTTP_POST, handleSaveMemory);
  server.on("/api/reset", HTTP_POST, handleReset);
  // HTTP OTA upload (browser .bin upload, no cable)
  server.on("/update", HTTP_POST,
    [] {
      bool ok = !Update.hasError();
      server.send(ok ? 200 : 500, "text/plain", ok ? "OK - rebooting..." : "FAIL");
      delay(500);
      ESP.restart();
    },
    [] {
      HTTPUpload& up = server.upload();
      if (up.status == UPLOAD_FILE_START) {
        Serial.printf("OTA start: %s\n", up.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Serial.println("Update.begin failed");
      } else if (up.status == UPLOAD_FILE_WRITE) {
        if (Update.write(up.buf, up.currentSize) != up.currentSize) Serial.println("Update.write failed");
      } else if (up.status == UPLOAD_FILE_END) {
        if (Update.end(true)) Serial.printf("OTA done: %u bytes\n", up.totalSize);
        else Serial.println("Update.end failed");
      }
    });
  server.onNotFound([] {
    // Captive portal: in AP mode, redirect everything unknown to /wifi.
    if (apMode && (server.hostHeader() != "192.168.4.1")) {
      server.sendHeader("Location", String("http://192.168.4.1/wifi"), true);
      server.send(302, "text/plain", "");
      return;
    }
    server.send(404, "text/plain", "Not found. Try /, /setup, /wifi, /agent, /functions, /docs, /update, /api/status");
  });
}
