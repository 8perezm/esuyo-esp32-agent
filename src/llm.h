#pragma once
// llm.h — OpenAI-compatible bridge (non-stream + agentic tool loop).
#include <Arduino.h>
#include <ArduinoJson.h>

bool postLLM(const String& reqBody, String& respBody, int& code);
void addHistoryCapped(JsonArray msgs, JsonArray history, int maxTurns = 10);
String callLLMWithTools(const String& userMsg, JsonArray history, String* outReasoning, String* outToolsUsed);
String callLLM(const String& userMsg, JsonArray history, String* outReasoning = nullptr);
