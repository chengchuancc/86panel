#include "OtaService.h"

#include "AppConfig.h"

#include <Arduino.h>
#include <ArduinoOTA.h>

void OtaService::begin()
{
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() { Serial.println("OTA start"); });
  ArduinoOTA.onEnd([]() { Serial.println("OTA end"); });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error: %u\n", error);
  });
  ArduinoOTA.begin();
}

void OtaService::handle()
{
  ArduinoOTA.handle();
}
