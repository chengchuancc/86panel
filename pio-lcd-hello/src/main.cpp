#include "AppConfig.h"
#include "DashboardUi.h"
#include "DisplayPanel.h"
#include "NetStats.h"
#include "OtaService.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <lvgl.h>
#include <time.h>

static Stats stats;
static uint32_t reconnect_count = 0;
static bool wifi_connecting = false;

static void syncTime()
{
  configTzTime("CST-8", "ntp.aliyun.com", "ntp.tencent.com", "pool.ntp.org");
}

static void connectWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(true);
  if (USE_STATIC_IP) {
    WiFi.config(DEVICE_IP, GATEWAY_IP, SUBNET_MASK, DNS_IP);
  }
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  wifi_connecting = true;
}

void setup()
{
  Serial.begin(115200);
  delay(200);

  // Reconfigure TWDT to be more lenient (loop has network I/O)
  esp_task_wdt_config_t wdt_cfg = {
    .timeout_ms = 30000,
    .idle_core_mask = 0,
    .trigger_panic = false,
  };
  esp_task_wdt_reconfigure(&wdt_cfg);

  if (!DisplayPanel::begin()) {
    Serial.println("display init failed");
    while (true) delay(1000);
  }

  DashboardUi::build();
  DashboardUi::updateClock();
  DashboardUi::updateNetworkIdentity();
  DashboardUi::setTargetStats(stats);
  DashboardUi::interpolateUpdate(1.0f);
  DashboardUi::updateConnectionView(stats, reconnect_count);

  connectWiFi();
  syncTime();

  // Serial only needed during boot diagnostics; close to save USB CDC power
  Serial.end();
}

void loop()
{
  static uint32_t last_stats_ms = 0;
  static uint32_t last_clock_ms = 0;
  static uint32_t last_wifi_retry_ms = 0;
  static bool ota_started = false;
  static int last_sync_yday = -1;
  static bool stats_changed = false;

  lv_timer_handler();

  if (ota_started) {
    OtaService::handle();
  }

  uint32_t now = millis();

  wl_status_t ws = WiFi.status();

  if (ws == WL_CONNECTED && !ota_started) {
    OtaService::begin();
    ota_started = true;
    wifi_connecting = false;
    DashboardUi::updateNetworkIdentity();
  }

  if (ws != WL_CONNECTED && !wifi_connecting && now - last_wifi_retry_ms > 5000) {
    last_wifi_retry_ms = now;
    reconnect_count++;
    ota_started = false; // Reset so OTA re-initializes after reconnect
    connectWiFi();
    DashboardUi::updateNetworkIdentity();
  }

  if (now - last_clock_ms > 1000) {
    last_clock_ms = now;
    DashboardUi::updateClock();
    DashboardUi::updateNetworkIdentity();

    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5)) {
      if (timeinfo.tm_hour == 0 && timeinfo.tm_min == 0 &&
          timeinfo.tm_yday != last_sync_yday) {
        last_sync_yday = timeinfo.tm_yday;
        syncTime();
      }
    }
  }

  if (now - last_stats_ms > STATS_REFRESH_MS) {
    last_stats_ms = now;
    bool ok = fetchStats(stats);
    if (!ok && stats.last_ok_ms == 0) {
      stats.online = false;
    } else if (!ok && ws != WL_CONNECTED && now - stats.last_ok_ms > 3000) {
      stats.online = false;
    } else if (!ok && now - stats.last_ok_ms > 30000) {
      stats.online = false;
    }
    DashboardUi::setTargetStats(stats);
    DashboardUi::updateConnectionView(stats, reconnect_count);
    stats_changed = true;
  }

  {
    static uint32_t last_interp_ms = 0;
    // Only redraw when new data arrived or animation is still in progress
    if (now - last_stats_ms < STATS_REFRESH_MS || stats_changed) {
      if (now - last_interp_ms >= 100) {
        last_interp_ms = now;
        float t = (float)(now - last_stats_ms) / (float)STATS_REFRESH_MS;
        DashboardUi::interpolateUpdate(t);
        stats_changed = (t < 1.0f);
      }
    }
  }

  delay(10);
}
