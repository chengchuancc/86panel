#include "AppConfig.h"
#include "DashboardUi.h"
#include "DisplayPanel.h"
#include "NetStats.h"
#include "OtaService.h"

#include <Arduino.h>
#include <WiFi.h>
#include <lvgl.h>
#include <time.h>

static Stats stats;
static uint32_t reconnect_count = 0;

static void syncTime()
{
  configTzTime("CST-8", "ntp.aliyun.com", "ntp.tencent.com", "pool.ntp.org");
}

static void connectWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  if (USE_STATIC_IP) {
    WiFi.config(DEVICE_IP, GATEWAY_IP, SUBNET_MASK, DNS_IP);
  }
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void setup()
{
  Serial.begin(115200);
  delay(200);

  if (!DisplayPanel::begin()) {
    Serial.println("display init failed");
    while (true) delay(1000);
  }

  DashboardUi::build();
  DashboardUi::updateClock();
  DashboardUi::updateNetworkIdentity();
  DashboardUi::updateStats(stats);
  DashboardUi::updateConnectionView(stats, reconnect_count);

  connectWiFi();
  syncTime();
}

void loop()
{
  static uint32_t last_stats_ms = 0;
  static uint32_t last_clock_ms = 0;
  static uint32_t last_wifi_retry_ms = 0;
  static bool ota_started = false;
  static int last_sync_yday = -1;

  lv_timer_handler();

  if (ota_started) {
    OtaService::handle();
  }

  uint32_t now = millis();

  if (WiFi.status() == WL_CONNECTED && !ota_started) {
    Serial.printf("WiFi connected, IP=%s, gateway=%s, RSSI=%d\n",
                  WiFi.localIP().toString().c_str(),
                  WiFi.gatewayIP().toString().c_str(),
                  WiFi.RSSI());
    OtaService::begin();
    ota_started = true;
    DashboardUi::updateNetworkIdentity();
  }

  if (WiFi.status() != WL_CONNECTED && now - last_wifi_retry_ms > 5000) {
    last_wifi_retry_ms = now;
    WiFi.disconnect();
    reconnect_count++;
    connectWiFi();
    DashboardUi::updateNetworkIdentity();
  }

  if (now - last_clock_ms > 1000) {
    last_clock_ms = now;
    DashboardUi::updateClock();
    DashboardUi::updateNetworkIdentity();

    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5) && timeinfo.tm_hour == 0 && timeinfo.tm_min == 0 &&
        timeinfo.tm_yday != last_sync_yday) {
      last_sync_yday = timeinfo.tm_yday;
      syncTime();
    }
  }

  if (now - last_stats_ms > STATS_REFRESH_MS) {
    last_stats_ms = now;
    bool ok = fetchStats(stats);
    if (!ok && stats.last_ok_ms == 0) {
      stats.online = false;
    } else if (!ok && WiFi.status() != WL_CONNECTED && now - stats.last_ok_ms > 3000) {
      stats.online = false;
    } else if (!ok && now - stats.last_ok_ms > 30000) {
      stats.online = false;
    }
    DashboardUi::updateStats(stats);
    DashboardUi::updateConnectionView(stats, reconnect_count);
  }

  delay(5);
}
