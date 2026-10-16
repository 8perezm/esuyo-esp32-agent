#include "agent_tools.h"
#include "config.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

const char* TOOL_NAMES[] = {
  "get_status", "gpio_write", "gpio_read", "adc_read",
  "wifi_scan", "mem_save", "mem_read", "restart_device", "blink",
  "web_fetch"
};
const int TOOL_COUNT = 10;

bool gFsOk = false;
bool gRestartPending = false;
const char* LOG_PATH = "/chatlog.jsonl";
const char* MEM_PATH = "/memory.txt";
const size_t LOG_MAX_BYTES = 32768;
const size_t MEM_MAX_BYTES = 4096;

int toolIndex(const String& n) {
  for (int i = 0; i < TOOL_COUNT; i++) if (n == TOOL_NAMES[i]) return i;
  return -1;
}
bool toolEnabled(const String& n) {
  int i = toolIndex(n);
  if (i < 0) return false;
  return (gToolsMask >> i) & 1U;
}
bool toolEnabledIdx(int i) { return (gToolsMask >> i) & 1U; }

void initAgentFS() {
  gFsOk = LittleFS.begin(true);  // format on first boot
  Serial.printf("[fs] LittleFS %s\n", gFsOk ? "ok" : "FAIL");
}

String readMemoryFacts() {
  if (!gFsOk) return "";
  File f = LittleFS.open(MEM_PATH, "r");
  if (!f) return "";
  String s = f.readString();
  f.close();
  if (s.length() > MEM_MAX_BYTES) s = s.substring(s.length() - MEM_MAX_BYTES);
  return s;
}

bool appendMemoryFact(const String& text) {
  if (!gFsOk || !text.length()) return false;
  String cur = readMemoryFacts();
  // append one line; drop oldest lines if over cap (rare rewrite -> low wear)
  String line = text;
  line.trim();
  line.replace("\n", " ");
  if (line.length() > 500) line = line.substring(0, 500);
  if (!line.length()) return false;
  cur += "- " + line + "\n";
  while (cur.length() > MEM_MAX_BYTES) {
    int nl = cur.indexOf('\n');
    if (nl < 0) { cur = cur.substring(cur.length() - MEM_MAX_BYTES); break; }
    cur = cur.substring(nl + 1);
  }
  File f = LittleFS.open(MEM_PATH, "w");
  if (!f) return false;
  f.print(cur);
  f.close();
  return true;
}

// Append one turn (user+assistant) in a single open/close = minimal flash wear.
// JSONL with short keys to save space. Called ONCE per completed turn, never per token.
bool appendChatLog(const String& user, const String& asst) {
  if (!gFsOk) return false;
  // rotation: if oversize, keep last half (rare, ~every few hundred turns)
  if (LittleFS.exists(LOG_PATH)) {
    File f = LittleFS.open(LOG_PATH, "r");
    size_t sz = f ? f.size() : 0;
    if (f) f.close();
    if (sz > LOG_MAX_BYTES) {
      File r = LittleFS.open(LOG_PATH, "r");
      String all = r ? r.readString() : "";
      if (r) r.close();
      // keep last ~16KB
      if (all.length() > 16384) all = all.substring(all.length() - 16384);
      int nl = all.indexOf('\n');
      if (nl >= 0) all = all.substring(nl + 1);  // align to line start
      File w = LittleFS.open(LOG_PATH, "w");
      if (w) { w.print(all); w.close(); }
    }
  }
  File f = LittleFS.open(LOG_PATH, "a");
  if (!f) return false;
  JsonDocument a, b;
  a["r"] = "user"; a["c"] = user.substring(0, 1500);
  b["r"] = "assistant"; b["c"] = asst.substring(0, 1500);
  serializeJson(a, f); f.println();
  serializeJson(b, f); f.println();
  f.close();
  return true;
}

