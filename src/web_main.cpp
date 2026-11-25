/*
 * Nat X Dg Scanner - Web (headless, tanpa OLED)  v1.4
 * Untuk ESP8266 (Wemos D1 mini) dan ESP32 / S3 / C3 / C6.
 *
 * Alat membuat WiFi sendiri (Access Point). Sambungkan HP/laptop ke WiFi itu,
 * lalu buka halaman scanner (tanpa internet):
 *     WiFi     : NatXDg-Scanner      (bisa diganti lewat halaman web)
 *     Password : 12345678            (bisa diganti lewat halaman web)
 *     Alamat   : http://192.168.4.1   atau   http://natxdg.local
 *
 * Tidak butuh pin apa pun: cukup colok USB / power bank.
 * Lupa password? Tahan tombol BOOT (GPIO0 / GPIO9 di C3 & C6; di D1 mini sambungkan
 * D3 ke GND) selama 5 detik setelah alat menyala -> kembali ke pengaturan bawaan.
 */

#include <Arduino.h>
#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266mDNS.h>
  #include <EEPROM.h>
  typedef ESP8266WebServer WebSrv;
#else
  #include <WiFi.h>
  #include <WebServer.h>
  #include <ESPmDNS.h>
  #include <Preferences.h>
  typedef WebServer WebSrv;
#endif
#include <DNSServer.h>
#include "web_page.h"
#include "donate_png.h"

#define FW_NAME "Nat X Dg Scanner"
#define FW_VER  "1.4"

#define DEF_SSID  "NatXDg-Scanner"
#define DEF_PASS  "12345678"
#define MDNS_NAME "natxdg"

#if defined(ESP8266)
  #define CHIP_NAME "ESP8266"
  #define RESET_PIN D3
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  #define CHIP_NAME "ESP32-S3"
  #define RESET_PIN 0
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  #define CHIP_NAME "ESP32-C3"
  #define RESET_PIN 9
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
  #define CHIP_NAME "ESP32-C6"
  #define RESET_PIN 9
#else
  #define CHIP_NAME "ESP32"
  #define RESET_PIN 0
#endif

WebSrv   server(80);
DNSServer dns;

// ---------- Pengaturan WiFi alat (tersimpan di flash) ----------
struct WCfg {
  uint32_t magic;
  char     ssid[33];
  char     pass[64];
  uint8_t  open;  // 1 = WiFi terbuka tanpa password
};
WCfg cfg;
#define WMAGIC 0x4E584447

uint32_t restartAt = 0;

void cfgDefault() {
  memset(&cfg, 0, sizeof(cfg));
  cfg.magic = WMAGIC;
  strcpy(cfg.ssid, DEF_SSID);
  strcpy(cfg.pass, DEF_PASS);
  cfg.open = 0;
}

void cfgLoad() {
  bool okc;
#if defined(ESP8266)
  EEPROM.begin(128);
  EEPROM.get(0, cfg);
  okc = (cfg.magic == WMAGIC);
#else
  Preferences p;
  p.begin("natxdg", true);
  size_t n = p.getBytes("wcfg", &cfg, sizeof(cfg));
  p.end();
  okc = (n == sizeof(cfg) && cfg.magic == WMAGIC);
#endif
  cfg.ssid[32] = 0;
  cfg.pass[63] = 0;
  if (!okc || !cfg.ssid[0] || (!cfg.open && strlen(cfg.pass) < 8)) cfgDefault();
}

void cfgSave() {
#if defined(ESP8266)
  EEPROM.put(0, cfg);
  EEPROM.commit();
#else
  Preferences p;
  p.begin("natxdg", false);
  p.putBytes("wcfg", &cfg, sizeof(cfg));
  p.end();
#endif
}

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
#if defined(ESP8266)
  switch (e) {
    case ENC_TYPE_NONE: return "OPEN";
    case ENC_TYPE_WEP:  return "WEP";
    case ENC_TYPE_TKIP: return "WPA";
    case ENC_TYPE_CCMP: return "WPA2";
    case ENC_TYPE_AUTO: return "WPA/WPA2";
    default:            return "?";
  }
#else
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
#endif
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
void sendJson(const String& body, int code = 200) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", body);
}

