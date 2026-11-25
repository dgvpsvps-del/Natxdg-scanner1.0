/*
 * Nat X Dg Scanner - Web (headless, tanpa OLED)  v1.3
 * Untuk ESP32 / S3 / C3 / C6.
 *
 * ESP32 membuat WiFi sendiri (Access Point). Sambungkan HP/laptop ke WiFi itu,
 * lalu buka halaman scanner (tanpa internet):
 *     WiFi     : NatXDg-Scanner
 *     Password : natxdg123
 *     Alamat   : http://192.168.4.1   atau   http://natxdg.local
 * Halaman akan terbuka otomatis di sebagian HP (captive portal).
 *
 * Tidak butuh pin apa pun: cukup colok USB / power bank.
 * Ubah nama & password WiFi lewat AP_SSID / AP_PASS di bawah (password min. 8 huruf,
 * kosongkan "" untuk WiFi terbuka).
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include "web_page.h"

#define FW_NAME "Nat X Dg Scanner"
#define FW_VER  "1.3"

#ifndef AP_SSID
#define AP_SSID "NatXDg-Scanner"
#endif
#ifndef AP_PASS
#define AP_PASS "natxdg123"
#endif
#define MDNS_NAME "natxdg"

#if defined(CONFIG_IDF_TARGET_ESP32S3)
  #define CHIP_NAME "ESP32-S3"
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  #define CHIP_NAME "ESP32-C3"
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
  #define CHIP_NAME "ESP32-C6"
#else
  #define CHIP_NAME "ESP32"
#endif

WebServer server(80);
DNSServer dns;

#define MAX_NET 60
struct Net {
  char    ssid[33];
  int32_t rssi;
  uint8_t bssid[6];
  uint8_t ch;
  uint8_t enc;
};
Net      nets[MAX_NET];
int      netCount = 0;
uint32_t scanId = 0;      // naik setiap scan selesai
uint32_t doneAt = 0;
uint32_t scanStart = 0;
bool     scanning = false;

const char* encName(uint8_t e) {
  switch (e) {
    case WIFI_AUTH_OPEN:            return "OPEN";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-E";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/3";
    default:                        return "?";
  }
}

// ---------- JSON ----------
// SSID bisa berisi byte sembarang: escape + buang UTF-8 yang rusak supaya JSON selalu valid
static void jsonStr(String& out, const char* s) {
  out += '"';
  const uint8_t* p = (const uint8_t*)s;
  while (*p) {
    uint8_t c = *p;
    if (c == '"' || c == '\\') { out += '\\'; out += (char)c; p++; }
    else if (c < 0x20)         { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); out += b; p++; }
    else if (c < 0x80)         { out += (char)c; p++; }
    else {
      int n = (c >= 0xC2 && c <= 0xDF) ? 1 : (c >= 0xE0 && c <= 0xEF) ? 2 : (c >= 0xF0 && c <= 0xF4) ? 3 : -1;
      bool ok = n > 0;
      for (int i = 1; ok && i <= n; i++) if ((p[i] & 0xC0) != 0x80) ok = false;
      if (ok) { for (int i = 0; i <= n; i++) out += (char)p[i]; p += n + 1; }
      else    { out += '?'; p++; }
    }
  }
  out += '"';
}

// ---------- Scan ----------
void startScan() {
  if (scanning) return;
  WiFi.scanDelete();
  WiFi.scanNetworks(true, true);  // async, tampilkan SSID tersembunyi
  scanning = true;
  scanStart = millis();
}

void collectScan(int n) {
  netCount = 0;
  for (int i = 0; i < n && netCount < MAX_NET; i++) {
    Net& e = nets[netCount];
    String s = WiFi.SSID(i);
    strncpy(e.ssid, s.c_str(), 32);
    e.ssid[32] = 0;
    e.rssi = WiFi.RSSI(i);
    e.ch   = WiFi.channel(i);
    e.enc  = WiFi.encryptionType(i);
    memcpy(e.bssid, WiFi.BSSID(i), 6);
    netCount++;
  }
  WiFi.scanDelete();
  for (int i = 1; i < netCount; i++) {  // urut sinyal terkuat
    Net key = nets[i];
    int j = i - 1;
    while (j >= 0 && nets[j].rssi < key.rssi) { nets[j + 1] = nets[j]; j--; }
    nets[j + 1] = key;
  }
  scanId++;
  doneAt = millis();
  scanning = false;
}

// ---------- HTTP ----------
void sendJson(const String& body) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", body);
}

void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache");
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleScan() {
  startScan();
  sendJson("{\"scanning\":true}");
}

void handleResults() {
  String out;
  out.reserve(160 + netCount * 120);
  out += "{\"scanning\":";
  out += scanning ? "true" : "false";
  out += ",\"id\":";
  out += scanId;
  out += ",\"age\":";
  out += scanId ? (millis() - doneAt) : 0;
  out += ",\"nets\":[";
  for (int i = 0; i < netCount; i++) {
    const Net& n = nets[i];
    char mac[20];
    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
             n.bssid[0], n.bssid[1], n.bssid[2], n.bssid[3], n.bssid[4], n.bssid[5]);
    if (i) out += ',';
    out += "{\"s\":";
    jsonStr(out, n.ssid);
    out += ",\"b\":\"";
    out += mac;
    out += "\",\"r\":";
    out += n.rssi;
    out += ",\"c\":";
    out += n.ch;
    out += ",\"e\":\"";
    out += encName(n.enc);
    out += "\"}";
  }
  out += "]}";
  sendJson(out);
}

void handleInfo() {
  String out = "{\"name\":\"" FW_NAME "\",\"ver\":\"" FW_VER "\",\"chip\":\"" CHIP_NAME "\",\"up\":";
  out += (uint32_t)(millis() / 1000);
  out += ",\"heap\":";
  out += (uint32_t)ESP.getFreeHeap();
  out += ",\"cl\":";
  out += (int)WiFi.softAPgetStationNum();
  out += "}";
  sendJson(out);
}

// semua alamat lain (cek koneksi Android/iOS/Windows) diarahkan ke halaman utama
void handleNotFound() {
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
}

// ---------- Setup / Loop ----------
void setup() {
  Serial.begin(115200);
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, strlen(AP_PASS) ? AP_PASS : nullptr);
  delay(200);

  IPAddress ip = WiFi.softAPIP();
  dns.start(53, "*", ip);  // captive portal
  if (MDNS.begin(MDNS_NAME)) MDNS.addService("http", "tcp", 80);

  server.on("/", handleRoot);
  server.on("/api/scan", handleScan);
  server.on("/api/results", handleResults);
  server.on("/api/info", handleInfo);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.printf("\n%s v%s (%s)\nWiFi: %s  Password: %s\nBuka: http://%s  atau  http://%s.local\n",
                FW_NAME, FW_VER, CHIP_NAME, AP_SSID, strlen(AP_PASS) ? AP_PASS : "(terbuka)",
                ip.toString().c_str(), MDNS_NAME);
  startScan();
}

void loop() {
  dns.processNextRequest();
  server.handleClient();

  if (scanning) {
    int r = WiFi.scanComplete();
    if (r >= 0) {
      collectScan(r);
    } else if (r == WIFI_SCAN_FAILED || millis() - scanStart > 15000) {
      WiFi.scanDelete();
      scanning = false;
    }
  }
  delay(2);
}
