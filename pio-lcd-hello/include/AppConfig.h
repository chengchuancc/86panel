#pragma once

#include <Arduino.h>

static constexpr int I2C_SDA = 15;
static constexpr int I2C_SCL = 7;
static constexpr uint8_t IO_EXPANDER_ADDR = 0x24;
static constexpr uint32_t LVGL_TICK_MS = 2;
static constexpr uint16_t SCREEN_W = 480;
static constexpr uint16_t SCREEN_H = 480;
static constexpr uint16_t CHART_POINTS = 118;
static constexpr int32_t CHART_MAX_KBPS = 12288;
static constexpr uint32_t STATS_REFRESH_MS = 1000;
static constexpr uint32_t STATS_HTTP_TIMEOUT_MS = 900;
static constexpr uint8_t DISPLAY_ROTATION = 2;

static const char *WIFI_SSID = "cheng";
static const char *WIFI_PASSWORD = "CC12369874";
static const IPAddress DEVICE_IP(192, 168, 10, 210);
static const IPAddress GATEWAY_IP(192, 168, 10, 1);
static const IPAddress SUBNET_MASK(255, 255, 255, 0);
static const IPAddress DNS_IP(192, 168, 10, 1);
static constexpr bool USE_STATIC_IP = false;
static constexpr bool USE_GATEWAY_FOR_STATS = false;
static const IPAddress STATS_HOST(192, 168, 10, 1);
static constexpr uint16_t STATS_PORT = 80;
static const char *STATS_PATH = "/cgi-bin/stats";

static const char *OTA_HOSTNAME = "home-net-panel";
static const char *OTA_PASSWORD = "CC12369874";