// Inject last N persisted turns into msgs (reads only, no wear).
// Skips exact duplicate of current user msg to avoid double context.
int injectRecentLog(JsonArray msgs, const String& curUser, int maxMsgs) {
  if (!gFsOk || !LittleFS.exists(LOG_PATH)) return 0;
  File f = LittleFS.open(LOG_PATH, "r");
  if (!f) return 0;
  // Read tail: file capped at 32KB so full read is fine (~200KB heap free).
  String all = f.readString();
  f.close();
  // split into lines, keep last maxMsgs with role/content
  int total = 0;
  for (unsigned i = 0; i < all.length(); i++) if (all[i] == '\n') total++;
  int startLine = total > maxMsgs ? total - maxMsgs : 0;
  int line = 0, added = 0;
  int pos = 0;
  while (pos < (int)all.length()) {
    int nl = all.indexOf('\n', pos);
    String ln = nl < 0 ? all.substring(pos) : all.substring(pos, nl);
    if (nl < 0) pos = all.length(); else pos = nl + 1;
    if (line++ < startLine) continue;
    ln.trim();
    if (!ln.length()) continue;
    JsonDocument d;
    if (deserializeJson(d, ln)) continue;
    const char* r = d["r"] | "";
    const char* c = d["c"] | "";
    if (!r[0] || !c[0]) continue;
    if (String(r) == "user" && String(c) == curUser) continue;  // avoid dup
    JsonObject o = msgs.add<JsonObject>();
    o["role"] = r; o["content"] = String(c).substring(0, 1500);
    added++;
  }
  return added;
}

bool pinCanWrite(int p) {
  const int ok[] = {2,4,5,12,13,14,15,16,17,18,19,21,22,23,25,26,27,32,33};
  for (unsigned i = 0; i < sizeof(ok)/sizeof(ok[0]); i++) if (p == ok[i]) return true;
  return false;
}
bool pinCanRead(int p) {
  if (pinCanWrite(p)) return true;
  return (p==34||p==35||p==36||p==39);
}
bool pinCanADC(int p) { return (p==32||p==33||p==34||p==35||p==36||p==39); }

// Build system prompt from ONLY enabled tools -> conserves tokens.
String buildSystemPrompt() {
  String s = "You are ESP32-Agent on esp32-agent.local (ESP32, low RAM). Be brief, plain text. ";
  s += "Enabled device tools: ";
  bool first = true;
  int n = 0;
  for (int i = 0; i < TOOL_COUNT; i++) if (toolEnabledIdx(i)) {
    if (!first) s += ", ";
    s += TOOL_NAMES[i];
    first = false; n++;
  }
  if (!n) s += "(none - answer from knowledge only)";
  s += ". Use a tool when the user asks about device state, pins, sensors, wifi, memory, restart, or fresh web info - never guess pin levels or current events. Always finish with a short plain-text reply summarizing the tool results (never leave the reply empty after tools).";
  String mem = readMemoryFacts();
  if (mem.length()) {
    s += " Long-term memory:\n" + mem.substring(0, 1200);
  }
  // tiny live snapshot (cheap, no tool call needed for trivial questions)
  char snap[160];
  snprintf(snap, sizeof(snap), " [dev ip=%s up=%lus heap=%u rssi=%d]",
    WiFi.status()==WL_CONNECTED ? WiFi.localIP().toString().c_str() : "ap",
    (unsigned long)millis()/1000, (unsigned)ESP.getFreeHeap(),
    WiFi.status()==WL_CONNECTED ? WiFi.RSSI() : 0);
  s += snap;
  return s;
}

