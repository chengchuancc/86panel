#include "NetStats.h"

#include "AppConfig.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>

IPAddress statsTargetHost()
{
  IPAddress gateway = WiFi.gatewayIP();
  if (USE_GATEWAY_FOR_STATS && gateway != IPAddress(0, 0, 0, 0)) return gateway;
  return STATS_HOST;
}

String statsTargetUrl()
{
  return String("http://") + statsTargetHost().toString() + ":" + STATS_PORT + STATS_PATH;
}

static String compactSnippet(const String &text)
{
  String out = text;
  out.replace("\r", " ");
  out.replace("\n", " ");
  out.trim();
  if (out.length() > 36) out = out.substring(0, 36);
  return out;
}

static bool extractJsonBody(String &text)
{
  int object_start = text.indexOf('{');
  int object_end = text.lastIndexOf('}');
  if (object_start < 0 || object_end <= object_start) return false;
  text = text.substring(object_start, object_end + 1);
  text.trim();
  return true;
}

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

  uint32_t start_ms = millis();
  while (millis() - start_ms < STATS_HTTP_TIMEOUT_MS) {
    while (client.available()) {
      body += (char)client.read();
      start_ms = millis();
    }
    if (!client.connected()) break;
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

  if (!extractJsonBody(payload)) {
    Serial.println("No JSON object in response:");
    Serial.println(payload);
    stats.error = "NO JSON " + compactSnippet(payload);
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.printf("JSON error: %s\n", error.c_str());
    Serial.println(payload);
    stats.error = String("JSON ") + error.c_str();
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

static bool fetchStatsWithHttpClient(Stats &stats)
{
  WiFiClient client;
  HTTPClient http;
  String url = statsTargetUrl();

  http.setTimeout(STATS_HTTP_TIMEOUT_MS);
  http.setReuse(false);
  http.setUserAgent("ESP32Panel/1.0");

  if (!http.begin(client, url)) {
    stats.error = "HTTP BEGIN";
    return false;
  }

  int http_code = http.GET();
  if (http_code != HTTP_CODE_OK) {
    stats.error = "HTTPC " + String(http_code);
    Serial.printf("HTTPClient failed: code=%d url=%s local=%s gateway=%s rssi=%d\n",
                  http_code,
                  url.c_str(),
                  WiFi.localIP().toString().c_str(),
                  WiFi.gatewayIP().toString().c_str(),
                  WiFi.RSSI());
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();
  return parseStatsPayload(payload, stats);
}

static bool fetchStatsWithRawClient(Stats &stats)
{
  IPAddress host = statsTargetHost();
  WiFiClient client;
  client.setTimeout(STATS_HTTP_TIMEOUT_MS);

  if (!client.connect(host, STATS_PORT, STATS_HTTP_TIMEOUT_MS)) {
    stats.error = "TCP FAIL";
    Serial.printf("Stats TCP failed: local=%s gateway=%s target=%s:%u rssi=%d\n",
                  WiFi.localIP().toString().c_str(),
                  WiFi.gatewayIP().toString().c_str(),
                  host.toString().c_str(),
                  STATS_PORT,
                  WiFi.RSSI());
    return false;
  }

  client.print("GET ");
  client.print(STATS_PATH);
  client.print(" HTTP/1.1\r\nHost: ");
  client.print(host);
  client.print("\r\nAccept: application/json\r\nUser-Agent: ESP32Panel/1.0\r\nConnection: close\r\n\r\n");

  int http_code = 0;
  String payload;
  String status_line;
  if (!readHttpBody(client, http_code, payload, status_line)) {
    stats.error = "BAD " + compactSnippet(status_line);
    client.stop();
    return false;
  }
  client.stop();

  if (http_code != 200) {
    stats.error = "RAW " + String(http_code);
    return false;
  }

  return parseStatsPayload(payload, stats);
}

bool fetchStats(Stats &stats)
{
  if (WiFi.status() != WL_CONNECTED) {
    stats.error = "WIFI LOST";
    return false;
  }

  String http_client_error;
  if (fetchStatsWithHttpClient(stats)) return true;
  http_client_error = stats.error;

  if (fetchStatsWithRawClient(stats)) return true;
  stats.error = http_client_error + " / " + stats.error;
  return false;
}
