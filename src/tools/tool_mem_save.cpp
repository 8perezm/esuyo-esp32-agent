// mem_save — save one durable fact (<=500 chars) to /memory.txt.
#include "tool_api.h"
#include "agent_tools.h"

static void describe(JsonObject fn) {
  fn["description"] = "Save a durable user preference/fact (<=500 chars).";
  JsonObject p = fn["parameters"].to<JsonObject>();
  p["type"] = "object";
  JsonObject pr = p["properties"].to<JsonObject>();
  pr["text"].to<JsonObject>()["type"] = "string";
  JsonArray r = p["required"].to<JsonArray>();
  r.add("text");
}

static String run(JsonObject args) {
  JsonDocument out;
  String t = String((const char*)(args["text"] | ""));
  out["ok"] = appendMemoryFact(t);
  String s;
  serializeJson(out, s);
  return s;
}

extern const ToolDef Tool_mem_save = { "mem_save", "Save a memory fact", describe, run, 1200 };