// Add OpenAI-compatible tools array with ONLY enabled functions.
void addToolsArray(JsonDocument& req) {
  JsonArray tools = req["tools"].to<JsonArray>();
  if (toolEnabled("get_status")) {
    JsonObject t = tools.add<JsonObject>();
    t["type"] = "function";
    JsonObject fn = t["function"].to<JsonObject>();
    fn["name"] = "get_status";
    fn["description"] = "Get ESP32 status: heap, uptime, wifi, ip, model";
    JsonObject p = fn["parameters"].to<JsonObject>();
    p["type"] = "object";
    p["properties"].to<JsonObject>();
  }
  if (toolEnabled("gpio_write")) {
    JsonObject t = tools.add<JsonObject>();
    t["type"] = "function";
    JsonObject fn = t["function"].to<JsonObject>();
    fn["name"] = "gpio_write";
    fn["description"] = "Set GPIO output level. Allowed pins 2,4,5,12-19,21-23,25-27,32,33.";
    JsonObject p = fn["parameters"].to<JsonObject>();
    p["type"] = "object";
    JsonObject pr = p["properties"].to<JsonObject>();
    pr["pin"].to<JsonObject>()["type"] = "integer";
    pr["level"].to<JsonObject>()["type"] = "integer";
    JsonArray r = p["required"].to<JsonArray>();
    r.add("pin"); r.add("level");
  }
  if (toolEnabled("gpio_read")) {
    JsonObject t = tools.add<JsonObject>();
    t["type"] = "function";
    JsonObject fn = t["function"].to<JsonObject>();
    fn["name"] = "gpio_read";
    fn["description"] = "Read GPIO level.";
    JsonObject p = fn["parameters"].to<JsonObject>();
    p["type"] = "object";
    JsonObject pr = p["properties"].to<JsonObject>();
    pr["pin"].to<JsonObject>()["type"] = "integer";
    JsonArray r = p["required"].to<JsonArray>();
    r.add("pin");
  }
  if (toolEnabled("adc_read")) {
    JsonObject t = tools.add<JsonObject>();
    t["type"] = "function";
    JsonObject fn = t["function"].to<JsonObject>();
    fn["name"] = "adc_read";
    fn["description"] = "Analog read. Best pins 32,33,34,35,36,39.";
    JsonObject p = fn["parameters"].to<JsonObject>();
    p["type"] = "object";
    JsonObject pr = p["properties"].to<JsonObject>();
    pr["pin"].to<JsonObject>()["type"] = "integer";
    JsonArray r = p["required"].to<JsonArray>();
    r.add("pin");
  }
  if (toolEnabled("wifi_scan")) {
    JsonObject t = tools.add<JsonObject>();
    t["type"] = "function";
    JsonObject fn = t["function"].to<JsonObject>();
    fn["name"] = "wifi_scan";
    fn["description"] = "Scan nearby WiFi networks (takes ~2s).";
    JsonObject p = fn["parameters"].to<JsonObject>();
    p["type"] = "object";
    p["properties"].to<JsonObject>();
  }
  if (toolEnabled("mem_save")) {
    JsonObject t = tools.add<JsonObject>();
    t["type"] = "function";
    JsonObject fn = t["function"].to<JsonObject>();
    fn["name"] = "mem_save";
    fn["description"] = "Save a durable user preference/fact (<=500 chars).";
    JsonObject p = fn["parameters"].to<JsonObject>();
    p["type"] = "object";
    JsonObject pr = p["properties"].to<JsonObject>();
    pr["text"].to<JsonObject>()["type"] = "string";
    JsonArray r = p["required"].to<JsonArray>();
    r.add("text");
  }
  if (toolEnabled("mem_read")) {
    JsonObject t = tools.add<JsonObject>();
    t["type"] = "function";
    JsonObject fn = t["function"].to<JsonObject>();
    fn["name"] = "mem_read";
    fn["description"] = "Read saved memory facts.";
    JsonObject p = fn["parameters"].to<JsonObject>();
    p["type"] = "object";
    p["properties"].to<JsonObject>();
  }
  if (toolEnabled("restart_device")) {
    JsonObject t = tools.add<JsonObject>();
    t["type"] = "function";
    JsonObject fn = t["function"].to<JsonObject>();
    fn["name"] = "restart_device";
    fn["description"] = "Restart ESP32. Only when user explicitly asks. Requires confirm=true.";
    JsonObject p = fn["parameters"].to<JsonObject>();
    p["type"] = "object";
    JsonObject pr = p["properties"].to<JsonObject>();
    pr["confirm"].to<JsonObject>()["type"] = "boolean";
    JsonArray r = p["required"].to<JsonArray>();
    r.add("confirm");
  }
  if (toolEnabled("blink")) {
    JsonObject t = tools.add<JsonObject>();
    t["type"] = "function";
    JsonObject fn = t["function"].to<JsonObject>();
    fn["name"] = "blink";
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
  if (toolEnabled("web_fetch")) {
    JsonObject t = tools.add<JsonObject>();
    t["type"] = "function";
    JsonObject fn = t["function"].to<JsonObject>();
    fn["name"] = "web_fetch";
    fn["description"] = "Fetch an http(s) URL and return its text (up to ~2000 chars). Use for current info, docs, prices, headlines. No JS, no login pages.";
    JsonObject p = fn["parameters"].to<JsonObject>();
    p["type"] = "object";
    JsonObject pr = p["properties"].to<JsonObject>();
    pr["url"].to<JsonObject>()["type"] = "string";
    JsonArray r = p["required"].to<JsonArray>();
    r.add("url");
  }
  if (tools.size() > 0) req["tool_choice"] = "auto";
}

// Strip HTML down to readable text (removes scripts/styles/tags, decodes entities).
static String stripHtmlToText(const String& html, size_t maxChars) {
  String t = html;
  // Remove <script...>...</script> and <style...>...</style> (case-insensitive via lower copy).
  {
    String low = t;
    low.toLowerCase();
    const char* tags[] = {"script", "style"};
    for (int k = 0; k < 2; k++) {
      String open = String("<") + tags[k];
      String close = String("</") + tags[k];
      while (true) {
        int s = low.indexOf(open);
        if (s < 0) break;
        int ce = low.indexOf(close, s + open.length());
        if (ce < 0) { t.remove(s); low.remove(s); break; }
        int ge = low.indexOf('>', ce + close.length());
        if (ge < 0) ge = low.length() - 1;
        t.remove(s, ge - s + 1);
        low.remove(s, ge - s + 1);
      }
    }
  }
  // Strip remaining tags (each tag becomes a space to avoid word joins).
  String noTag;
  noTag.reserve(t.length() / 2 + 64);
  bool inTag = false;
  for (unsigned i = 0; i < t.length(); i++) {
    char c = t[i];
    if (!inTag && c == '<') { inTag = true; continue; }
    if (inTag) { if (c == '>') { inTag = false; noTag += ' '; } continue; }
    noTag += c;
  }
  noTag.replace("&nbsp;", " ");
  noTag.replace("&amp;", "&");
  noTag.replace("&lt;", "<");
  noTag.replace("&gt;", ">");
  noTag.replace("&quot;", "\"");
  noTag.replace("&#39;", "'");
  noTag.replace("&#x27;", "'");
  // Collapse whitespace.
  String out;
  out.reserve(noTag.length() / 2 + 64);
  bool lastSpace = true;
  for (unsigned i = 0; i < noTag.length(); i++) {
    char c = noTag[i];
    bool sp = (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v');
    if (sp) { if (!lastSpace) { out += ' '; lastSpace = true; } }
    else { out += c; lastSpace = false; }
    if (out.length() >= maxChars + 32) break;
  }
  out.trim();
  if (out.length() > maxChars) out = out.substring(0, maxChars);
  return out;
}

// GET a URL and return cleaned text. Returns true on success.
static bool fetchWebText(const String& url, String& text, String& err) {
  bool isHttps = url.startsWith("https://");
  HTTPClient http;
  WiFiClientSecure secure;
  WiFiClient plain;
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  bool begun = false;
  if (isHttps) {
    secure.setInsecure();
    secure.setTimeout(15000);
    begun = http.begin(secure, url);
  } else {
    plain.setTimeout(15000);
    begun = http.begin(plain, url);
  }
  if (!begun) { err = "begin failed"; return false; }
  http.addHeader("User-Agent", "ESP32-Agent/1.0");
  http.addHeader("Accept", "text/html,text/plain,*/*");
  http.useHTTP10(true);
  http.setReuse(false);
  int code = http.GET();
  if (code <= 0) { err = String("request failed: ") + http.errorToString(code); http.end(); return false; }
  if (code < 200 || code >= 300) {
    String eb = http.getString();
    http.end();
    eb.replace("\"", "'");
    eb.replace("\n", " ");
    if (eb.length() > 300) eb = eb.substring(0, 300);
    err = String("HTTP ") + code + ": " + eb;
    return false;
  }
  String body = http.getString();
  http.end();
  if (!body.length()) { err = "empty body"; return false; }
  if (body.length() > 8000) body = body.substring(0, 8000);
  text = stripHtmlToText(body, 2000);
  if (!text.length()) { err = "no text extracted"; return false; }
  return true;
}

// Execute one tool call. args is already-parsed object (may be empty).
String execTool(const String& name, JsonObject args) {
  JsonDocument out;
  if (name == "get_status") {
    out["heap"] = ESP.getFreeHeap();
    out["uptime_s"] = (int)(millis() / 1000);
    out["wifi"] = WiFi.SSID();
    out["rssi"] = WiFi.status()==WL_CONNECTED ? WiFi.RSSI() : 0;
    out["ip"] = WiFi.status()==WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    out["model"] = cfgModel;
  } else if (name == "gpio_write") {
    int pin = args["pin"] | -1;
    int lvl = args["level"] | -1;
    if (!pinCanWrite(pin)) { out["error"] = "pin not allowed for output"; }
    else if (lvl != 0 && lvl != 1) { out["error"] = "level must be 0 or 1"; }
    else { pinMode(pin, OUTPUT); digitalWrite(pin, lvl); out["ok"] = true; out["pin"] = pin; out["level"] = lvl; }
  } else if (name == "gpio_read") {
    int pin = args["pin"] | -1;
    if (!pinCanRead(pin)) { out["error"] = "pin not allowed"; }
    else { pinMode(pin, INPUT); out["pin"] = pin; out["level"] = digitalRead(pin); }
  } else if (name == "adc_read") {
    int pin = args["pin"] | -1;
    if (!pinCanADC(pin)) { out["error"] = "use ADC1 pins 32,33,34,35,36,39"; }
    else { out["pin"] = pin; out["value"] = analogRead(pin); }
  } else if (name == "wifi_scan") {
    int n = WiFi.scanNetworks();
    JsonArray arr = out["networks"].to<JsonArray>();
    for (int i = 0; i < n && i < 10; i++) {
      JsonObject o = arr.add<JsonObject>();
      o["ssid"] = WiFi.SSID(i);
      o["rssi"] = WiFi.RSSI(i);
    }
    out["count"] = n;
    WiFi.scanDelete();
  } else if (name == "mem_save") {
    String t = String((const char*)(args["text"] | ""));
    out["ok"] = appendMemoryFact(t);
  } else if (name == "mem_read") {
    out["memory"] = readMemoryFacts().substring(0, 2000);
  } else if (name == "restart_device") {
    bool c = args["confirm"] | false;
    if (!c) out["error"] = "needs confirm=true";
    else { out["ok"] = true; out["restarting"] = true; gRestartPending = true; }
  } else if (name == "blink") {
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
  } else if (name == "web_fetch") {
    String url = String((const char*)(args["url"] | ""));
    url.trim();
    if (!url.length()) { out["error"] = "url required (include https://)"; }
    else if (url.length() > 512) { out["error"] = "url too long (max 512)"; }
    else if (!(url.startsWith("http://") || url.startsWith("https://"))) { out["error"] = "url must start with http:// or https://"; }
    else {
      String txt, ferr;
      if (fetchWebText(url, txt, ferr)) {
        out["url"] = url.substring(0, 200);
        out["text"] = txt;
      } else {
        out["error"] = ferr.substring(0, 300);
        out["url"] = url.substring(0, 200);
      }
    }
  } else {
    out["error"] = "unknown tool";
  }
  String s; serializeJson(out, s);
  size_t cap = (name == "web_fetch") ? 2200 : 1200;
  if (s.length() > cap) s = s.substring(0, cap);
  return s;
}
