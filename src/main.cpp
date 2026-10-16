// main.cpp — slim entry point. Logic lives in config / agent_tools / llm / routes / net.
// web_ui.h holds PROGMEM HTML (skip it unless editing UI to save context).
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

#include "config.h"
#include "agent_tools.h"
#include "net.h"
#include "routes.h"

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println("\n=== Agent boot ===");
  loadConfig();
  initAgentFS();

  bool ok = connectWiFi(15000);
  if (!ok) startAP();
  else { WiFi.mode(WIFI_STA); }
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);  // STA responsiveness over power-save; avoids ping/page stalls

  if (MDNS.begin(MDNS_NAME)) {
    Serial.printf("mDNS: http://%s.local\n", MDNS_NAME);
    MDNS.addService("http", "tcp", 80);
  }

  // ArduinoOTA — enables `pio run --target upload --upload-port <IP>` (no button, no cable).
  ArduinoOTA.setHostname(MDNS_NAME);
  if (cfgAuthToken.length()) ArduinoOTA.setPassword(cfgAuthToken.c_str());
  // When token is empty OTA stays open (first-boot UX); setting a device
  // password via POST /api/auth also locks ArduinoOTA to the same password.
  ArduinoOTA.onStart([] { Serial.println("OTA start"); });
  ArduinoOTA.onEnd([] { Serial.println("OTA end"); });
  ArduinoOTA.onError([](ota_error_t e) { Serial.printf("OTA error %u\n", e); });
  ArduinoOTA.begin();

  setupRoutes();
  server.begin();
  Serial.println("HTTP server on :80");
  Serial.printf("Mode: %s | endpoint %s | model %s\n",
    apMode ? "AP-SETUP (http://192.168.4.1/setup)" : WiFi.localIP().toString().c_str(),
    cfgEndpoint.length() ? cfgEndpoint.c_str() : "(not set)",
    cfgModel.c_str());
}

void loop() {
  if (dnsRunning) dnsServer.processNextRequest();
  server.handleClient();
  ArduinoOTA.handle();
  // Non-blocking STA watchdog: a brief router drop used to leave .44 dead
  // until reboot (connectWiFi runs once at boot). Reconnect in background.
  if (!apMode && WiFi.status() != WL_CONNECTED) {
    static unsigned long lastRetry = 0;
    unsigned long now = millis();
    if (now - lastRetry > 30000) {
      lastRetry = now;
      Serial.println("WiFi lost, reconnecting…");
      WiFi.disconnect(false);
      WiFi.reconnect();
    }
  }
  delay(2);
}
