#include "ControlServer.h"

#include "DisplayPanel.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiServer.h>

static WiFiServer server(80);
static bool server_started = false;

static void sendHeader(WiFiClient &client, const char *content_type)
{
  client.print("HTTP/1.0 200 OK\r\nContent-Type: ");
  client.print(content_type);
  client.print("\r\nConnection: close\r\nCache-Control: no-store\r\n\r\n");
}

static void sendNotFound(WiFiClient &client)
{
  client.print("HTTP/1.0 404 Not Found\r\nConnection: close\r\n\r\nnot found\n");
}

static void sendStatusJson(WiFiClient &client, const Stats &stats)
{
  sendHeader(client, "application/json");
  client.print("{\"ip\":\"");
  client.print(WiFi.localIP());
  client.print("\",\"rssi\":");
  client.print(WiFi.RSSI());
  client.print(",\"backlight\":");
  client.print(DisplayPanel::backlightEnabled() ? "true" : "false");
  client.print(",\"online\":");
  client.print(stats.online ? "true" : "false");
  client.print(",\"error\":\"");
  client.print(stats.error);
  client.print("\",\"cpu\":");
  client.print(stats.cpu);
  client.print(",\"mem\":");
  client.print(stats.mem);
  client.print(",\"conn\":");
  client.print(stats.conn);
  client.print("}\n");
}

static void sendHome(WiFiClient &client)
{
  sendHeader(client, "text/html; charset=utf-8");
  client.print("<!doctype html><meta name='viewport' content='width=device-width,initial-scale=1'>");
  client.print("<title>Home Net Panel</title>");
  client.print("<style>body{font-family:system-ui;margin:28px;max-width:520px;background:#101418;color:#eaf2f7}");
  client.print("a,button{display:block;width:100%;box-sizing:border-box;margin:12px 0;padding:14px;border:1px solid #3b4a55;border-radius:8px;background:#17212a;color:#eaf2f7;text-align:center;text-decoration:none;font-size:16px}");
  client.print("code{color:#8ee8ff}</style>");
  client.print("<h2>Home Net Panel</h2>");
  client.print("<p><code>/api/status</code> 查看状态</p>");
  client.print("<a href='/api/backlight/on'>背光开启</a>");
  client.print("<a href='/api/backlight/off'>背光关闭</a>");
  client.print("<a href='/api/backlight/toggle'>背光切换</a>");
  client.print("<a href='/api/reinit'>重初始化显示</a>");
}

static String readRequestLine(WiFiClient &client)
{
  String line;
  uint32_t start_ms = millis();
  while (millis() - start_ms < 200) {
    while (client.available()) {
      char c = (char)client.read();
      if (c == '\r') continue;
      if (c == '\n') return line;
      if (line.length() < 160) line += c;
    }
    if (!client.connected()) break;
    delay(1);
  }
  return line;
}

static String requestPath(const String &request_line)
{
  int first_space = request_line.indexOf(' ');
  if (first_space < 0) return "/";

  int second_space = request_line.indexOf(' ', first_space + 1);
  String path = second_space > first_space
                    ? request_line.substring(first_space + 1, second_space)
                    : request_line.substring(first_space + 1);

  if (path.startsWith("http://")) {
    int slash = path.indexOf('/', 7);
    path = slash >= 0 ? path.substring(slash) : "/";
  }

  int query = path.indexOf('?');
  if (query >= 0) path = path.substring(0, query);
  path.trim();
  return path;
}

void ControlServer::begin()
{
  if (server_started) return;
  server.begin();
  server_started = true;
}

void ControlServer::handle(const Stats &stats)
{
  if (!server_started || WiFi.status() != WL_CONNECTED) return;

  WiFiClient client = server.available();
  if (!client) return;

  client.setTimeout(35);
  String request_line = readRequestLine(client);
  String path = requestPath(request_line);

  if (path == "/") {
    sendHome(client);
  } else if (path == "/api/status") {
    sendStatusJson(client, stats);
  } else if (path == "/api/backlight/on") {
    DisplayPanel::setBacklight(true);
    sendStatusJson(client, stats);
  } else if (path == "/api/backlight/off") {
    DisplayPanel::setBacklight(false);
    sendStatusJson(client, stats);
  } else if (path == "/api/backlight/toggle") {
    DisplayPanel::setBacklight(!DisplayPanel::backlightEnabled());
    sendStatusJson(client, stats);
  } else if (path == "/api/reinit") {
    DisplayPanel::reinitialize();
    sendStatusJson(client, stats);
  } else {
    sendNotFound(client);
  }

  client.stop();
}