void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache");
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleDonate() {
  server.sendHeader("Cache-Control", "max-age=86400");
  server.send_P(200, "image/png", (PGM_P)DONATE_PNG, DONATE_PNG_LEN);
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
  out += (uint32_t)scanId;
  out += ",\"age\":";
  out += (uint32_t)(scanId ? (millis() - doneAt) : 0);
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
    out += (int)n.rssi;
    out += ",\"c\":";
    out += (int)n.ch;
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

void handleCfgGet() {
  String out = "{\"ssid\":";
  jsonStr(out, cfg.ssid);
  out += ",\"open\":";
  out += cfg.open ? "true" : "false";
  out += "}";
  sendJson(out);
}

void handleCfgPost() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  bool open = (server.arg("open") == "1");
  ssid.trim();

  if (ssid.length() < 1 || ssid.length() > 32) {
    sendJson("{\"ok\":false,\"err\":\"Nama WiFi harus 1-32 karakter\"}", 400);
    return;
  }
  for (unsigned i = 0; i < ssid.length(); i++) {
    if ((uint8_t)ssid[i] < 0x20) {
      sendJson("{\"ok\":false,\"err\":\"Nama WiFi mengandung karakter tidak valid\"}", 400);
      return;
    }
  }
  if (!open) {
    if (pass.length() == 0) {
      if (cfg.open) {  // sebelumnya terbuka, wajib isi password baru
        sendJson("{\"ok\":false,\"err\":\"Isi password baru (minimal 8 karakter)\"}", 400);
        return;
      }
      // kosong = password lama tidak diubah
    } else if (pass.length() < 8 || pass.length() > 63) {
      sendJson("{\"ok\":false,\"err\":\"Password harus 8-63 karakter\"}", 400);
      return;
    } else {
      strncpy(cfg.pass, pass.c_str(), 63);
      cfg.pass[63] = 0;
    }
  }
  strncpy(cfg.ssid, ssid.c_str(), 32);
  cfg.ssid[32] = 0;
  cfg.open = open ? 1 : 0;
  cfgSave();
  sendJson("{\"ok\":true}");
  restartAt = millis() + 1500;  // beri waktu agar balasan terkirim
}

void handleReset() {
  cfgDefault();
  cfgSave();
  sendJson("{\"ok\":true}");
  restartAt = millis() + 1500;
}

// semua alamat lain (cek koneksi Android/iOS/Windows) diarahkan ke halaman utama
void handleNotFound() {
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
}

// ---------- Setup / Loop ----------
void setup() {
  Serial.begin(115200);
  pinMode(RESET_PIN, INPUT_PULLUP);
  cfgLoad();

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(cfg.ssid, cfg.open ? nullptr : cfg.pass);
  delay(200);

  IPAddress ip = WiFi.softAPIP();
  dns.start(53, "*", ip);  // captive portal
#if defined(ESP8266)
  if (MDNS.begin(MDNS_NAME, ip)) MDNS.addService("http", "tcp", 80);
#else
  if (MDNS.begin(MDNS_NAME)) MDNS.addService("http", "tcp", 80);
#endif

  server.on("/", HTTP_GET, handleRoot);
  server.on("/donate.png", HTTP_GET, handleDonate);
  server.on("/api/scan", HTTP_GET, handleScan);
  server.on("/api/results", HTTP_GET, handleResults);
  server.on("/api/info", HTTP_GET, handleInfo);
  server.on("/api/config", HTTP_GET, handleCfgGet);
  server.on("/api/config", HTTP_POST, handleCfgPost);
  server.on("/api/reset", HTTP_POST, handleReset);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.printf("\n%s v%s (%s)\nWiFi: %s  Password: %s\nBuka: http://%s  atau  http://%s.local\n",
                FW_NAME, FW_VER, CHIP_NAME, cfg.ssid, cfg.open ? "(terbuka)" : cfg.pass,
                ip.toString().c_str(), MDNS_NAME);
  startScan();
}

void loop() {
  dns.processNextRequest();
  server.handleClient();
#if defined(ESP8266)
  MDNS.update();
#endif

  if (scanning) {
    int r = WiFi.scanComplete();
    if (r >= 0) {
      collectScan(r);
    } else if (r == WIFI_SCAN_FAILED || millis() - scanStart > 15000) {
      WiFi.scanDelete();
      scanning = false;
    }
  }

  // tahan tombol BOOT 5 detik -> kembali ke WiFi bawaan
  static uint32_t lowSince = 0;
  if (digitalRead(RESET_PIN) == LOW) {
    if (!lowSince) lowSince = millis();
    else if (millis() - lowSince > 5000) {
      cfgDefault();
      cfgSave();
      ESP.restart();
    }
  } else {
    lowSince = 0;
  }

  if (restartAt && (int32_t)(millis() - restartAt) >= 0) ESP.restart();
  delay(2);
}
