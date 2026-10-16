// web_fetch — GET a URL, return cleaned text (<=2000 chars). No JS, no logins.
#include "tool_api.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// Strip HTML down to readable text (removes scripts/styles/tags, decodes entities).
static String stripHtmlToText(const String& html, size_t maxChars) {
  String t = html;
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

static void describe(JsonObject fn) {
  fn["description"] = "Fetch an http(s) URL and return its text (up to ~2000 chars). Use for current info, docs, prices, headlines. No JS, no login pages.";
  JsonObject p = fn["parameters"].to<JsonObject>();
  p["type"] = "object";
  JsonObject pr = p["properties"].to<JsonObject>();
  pr["url"].to<JsonObject>()["type"] = "string";
  JsonArray r = p["required"].to<JsonArray>();
  r.add("url");
}

static String run(JsonObject args) {
  JsonDocument out;
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
  String s;
  serializeJson(out, s);
  return s;
}

extern const ToolDef Tool_web_fetch = { "web_fetch", "Fetch a web page as text", describe, run, 2200 };
