#pragma once
// net.h — WiFi STA + AP + shared server objects.
#include <Arduino.h>
#include <WebServer.h>
#include <DNSServer.h>

extern WebServer server;
extern DNSServer dnsServer;
extern bool apMode;
extern bool dnsRunning;
extern const byte DNS_PORT;

bool connectWiFi(unsigned long timeoutMs);
void startAP();
