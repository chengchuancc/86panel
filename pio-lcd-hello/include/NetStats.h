#pragma once

#include <Arduino.h>

struct Stats {
  int cpu = 0;
  int temp = 0;
  int mem = 0;
  int disk = 0;
  int conn = 0;
  int clients = 0;
  int cores[4] = {0, 0, 0, 0};
  float eth0_rx = 0;
  float eth0_tx = 0;
  float eth1_rx = 0;
  float eth1_tx = 0;
  String uptime = "00d 00h 00m";
  bool online = false;
  uint32_t last_ok_ms = 0;
  String error = "boot";
};

IPAddress statsTargetHost();
String statsTargetUrl();
bool fetchStats(Stats &stats);
