#include "net.h"
#include "config.h"
#include <WiFi.h>

WebServer server(80);
DNSServer dnsServer;
bool apMode = false;
bool dnsRunning = false;
const byte DNS_PORT = 53;

bool connectWiFi(unsigned long timeoutMs) {
  if (!cfgSsid.length()) return false;
  Serial.printf("Connecting to %s ...\n", cfgSsid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(cfgSsid.c_str(), cfgPass.c_str());
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeoutMs) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("WiFi OK: %s\n", WiFi.localIP().toString().c_str());
    return true;
  }
  Serial.println("WiFi failed");
  WiFi.disconnect(true, true);
  return false;
}

void startAP() {
  apMode = true;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("AP started: %s -> http://192.168.4.1/setup\n", AP_SSID);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  dnsRunning = true;
}
