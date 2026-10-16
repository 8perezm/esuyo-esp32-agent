#pragma once
// tool_api.h — the ONLY file you need to read to add a tool.
//
// One tool = one .cpp file in this folder. Copy `_template.cpp.sample`,
// fill in the 3 marked spots (name, schema, run), add one line to
// `registry.cpp`, done. No other file needs editing.
//
// Registry order = bit position in gToolsMask. Append new tools at the END
// of TOOL_REGISTRY so saved /functions toggles don't scramble.
#include <Arduino.h>
#include <ArduinoJson.h>

struct ToolDef {
  const char* name;                  // e.g. "blink" — unique, sent to the model
  const char* uiHint;                // short label for /functions + /api/tools
  void (*describe)(JsonObject fn);    // fills fn["description"] + fn["parameters"]
  String (*run)(JsonObject args);     // executes, returns a small JSON string
  size_t maxResultChars;             // result cap before sending back to model
};

extern const ToolDef* TOOL_REGISTRY[];
extern const int TOOL_COUNT;
