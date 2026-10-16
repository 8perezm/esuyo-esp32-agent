#include "llm.h"
#include "config.h"
#include "agent_tools.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

bool postLLM(const String& reqBody, String& respBody, int& code) {
  String url = chatCompletionsUrl(cfgEndpoint);
  bool https = url.startsWith("https://");
  HTTPClient http;
  WiFiClientSecure secure;
  WiFiClient plain;
  http.setTimeout(45000);
  if (https) {
    secure.setInsecure();
    secure.setTimeout(45000);
    if (!http.begin(secure, url)) return false;
  } else {
    plain.setTimeout(45000);
    if (!http.begin(plain, url)) return false;
  }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  http.addHeader("HTTP-Referer", "http://esp32-agent.local");
  http.addHeader("X-Title", "ESP32-Agent");
  if (cfgApiKey.length()) http.addHeader("Authorization", "Bearer " + cfgApiKey);
  http.useHTTP10(true);
  http.setReuse(false);
  code = http.POST(reqBody);
  if (code <= 0) { http.end(); return false; }
  respBody = http.getString();
  http.end();
  return true;
}

void addHistoryCapped(JsonArray msgs, JsonArray history, int maxTurns) {
  if (history.isNull()) return;
  int count = 0;
  for (JsonObject m : history) {
    if (count++ >= maxTurns * 2) break;
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

// Agentic loop: sends tools (only enabled), executes tool_calls locally, repeats.
// Returns final text or ERROR:. outToolsUsed collects names for UI.
String callLLMWithTools(const String& userMsg, JsonArray history, String* outReasoning, String* outToolsUsed) {
  if (cfgEndpoint.length() == 0) return "ERROR: agent endpoint not configured. Open /agent.";
  JsonDocument req;
  req["model"] = cfgModel;
  req["temperature"] = 0.7;
  req["max_tokens"] = 800;
  req["stream"] = false;
  JsonArray msgs = req["messages"].to<JsonArray>();
  JsonObject sys = msgs.add<JsonObject>();
  sys["role"] = "system";
  sys["content"] = buildSystemPrompt();
  injectRecentLog(msgs, userMsg, 8);
  addHistoryCapped(msgs, history, 8);
  JsonObject u = msgs.add<JsonObject>();
  u["role"] = "user";
  u["content"] = userMsg.substring(0, 1500);
  addToolsArray(req);

  String toolsLog = "";
  String toolSummary = "";    // "name: result" lines for graceful fallback
  String pendingPrefix = "";  // text that arrived alongside tool calls
  String finalText = "";
  String finalReason = "";
  bool sawHttpFail = false;   // a post-tools follow-up POST failed
  for (int iter = 0; iter < 3; iter++) {
    String body; serializeJson(req, body);
    Serial.printf("[agent] iter%d body=%dB msgs=%d\n", iter, body.length(), msgs.size());
    String resp; int code = 0;
    if (!postLLM(body, resp, code)) {
      if (iter > 0 && toolSummary.length()) { sawHttpFail = true; break; }
      return "ERROR: upstream request failed.";
    }
    if (code < 200 || code >= 300) {
      if (resp.length() > 600) resp = resp.substring(0, 600);
      if (iter > 0 && toolSummary.length()) {
        Serial.printf("[agent] follow-up HTTP %d, answering from tool results\n", code);
        sawHttpFail = true;
        break;  // degrade: answer from tool results
      }
      return "ERROR: upstream HTTP " + String(code) + ": " + resp;
    }
    JsonDocument res;
    if (deserializeJson(res, resp)) {
      if (iter > 0 && toolSummary.length()) { sawHttpFail = true; break; }
      return "ERROR: bad JSON from endpoint.";
    }
    JsonObject msg0 = res["choices"][0]["message"].as<JsonObject>();
    if (msg0.isNull()) return "ERROR: empty reply from model.";
    JsonArray calls = msg0["tool_calls"].as<JsonArray>();
    // legacy function_call fallback
    bool hasLegacy = !msg0["function_call"].isNull() && msg0["function_call"]["name"];
    if (calls.isNull() || calls.size() == 0) {
      if (!hasLegacy) {
        const char* c1 = msg0["content"] | (const char*)nullptr;
        const char* rCC = msg0["reasoning_content"] | (const char*)nullptr;
        const char* rR = msg0["reasoning"] | (const char*)nullptr;
        const char* rD = msg0["reasoning_details"][0]["text"] | (const char*)nullptr;
        if (c1 && c1[0]) finalText = String(c1);
        else if (rCC && rCC[0]) finalText = String(rCC);  // reasoning-model answer
        else if (rR && rR[0]) finalText = String(rR);
        else if (rD && rD[0]) finalText = String(rD);
        else {
          String dbg = resp.substring(0, 300);
          Serial.printf("[agent] no content: %s\n", dbg.c_str());
          finalText = "";
        }
        // keep the deepest reasoning for the Thinking block (accumulates tool rationale)
        if (rCC && rCC[0]) finalReason += String(rCC);
        else if (rR && rR[0]) finalReason += String(rR);
        else if (rD && rD[0]) finalReason += String(rD);
        if (pendingPrefix.length() && finalText.length()) finalText = pendingPrefix + finalText;
        break;
      }
    }
    // collect calls (new + legacy). Block cap 20: stack + heap + time
    // budget guard below keep this safe on ESP32 (~220KB heap).
    static const int MAX_CALLS = 20;
    struct One { String id, name, args; };
    One list[MAX_CALLS]; int nCalls = 0;
    if (!calls.isNull()) {
      for (JsonObject tc : calls) {
        if (nCalls >= MAX_CALLS) break;
        String id = String((const char*)(tc["id"] | ""));
        JsonObject fn = tc["function"].as<JsonObject>();
        String nm = String((const char*)(fn["name"] | ""));
        String ag = String((const char*)(fn["arguments"] | ""));
        if (!nm.length()) continue;
        if (!id.length()) id = "call_" + String(iter) + "_" + String(nCalls);
        list[nCalls++] = {id, nm, ag};
      }
    }
    if (hasLegacy && nCalls < MAX_CALLS) {
      String nm = String((const char*)(msg0["function_call"]["name"] | ""));
      String ag = String((const char*)(msg0["function_call"]["arguments"] | ""));
      if (nm.length()) list[nCalls++] = {"call_" + String(iter) + "_0", nm, ag};
    }
    if (nCalls == 0) {
      const char* c1 = msg0["content"] | "";
      finalText = String(c1);
      break;
    }
    // append assistant tool_calls message (preserve ids for OpenAI).
    // Keep any same-message text: some models narrate ("I'll check…") + call.
    const char* pre = msg0["content"] | "";
    const char* preR = msg0["reasoning_content"] | (const char*)nullptr;
    if (!preR || !preR[0]) preR = msg0["reasoning"] | (const char*)nullptr;
    if (pre && pre[0]) {
      pendingPrefix += String(pre).substring(0, 500) + "\n";
      if (pendingPrefix.length() > 800) pendingPrefix = pendingPrefix.substring(0, 800);
    }
    if (preR && preR[0]) {
      finalReason += String(preR).substring(0, 800) + "\n";
      if (finalReason.length() > 3000) finalReason = finalReason.substring(0, 3000);
    }
    JsonObject am = msgs.add<JsonObject>();
    am["role"] = "assistant";
    if (pre && pre[0]) am["content"] = String(pre).substring(0, 500);
    JsonArray tc2 = am["tool_calls"].to<JsonArray>();
    for (int i = 0; i < nCalls; i++) {
      JsonObject t = tc2.add<JsonObject>();
      t["id"] = list[i].id;
      t["type"] = "function";
      JsonObject fn = t["function"].to<JsonObject>();
      fn["name"] = list[i].name;
      fn["arguments"] = list[i].args;
    }
    // execute each, append tool result. Time budget: slow tools (wifi_scan
    // ~2s, blink ~10s) must not wedge the HTTP handler forever. Every call
    // still gets a result so tool_call_id pairing stays valid.
    unsigned long tBudget = millis();
    for (int i = 0; i < nCalls; i++) {
      if (!toolEnabled(list[i].name)) {
        JsonObject tm = msgs.add<JsonObject>();
        tm["role"] = "tool"; tm["tool_call_id"] = list[i].id;
        tm["content"] = "{\"error\":\"tool disabled in /functions\"}";
        toolsLog += list[i].name + "(disabled) ";
        toolSummary += String(list[i].name) + ": disabled\n";
        continue;
      }
      String res2;
      if (i > 0 && millis() - tBudget > 25000) {
        res2 = "{\"error\":\"tool time budget (25s) exceeded, skipped\"}";
        Serial.printf("[tool] %s SKIPPED (budget)\n", list[i].name.c_str());
      } else {
        JsonDocument ad;
        JsonObject ao;
        if (list[i].args.length() && !deserializeJson(ad, list[i].args)) ao = ad.as<JsonObject>();
        else { JsonDocument e; ao = e.as<JsonObject>(); }
        res2 = execTool(list[i].name, ao);
        Serial.printf("[tool] %s -> %s\n", list[i].name.c_str(), res2.substring(0, 120).c_str());
      }
      JsonObject tm = msgs.add<JsonObject>();
      tm["role"] = "tool";
      tm["tool_call_id"] = list[i].id;
      tm["content"] = res2;
      // also support legacy function role for old models
      tm["name"] = list[i].name;
      toolsLog += list[i].name + " ";
      toolSummary += String(list[i].name) + ": " + res2.substring(0, 400) + "\n";
      if (toolSummary.length() > 1500) toolSummary = toolSummary.substring(0, 1500);
    }
    // loop continues with enriched msgs; keep tools array as-is
  }
  if (finalText.length() == 0 && toolSummary.length()) {
    // No text synthesis came back: either a follow-up POST failed (sawHttpFail)
    // or the model kept emitting calls until the step budget ran out.
    // Either way, answer straight from the tool results — never empty-handed.
    if (sawHttpFail) finalText = "Ran " + toolsLog + "but the summary request failed. Results:\n" + toolSummary;
    else finalText = "Ran " + toolsLog + "(step budget reached before a summary). Results:\n" + toolSummary;
  }
  if (finalText.length() == 0) finalText = "(tool run done, no text reply)";
  if (finalText.length() > 3500) finalText = finalText.substring(0, 3500);
  if (outReasoning) *outReasoning = finalReason.substring(0, 2500);
  if (outToolsUsed) *outToolsUsed = toolsLog;
  // persist once per turn (never per token) + handle pending restart
  appendChatLog(userMsg, finalText);
  if (gRestartPending) {
    // do restart after response is sent; flag consumed by caller
  }
  return finalText;
}

String callLLM(const String& userMsg, JsonArray history, String* outReasoning) {
  if (cfgEndpoint.length() == 0) return "ERROR: agent endpoint not configured. Open /agent.";
  String url = chatCompletionsUrl(cfgEndpoint);

  // Build request JSON (cap history to last 10 turns to fit ESP32 RAM).
  JsonDocument req;
  req["model"] = cfgModel;
  req["temperature"] = 0.7;
  req["max_tokens"] = 800;  // bound reply size for ESP32 RAM + avoids chunked giant bodies
  req["stream"] = false;
  JsonArray msgs = req["messages"].to<JsonArray>();
  JsonObject sys = msgs.add<JsonObject>();
  sys["role"] = "system";
  sys["content"] = buildSystemPrompt();
  injectRecentLog(msgs, userMsg, 8);
  int count = 0;
  for (JsonObject m : history) {
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
  JsonObject u = msgs.add<JsonObject>();
  u["role"] = "user";
  u["content"] = userMsg.substring(0,1500);

  String body;
  serializeJson(req, body);

  bool https = url.startsWith("https://");
  HTTPClient http;
  WiFiClientSecure secure;
  WiFiClient plain;
  http.setTimeout(60000);
  if (https) {
    secure.setInsecure();  // no cert store on ESP32; LAN-friendly. Use http:// on LAN if you prefer.
    secure.setTimeout(60000);
    if (!http.begin(secure, url)) return "ERROR: http.begin(https) failed.";
  } else {
    plain.setTimeout(60000);
    if (!http.begin(plain, url)) return "ERROR: http.begin(http) failed.";
  }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  http.addHeader("HTTP-Referer", "http://esp32-agent.local");
  http.addHeader("X-Title", "ESP32-Agent");
  if (cfgApiKey.length()) http.addHeader("Authorization", "Bearer " + cfgApiKey);
  http.useHTTP10(true);   // HTTP/1.0: server sends Content-Length instead of chunked (ESP32 HTTPClient handles it reliably)
  http.setReuse(false);

  Serial.printf("[llm] POST %s model=%s body=%dB\n", url.c_str(), cfgModel.c_str(), body.length());
  const char* hdrKeys[] = {"Content-Type", "Content-Encoding", "Transfer-Encoding"};
  http.collectHeaders(hdrKeys, 3);
  int code = http.POST(body);
  if (code <= 0) {
    String e = http.errorToString(code);
    http.end();
    Serial.printf("[llm] POST failed: %s\n", e.c_str());
    return "ERROR: upstream request failed: " + e;
  }
  // Read the body (getString handles chunked HTTPS; manual stream-read hung here).
  String resp = http.getString();
  int hdrLen = http.getSize();
  int len = resp.length();
  Serial.printf("[llm] hdrLen=%d got=%dB\n", hdrLen, len);
  Serial.printf("[llm] HTTP %d, %d bytes\n", code, len);
  {
    String preview = resp.substring(0, 160);
    preview.replace("\n", "\\n"); preview.replace("\r", "\\r");
    Serial.printf("[llm] body preview: %s\n", preview.c_str());
    Serial.print("[llm] hex:");
    for (int i = 0; i < len && i < 24; i++) { Serial.printf(" %02X", (uint8_t)resp[i]); }
    Serial.println();
    Serial.printf("[llm] enc:%s ctype:%s\n", http.header("Content-Encoding").c_str(), http.header("Content-Type").c_str());
  }
  http.end();
  String trimmed = resp;
  trimmed.trim();
  if (trimmed.length() == 0) return "ERROR: upstream returned HTTP " + String(code) + " with empty body (" + String(len) + "B whitespace). Check model name + API key at /agent.";
  if (code < 200 || code >= 300) {
    if (resp.length() > 800) resp = resp.substring(0, 800);
    return "ERROR: upstream HTTP " + String(code) + ": " + resp;
  }
  JsonDocument res;
  DeserializationError err = deserializeJson(res, resp);
  if (err) return "ERROR: bad JSON from endpoint: " + String(err.c_str());
  JsonArray choices = res["choices"].as<JsonArray>();
  Serial.printf("[llm] choices=%d mem=%u\n", (int)choices.size(), (unsigned)res.memoryUsage());
  Serial.printf("[llm] msgsize=%d\n", (int)res["choices"][0]["message"].size());
  const char* content = res["choices"][0]["message"]["content"] | (const char*)nullptr;
  // Provider variants: OpenRouter/Together use several field names.
  const char* reasoningCC = res["choices"][0]["message"]["reasoning_content"] | (const char*)nullptr;  // DeepSeek native
  const char* reasoning = res["choices"][0]["message"]["reasoning"] | (const char*)nullptr;            // OpenRouter Together wrapper
  const char* rdText = res["choices"][0]["message"]["reasoning_details"][0]["text"] | (const char*)nullptr;
  const char* text = res["choices"][0]["text"] | (const char*)nullptr;
  const char* errMsg = res["error"]["message"] | (const char*)nullptr;
  if ((!content || !content[0])) {
    String dbg = resp.substring(0, 600);
    dbg.replace("\n", " ");
    Serial.printf("[llm] no content. rCC=%d r=%d rd=%d text=%s. body: %s\n",
      reasoningCC ? (int)strlen(reasoningCC) : -1,
      reasoning ? (int)strlen(reasoning) : -1,
      rdText ? (int)strlen(rdText) : -1,
      text && text[0] ? "yes" : "no", dbg.c_str());
  }
  String out, reason;
  if (content && content[0]) {
    out = String(content);
    if (reasoningCC && reasoningCC[0]) reason = String(reasoningCC);
    else if (reasoning && reasoning[0]) reason = String(reasoning);
    else if (rdText && rdText[0]) reason = String(rdText);
    if (reason.length() > 3000) reason = reason.substring(0, 3000);
    if (outReasoning) *outReasoning = reason;
  }
  else if (reasoningCC && reasoningCC[0]) out = String(reasoningCC);
  else if (reasoning && reasoning[0]) out = String(reasoning);
  else if (rdText && rdText[0]) out = String(rdText);
  else if (text && text[0]) out = String(text);
  if (out.length() == 0) {
    if (errMsg && errMsg[0]) return "ERROR: " + String(errMsg);
    return "ERROR: empty reply from model (997B JSON but no content — see serial for body).";
  }
  if (out.length() > 4000) out = out.substring(0, 4000);
  return out;
}
