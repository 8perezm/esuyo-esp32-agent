#pragma once
// agent_tools.h — device tools + LittleFS memory/log.
// Tools themselves live in src/tools/tool_*.cpp (one file each).
// To add a tool, copy src/tools/_template.cpp.sample — don't edit this file.
#include <Arduino.h>
#include <ArduinoJson.h>
#include "tools/tool_api.h"

extern bool gFsOk;
extern bool gRestartPending;

extern const char* LOG_PATH;
extern const char* MEM_PATH;
extern const size_t LOG_MAX_BYTES;
extern const size_t MEM_MAX_BYTES;

void initAgentFS();
int toolIndex(const String& n);
bool toolEnabled(const String& n);
bool toolEnabledIdx(int i);
// Bitmask with the first TOOL_COUNT bits set (fresh-device default).
uint32_t allToolsMask();
// Per-tool metadata for /api/tools and /functions (no UI edit per tool).
const char* toolNameAt(int i);
const char* toolUiHintAt(int i);
String toolDescriptionAt(int i);
String readMemoryFacts();
bool appendMemoryFact(const String& text);
bool appendChatLog(const String& user, const String& asst);
int injectRecentLog(JsonArray msgs, const String& curUser, int maxMsgs = 10);
bool pinCanWrite(int p);
bool pinCanRead(int p);
bool pinCanADC(int p);
String buildSystemPrompt();
void addToolsArray(JsonDocument& req);
String execTool(const String& name, JsonObject args);
