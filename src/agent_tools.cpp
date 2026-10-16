#include "agent_tools.h"
#include "config.h"
#include <WiFi.h>
#include <LittleFS.h>

bool gFsOk = false;
bool gRestartPending = false;
const char* LOG_PATH = "/chatlog.jsonl";
const char* MEM_PATH = "/memory.txt";
const size_t LOG_MAX_BYTES = 32768;
const size_t MEM_MAX_BYTES = 4096;

int toolIndex(const String& n) {
  for (int i = 0; i < TOOL_COUNT; i++)
    if (n == TOOL_REGISTRY[i]->name) return i;
  return -1;
}
bool toolEnabled(const String& n) {
  int i = toolIndex(n);
  if (i < 0) return false;
  return (gToolsMask >> i) & 1U;
}
bool toolEnabledIdx(int i) { return (gToolsMask >> i) & 1U; }

uint32_t allToolsMask() {
  if (TOOL_COUNT >= 32) return 0xFFFFFFFFUL;
  if (TOOL_COUNT == 0) return 0;
  return (1UL << TOOL_COUNT) - 1UL;
}

const char* toolNameAt(int i) {
  if (i < 0 || i >= TOOL_COUNT) return "";
  return TOOL_REGISTRY[i]->name;
}
const char* toolUiHintAt(int i) {
  if (i < 0 || i >= TOOL_COUNT) return "";
  return TOOL_REGISTRY[i]->uiHint;
}
String toolDescriptionAt(int i) {
  if (i < 0 || i >= TOOL_COUNT) return "";
  JsonDocument tmp;
  JsonObject fn = tmp.to<JsonObject>();
  TOOL_REGISTRY[i]->describe(fn);
  return String((const char*)(fn["description"] | ""));
}

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
    s += TOOL_REGISTRY[i]->name;
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
  for (int i = 0; i < TOOL_COUNT; i++) {
    if (!toolEnabledIdx(i)) continue;
    const ToolDef* d = TOOL_REGISTRY[i];
    JsonObject t = tools.add<JsonObject>();
    t["type"] = "function";
    JsonObject fn = t["function"].to<JsonObject>();
    fn["name"] = d->name;
    d->describe(fn);
  }
  if (tools.size() > 0) req["tool_choice"] = "auto";
}

// Execute one tool call. args is already-parsed object (may be empty).
String execTool(const String& name, JsonObject args) {
  int i = toolIndex(name);
  if (i < 0) return "{\"error\":\"unknown tool\"}";
  const ToolDef* d = TOOL_REGISTRY[i];
  String s = d->run(args);
  size_t cap = d->maxResultChars ? d->maxResultChars : 1200;
  if (s.length() > cap) s = s.substring(0, cap);
  return s;
}
