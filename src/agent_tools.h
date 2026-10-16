#pragma once
// agent_tools.h — device tools + LittleFS memory/log. Small, safe to read.
// HTML blobs are in web_ui.h (skip that file unless editing UI to save context).
#include <Arduino.h>
#include <ArduinoJson.h>

extern const char* TOOL_NAMES[];
extern const int TOOL_COUNT;

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
