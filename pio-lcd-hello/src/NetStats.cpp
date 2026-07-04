#include "NetStats.h"

#include "AppConfig.h"

#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClient.h>

IPAddress statsTargetHost()
{
  return STATS_HOST;
}

String statsTargetUrl()
{
  static String cached;
  if (cached.length() == 0) {
    cached = String("http://") + statsTargetHost().toString() + ":" + STATS_PORT + STATS_PATH;
  }
  return cached;
}

String networkProbeSummary()
{
  return String("Target ") + statsTargetUrl();
}

static constexpr size_t MAX_HTTP_BODY = 8192;

static bool readHttpBody(WiFiClient &client, int &status_code, String &body, String &status_line)
{
  status_line = client.readStringUntil('\n');
  status_line.trim();
  if (!status_line.startsWith("HTTP/")) return false;

  int first_space = status_line.indexOf(' ');
  status_code = first_space > 0 ? status_line.substring(first_space + 1, first_space + 4).toInt() : 0;

  while (client.connected() || client.available()) {
    String line = client.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) break;
  }

  body.reserve(1024);
  uint32_t start_ms = millis();
  while (millis() - start_ms < STATS_HTTP_TIMEOUT_MS) {
    while (client.available()) {
      if (body.length() >= MAX_HTTP_BODY) break;
      body += (char)client.read();
      start_ms = millis();
    }
    if (!client.connected() || body.length() >= MAX_HTTP_BODY) break;
    delay(1);
  }

  body.trim();
  return true;
}

static bool parseStatsPayload(const String &raw_payload, Stats &stats)
{
  String payload = raw_payload;
  payload.trim();

  if (payload.length() == 0) {
    stats.error = "EMPTY";
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    // Truncate error message to prevent unbounded String growth
    char err_buf[32];
    snprintf(err_buf, sizeof(err_buf), "JSON %.20s", error.c_str());
    stats.error = err_buf;
    return false;
  }

  stats.cpu = doc["cpu"]["usage"] | 0;
  stats.temp = doc["cpu"]["temp"] | 0;
  for (int i = 0; i < 4; i++) stats.cores[i] = doc["cpu"]["cores"][i] | 0;
  stats.mem = doc["mem"]["usage"] | 0;
  stats.disk = doc["disk"] | 0;
  stats.conn = doc["conn"] | 0;
  stats.clients = doc["clients"] | 0;
  stats.uptime = doc["uptime"] | "00d 00h 00m";
  stats.eth0_rx = doc["net"]["eth0"]["rx"] | 0.0f;
  stats.eth0_tx = doc["net"]["eth0"]["tx"] | 0.0f;
  stats.eth1_rx = doc["net"]["eth1"]["rx"] | 0.0f;
  stats.eth1_tx = doc["net"]["eth1"]["tx"] | 0.0f;
  stats.online = true;
  stats.error = "";
  stats.last_ok_ms = millis();
  return true;
}

bool fetchStats(Stats &stats)
{
  if (WiFi.status() != WL_CONNECTED) {
    stats.error = "WIFI LOST";
    return false;
  }

  IPAddress host = statsTargetHost();
  WiFiClient client;
  client.setTimeout(STATS_HTTP_TIMEOUT_MS);

  if (!client.connect(host, STATS_PORT, STATS_HTTP_TIMEOUT_MS)) {
    stats.error = "CONN FAIL";
    return false;
  }

  client.print("GET ");
  client.print(STATS_PATH);
  client.print(" HTTP/1.0\r\nHost: ");
  client.print(host);
  client.print("\r\nUser-Agent: ESP32Panel/1.0\r\nConnection: close\r\n\r\n");

  int http_code = 0;
  String payload;
  String status_line;
  if (!readHttpBody(client, http_code, payload, status_line)) {
    stats.error = "BAD HTTP";
    client.stop();
    return false;
  }
  client.stop();

  if (http_code != 200) {
    char err_buf[12];
    snprintf(err_buf, sizeof(err_buf), "HTTP %d", http_code);
    stats.error = err_buf;
    return false;
  }

  return parseStatsPayload(payload, stats);
}
