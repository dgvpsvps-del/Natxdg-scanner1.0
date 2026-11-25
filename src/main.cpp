/*
 * Nat X Dg Scanner  v1.2
 * ESP8266 (Wemos D1 mini) + OLED SSD1306 128x64 I2C + 5 tombol
 *
 * Fitur:
 *  - Menu utama : Scan WiFi / Setting / About
 *  - Scan WiFi  : daftar WiFi + dBm, auto-scan, Live Signal Meter
 *  - Setting    : Auto Scan, Interval, Kecerahan, Layar Mati, Reset (tersimpan di EEPROM)
 *  - Indikator baterai di header (dibaca dari A0, pembagi 100k+100k)
 *
 * Pin (sesuai skematik NETHERCAP):
 *   OLED SDA -> D2 | OLED SCL -> D1
 *   Up -> D5 | Down -> D6 | Confirm -> D7 | Back -> D3 | Start -> D4
 *   Baterai -> 100k -> A0 -> 100k -> GND
 *
 * Kontrol:
 *   Up/Down : pilih (tahan = cepat)
 *   Confirm : masuk / ubah nilai / buka live meter
 *   Back    : kembali
 *   Start   : (daftar WiFi) tap = scan sekarang, tahan 1 dtk = AUTO ON/OFF
 *             (live meter)  reset MIN/MAX
 */

#include <Arduino.h>
#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#else
  #include <WiFi.h>
#endif
#include <Wire.h>
#if defined(ESP8266)
  #include <EEPROM.h>
#else
  #include <Preferences.h>
#endif
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "logo.h"

#define FW_NAME "Nat X Dg Scanner"
#define FW_VER  "1.3"

// ---------- Pin per chip ----------
// ESP8266 mengikuti skematik NETHERCAP. Untuk ESP32 pakai tabel di bawah
// (ubah sesuai wiring kamu). Semua tombol ke GND (INPUT_PULLUP).
#if defined(ESP8266)
  #define CHIP_NAME "ESP8266"
  #define PIN_SDA   D2
  #define PIN_SCL   D1
  #define PIN_UP    D5
  #define PIN_DOWN  D6
  #define PIN_OK    D7
  #define PIN_BACK  D3
  #define PIN_START D4
  #define BAT_PIN   A0
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  #define CHIP_NAME "ESP32-S3"
  #define PIN_SDA   8
  #define PIN_SCL   9
  #define PIN_UP    4
  #define PIN_DOWN  5
  #define PIN_OK    6
  #define PIN_BACK  7
  #define PIN_START 15
  #define BAT_PIN   1
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  #define CHIP_NAME "ESP32-C3"
  #define PIN_SDA   6
  #define PIN_SCL   7
  #define PIN_UP    1
  #define PIN_DOWN  3
  #define PIN_OK    4
  #define PIN_BACK  10
  #define PIN_START 5
  #define BAT_PIN   0
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
  #define CHIP_NAME "ESP32-C6"
  #define PIN_SDA   6
  #define PIN_SCL   7
  #define PIN_UP    1
  #define PIN_DOWN  2
  #define PIN_OK    3
  #define PIN_BACK  10
  #define PIN_START 11
  #define BAT_PIN   0
#else
  #define CHIP_NAME "ESP32"
  #define PIN_SDA   21
  #define PIN_SCL   22
  #define PIN_UP    25
  #define PIN_DOWN  26
  #define PIN_OK    27
  #define PIN_BACK  14
  #define PIN_START 13
  #define BAT_PIN   34
#endif

// ---------- OLED ----------
#define SCREEN_W 128
#define SCREEN_H 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// ---------- Baterai ----------
// Pembagi tegangan: Vbat -100k- ADC -100k- GND.
// ESP8266: ditambah pembagi bawaan D1 mini (220k+100k) -> rasio 0.1351 x Vbat.
//          Kalau angka kurang pas, ubah BAT_DIV (naikkan = tegangan terbaca lebih kecil).
// ESP32  : pembagi 2:1 (BAT_RATIO = 2.0), pakai analogReadMilliVolts (sudah terkalibrasi).
#define BAT_DIV   0.1351f
#define BAT_RATIO 2.0f
float batVolt = 0;

float batRead() {
  long s = 0;
  for (int i = 0; i < 8; i++) {
#if defined(ESP8266)
    s += analogRead(BAT_PIN);
#else
    s += analogReadMilliVolts(BAT_PIN);
#endif
    delay(1);
  }
#if defined(ESP8266)
  return (s / 8.0f) / 1023.0f / BAT_DIV;
#else
  return (s / 8.0f) * BAT_RATIO / 1000.0f;
#endif
}

void batUpdate(bool first) {
  static uint32_t t = 0;
  if (!first && millis() - t < 1000) return;
  t = millis();
  float v = batRead();
  batVolt = first ? v : (batVolt * 0.7f + v * 0.3f);
}

int batPct() {  // kurva kasar Li-ion 18650
  static const float V[] = { 4.20f, 4.10f, 4.00f, 3.90f, 3.80f, 3.70f, 3.60f, 3.50f, 3.30f };
  static const int   P[] = { 100,   88,    75,    58,    40,    22,    10,    5,     0 };
  if (batVolt >= V[0]) return 100;
  for (int i = 1; i < 9; i++) {
    if (batVolt >= V[i]) {
      float f = (batVolt - V[i]) / (V[i - 1] - V[i]);
      return P[i] + (int)(f * (P[i - 1] - P[i]));
    }
  }
  return 0;
}

// ---------- Setting (disimpan di EEPROM) ----------
struct Cfg {
  uint8_t magic;
  uint8_t autoScan;  // 0/1
  uint8_t interval;  // index INTERVAL_MS
  uint8_t bright;    // index BRIGHT_VAL
  uint8_t sleep;     // index SLEEP_MS
};
Cfg cfg;
#define CFG_MAGIC 0xA7
const uint32_t INTERVAL_MS[3]  = { 2000, 5000, 10000 };
const char*    INTERVAL_TXT[3] = { "2 dtk", "5 dtk", "10 dtk" };
const uint8_t  BRIGHT_VAL[3]   = { 0x10, 0x80, 0xFF };
const char*    BRIGHT_TXT[3]   = { "RENDAH", "SEDANG", "TERANG" };
const uint32_t SLEEP_MS[4]     = { 0, 30000, 60000, 300000 };
const char*    SLEEP_TXT[4]    = { "OFF", "30 dtk", "1 mnt", "5 mnt" };

void cfgDefault() {
  cfg.magic = CFG_MAGIC;
  cfg.autoScan = 1;
  cfg.interval = 0;
  cfg.bright = 2;
  cfg.sleep = 0;
}
void cfgLoad() {
#if defined(ESP8266)
  EEPROM.begin(16);
  EEPROM.get(0, cfg);
#else
  Preferences p;
  p.begin("natxdg", true);
  if (p.getBytes("cfg", &cfg, sizeof(cfg)) != sizeof(cfg)) cfg.magic = 0;
  p.end();
#endif
  if (cfg.magic != CFG_MAGIC || cfg.interval > 2 || cfg.bright > 2 || cfg.sleep > 3)
    cfgDefault();
}
void cfgSave() {
#if defined(ESP8266)
  EEPROM.put(0, cfg);
  EEPROM.commit();
#else
  Preferences p;
  p.begin("natxdg", false);
  p.putBytes("cfg", &cfg, sizeof(cfg));
  p.end();
#endif
}
void applyBrightness() {
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(BRIGHT_VAL[cfg.bright]);
}

// ---------- Tombol ----------
enum { B_UP, B_DOWN, B_OK, B_BACK, B_START, B_COUNT };
const uint8_t BTN_PIN[B_COUNT] = { PIN_UP, PIN_DOWN, PIN_OK, PIN_BACK, PIN_START };
bool     btnState[B_COUNT];
uint32_t btnSince[B_COUNT];
uint32_t btnRepeat[B_COUNT];

bool btnPressed(int i) {
  bool down = (digitalRead(BTN_PIN[i]) == LOW);
  uint32_t now = millis();
  if (down != btnState[i]) {
    if (now - btnSince[i] > 25) {
      btnState[i] = down;
      btnSince[i] = now;
      if (down) {
        btnRepeat[i] = now + 400;
        return true;
      }
    }
  } else if (down && (i == B_UP || i == B_DOWN) && now >= btnRepeat[i]) {
    btnRepeat[i] = now + 110;
    return true;
  }
  return false;
}

// ---------- Data WiFi ----------
#define MAX_NET 40
struct Net {
  char    ssid[33];
  int32_t rssi;
  uint8_t bssid[6];
  uint8_t ch;
  uint8_t enc;
};
Net netList[MAX_NET];
int netCount = 0;

enum Screen { SCR_BOOT, SCR_MENU, SCR_SCANNING, SCR_LIST, SCR_DETAIL, SCR_SETTINGS, SCR_ABOUT };
Screen screen = SCR_BOOT;

int sel = 0, topIdx = 0;
const int ROWS = 5;
int menuSel = 0;
int setSel = 0;

// auto scan
bool     bgScanning = false;  // scan latar belakang sedang berjalan
uint32_t lastScanDone = 0;
uint32_t startDownAt = 0;     // deteksi tap / tahan tombol Start
bool     startLong = false;

// layar mati otomatis
uint32_t lastActivity = 0;
bool     screenOff = false;
bool     swallow = false;

// live meter
int      liveRssi = 0, liveMin = 0, liveMax = -200;
bool     liveFound = false;
bool     liveScanRunning = false;
uint32_t liveDrawAt = 0;

// ---------- Util ----------
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

int quality(int rssi) {  // -100..-40 dBm -> 0..100 %
  return constrain(map(rssi, -100, -40, 0, 100), 0, 100);
}

const char* strengthName(int rssi) {
  // maksimal 6 huruf supaya muat di layar
  if (rssi >= -50) return "MANTAP";
  if (rssi >= -60) return "BAGUS";
  if (rssi >= -70) return "CUKUP";
  if (rssi >= -80) return "LEMAH";
  return "BURUK";
}

void shortText(char* out, const char* in, size_t maxChars) {
  size_t n = strlen(in);
  if (n == 0) { strcpy(out, "<hidden>"); return; }
  if (n <= maxChars) { strcpy(out, in); return; }
  memcpy(out, in, maxChars);
  out[maxChars] = 0;
}

void centerText(const char* t, int y, uint8_t size) {
  display.setTextSize(size);
  int w = strlen(t) * 6 * size;
  display.setCursor((SCREEN_W - w) / 2, y);
  display.print(t);
}

void rightText(const char* t, int xEnd, int y) {
  display.setCursor(xEnd - (int)strlen(t) * 6, y);
  display.print(t);
}

// ---------- Scan ----------
// Mulai scan async. ch=0 -> semua channel, ch>0 -> satu channel saja (cepat).
void wifiScan(uint8_t ch) {
#if defined(ESP8266)
  WiFi.scanNetworks(true, true, ch);
#else
  WiFi.scanNetworks(true, true, false, 300, ch);
#endif
}

// showScreen=true : tampilkan layar SCANNING
// showScreen=false: scan di latar belakang, daftar lama tetap tampil
void startScan(bool showScreen = true) {
  WiFi.scanDelete();
  wifiScan(0);  // async, tampilkan hidden SSID
  if (showScreen) screen = SCR_SCANNING;
  else            bgScanning = true;
}

void collectScan(int n, bool keepSel) {
  // ingat WiFi yang sedang dipilih supaya kursor tidak loncat
  uint8_t keep[6];
  bool hadSel = keepSel && netCount > 0 && sel < netCount;
  if (hadSel) memcpy(keep, netList[sel].bssid, 6);

  netCount = 0;
  for (int i = 0; i < n && netCount < MAX_NET; i++) {
    Net& e = netList[netCount];
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
  // urutkan dari sinyal terkuat
  for (int i = 1; i < netCount; i++) {
    Net key = netList[i];
    int j = i - 1;
    while (j >= 0 && netList[j].rssi < key.rssi) {
      netList[j + 1] = netList[j];
      j--;
    }
    netList[j + 1] = key;
  }
  sel = 0;
  if (hadSel) {
    for (int i = 0; i < netCount; i++) {
      if (memcmp(netList[i].bssid, keep, 6) == 0) { sel = i; break; }
    }
  } else {
    topIdx = 0;
  }
  if (topIdx > sel) topIdx = sel;
  if (sel >= topIdx + ROWS) topIdx = sel - ROWS + 1;
  if (topIdx < 0) topIdx = 0;
  bgScanning = false;
  lastScanDone = millis();
  screen = SCR_LIST;
}

// ---------- Elemen tampilan ----------
void drawBatt(int x, int y, uint16_t col) {
  display.drawRect(x, y, 14, 7, col);
  display.fillRect(x + 14, y + 2, 2, 3, col);
  int w = batPct() * 10 / 100;
  if (w > 0) display.fillRect(x + 2, y + 2, w, 3, col);
}

void drawHeader(const char* title) {
  display.fillRect(0, 0, SCREEN_W, 10, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(2, 1);
  display.print(title);
  drawBatt(110, 1, SSD1306_BLACK);
}

// satu baris menu/pengaturan (tinggi 10 px)
void drawRow(int y, bool active, const char* left, const char* right) {
  if (active) display.fillRect(0, y - 1, SCREEN_W, 10, SSD1306_WHITE);
  display.setTextColor(active ? SSD1306_BLACK : SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(2, y);
  display.print(left);
  if (right) rightText(right, SCREEN_W - 2, y);
}

// ---------- Boot dengan logo DG ----------
void flashScreen(int ms) {
  display.invertDisplay(true);
  delay(ms);
  display.invertDisplay(false);
}

void drawLogoBig() {
  display.clearDisplay();
  display.drawBitmap((SCREEN_W - LOGO_BIG_W) / 2, (SCREEN_H - LOGO_BIG_H) / 2,
                     LOGO_BIG, LOGO_BIG_W, LOGO_BIG_H, SSD1306_WHITE);
  display.display();
}

void drawBoot(int pct) {
  pct = constrain(pct, 0, 100);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.drawBitmap((SCREEN_W - LOGO_SMALL_W) / 2, 0,
                     LOGO_SMALL, LOGO_SMALL_W, LOGO_SMALL_H, SSD1306_WHITE);
  display.drawRect(14, 49, 100, 6, SSD1306_WHITE);
  display.fillRect(16, 51, pct * 96 / 100, 2, SSD1306_WHITE);
  centerText("NAT X DG SCANNER", 57, 1);
  display.display();
}

// Durasi boot total sekitar 3 detik (ubah sesuai selera)
#define BOOT_LOGO_MS 1200  // logo besar
#define BOOT_LOAD_MS 1800  // logo + loading bar

void bootAnimation() {
  uint32_t t0 = millis();
  for (int i = 0; i < 3; i++) {  // logo berkedip seperti petir lalu menyala
    display.clearDisplay();
    display.display();
    delay(50 + random(60));
    drawLogoBig();
    flashScreen(40);
    delay(60);
  }
  drawLogoBig();
  while (millis() - t0 < BOOT_LOGO_MS) delay(10);

  t0 = millis();
  while (true) {
    int p = constrain((int)((millis() - t0) * 100 / BOOT_LOAD_MS), 0, 100);
    drawBoot(p);
    if (p >= 100) break;
  }
  delay(150);
}

// ---------- Layar ----------
void drawScanning() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  centerText("SCANNING", 16, 2);
  int dots = (millis() / 300) % 4;
  char d[5] = "";
  for (int i = 0; i < dots; i++) strcat(d, ".");
  centerText(d, 40, 2);
  display.display();
}

const char* MENU_TXT[3] = { "Scan WiFi", "Setting", "About" };

void drawMenu() {
  display.clearDisplay();
  drawHeader("MENU");
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  char b[8];
  snprintf(b, sizeof(b), "%d%%", batPct());
  rightText(b, 106, 1);
  for (int i = 0; i < 3; i++) {
    int y = 13 + i * 17;
    bool active = (i == menuSel);
    if (active) display.fillRect(0, y - 1, SCREEN_W, 17, SSD1306_WHITE);
    display.setTextColor(active ? SSD1306_BLACK : SSD1306_WHITE);
    centerText(MENU_TXT[i], y, 2);
  }
  display.display();
}

void drawSettings() {
  display.clearDisplay();
  drawHeader("SETTING");
  const char* labels[5] = { "Auto Scan", "Interval", "Kecerahan", "Layar Mati", "Reset" };
  const char* vals[5] = {
    cfg.autoScan ? "ON" : "OFF",
    INTERVAL_TXT[cfg.interval],
    BRIGHT_TXT[cfg.bright],
    SLEEP_TXT[cfg.sleep],
    "OK"
  };
  for (int i = 0; i < 5; i++) drawRow(12 + i * 10, i == setSel, labels[i], vals[i]);
  display.display();
}

void drawAbout() {
  display.clearDisplay();
  drawHeader("ABOUT");
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  char b[28];
  display.setCursor(2, 14);
  display.print(FW_NAME);
  snprintf(b, sizeof(b), "Versi %s", FW_VER);
  display.setCursor(2, 24);
  display.print(b);
  snprintf(b, sizeof(b), "Baterai %.2fV %d%%", batVolt, batPct());
  display.setCursor(2, 34);
  display.print(b);
  display.setCursor(2, 44);
  snprintf(b, sizeof(b), "%s WiFi Scan", CHIP_NAME);
  display.print(b);
  snprintf(b, sizeof(b), "RAM bebas %uKB", (unsigned)(ESP.getFreeHeap() / 1024));
  display.setCursor(2, 54);
  display.print(b);
  display.display();
}

void drawList() {
  display.clearDisplay();
  drawHeader("WIFI LIST");
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  if (cfg.autoScan) {
    display.setCursor(58, 1);
    display.print("AUTO");
  }
  char cnt[8];
  snprintf(cnt, sizeof(cnt), "%d", netCount);
  int cx = 106 - (int)strlen(cnt) * 6;
  display.setCursor(cx, 1);
  display.print(cnt);
  if (bgScanning) {
    display.setCursor(cx - 8, 1);
    display.print("*");
  }

  if (netCount == 0) {
    display.setTextColor(SSD1306_WHITE);
    centerText("TIDAK ADA WIFI", 28, 1);
    display.display();
    return;
  }

  for (int r = 0; r < ROWS; r++) {
    int idx = topIdx + r;
    if (idx >= netCount) break;
    char name[20];
    shortText(name, netList[idx].ssid, 14);
    char rs[10];
    snprintf(rs, sizeof(rs), "[%d]", (int)netList[idx].rssi);
    drawRow(12 + r * 10, idx == sel, name, rs);
  }
  display.display();
}

void drawDetail() {
  Net& n = netList[sel];
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  char name[24];
  shortText(name, n.ssid, 21);
  display.setCursor(0, 0);
  display.print(name);
  display.drawFastHLine(0, 9, SCREEN_W, SSD1306_WHITE);

  char big[8];
  if (liveFound) snprintf(big, sizeof(big), "%d", liveRssi);
  else           strcpy(big, "--");
  display.setTextSize(3);
  display.setCursor(0, 13);
  display.print(big);

  display.setTextSize(1);
  int bx = strlen(big) * 18 + 6;
  display.setCursor(bx, 15);
  display.print("dBm");
  display.setCursor(bx, 27);
  if (liveFound) display.print(strengthName(liveRssi));
  else           display.print("HILANG");

  int q = liveFound ? quality(liveRssi) : 0;
  display.drawRect(0, 39, SCREEN_W, 7, SSD1306_WHITE);
  display.fillRect(2, 41, q * 124 / 100, 3, SSD1306_WHITE);

  char line[32];
  snprintf(line, sizeof(line), "CH%d %s %d%%", n.ch, encName(n.enc), q);
  display.setCursor(0, 49);
  display.print(line);

  if (liveMax > -200) snprintf(line, sizeof(line), "MIN %d  MAX %d", liveMin, liveMax);
  else                strcpy(line, "MIN --  MAX --");
  display.setCursor(0, 57);
  display.print(line);
  display.display();
}

// ---------- Live meter ----------
void liveStart() {
  liveRssi = netList[sel].rssi;
  liveMin = liveMax = liveRssi;
  liveFound = true;
  liveScanRunning = false;
  bgScanning = false;
  startDownAt = 0;
  screen = SCR_DETAIL;
}

void liveUpdate() {
  Net& n = netList[sel];
  if (!liveScanRunning) {
    if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) return;  // tunggu scan lama selesai
    WiFi.scanDelete();
    wifiScan(n.ch);  // scan 1 channel saja = cepat
    liveScanRunning = true;
    return;
  }
  int r = WiFi.scanComplete();
  if (r == WIFI_SCAN_RUNNING) return;
  liveScanRunning = false;
  if (r < 0) return;

  bool found = false;
  for (int i = 0; i < r; i++) {
    if (memcmp(WiFi.BSSID(i), n.bssid, 6) == 0) {
      liveRssi = WiFi.RSSI(i);
      found = true;
      break;
    }
  }
  WiFi.scanDelete();
  liveFound = found;
  if (found) {
    if (liveRssi < liveMin) liveMin = liveRssi;
    if (liveRssi > liveMax) liveMax = liveRssi;
  }
}

// ---------- Setup / Loop ----------
void setup() {
  for (int i = 0; i < B_COUNT; i++) {
    pinMode(BTN_PIN[i], INPUT_PULLUP);
    btnState[i] = false;
    btnSince[i] = 0;
  }
  batUpdate(true);
  cfgLoad();

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.setTextWrap(false);
  applyBrightness();
  display.clearDisplay();

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  randomSeed(analogRead(BAT_PIN) + micros());
  bootAnimation();
  lastActivity = millis();
  screen = SCR_MENU;
}

void loop() {
  batUpdate(false);

  // ----- layar mati otomatis -----
  bool anyDown = false;
  for (int i = 0; i < B_COUNT; i++)
    if (digitalRead(BTN_PIN[i]) == LOW) anyDown = true;
  if (anyDown) {
    if (screenOff) {  // tombol pertama hanya membangunkan layar
      display.ssd1306_command(SSD1306_DISPLAYON);
      screenOff = false;
      swallow = true;
    }
    lastActivity = millis();
  }
  if (screen == SCR_DETAIL) lastActivity = millis();  // live meter tidak pernah tidur
  if (swallow) {
    if (!anyDown) swallow = false;
    delay(5);
    return;
  }
  if (!screenOff && SLEEP_MS[cfg.sleep] && millis() - lastActivity > SLEEP_MS[cfg.sleep]) {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    screenOff = true;
  }

  static uint32_t lastDraw = 0;

  switch (screen) {
    // ===== MENU UTAMA =====
    case SCR_MENU: {
      bool redraw = false;
      if (btnPressed(B_UP))   { menuSel = (menuSel + 2) % 3; redraw = true; }
      if (btnPressed(B_DOWN)) { menuSel = (menuSel + 1) % 3; redraw = true; }
      if (btnPressed(B_OK)) {
        if (menuSel == 0)      { startScan(true); netCount = 0; }
        else if (menuSel == 1) { setSel = 0; screen = SCR_SETTINGS; }
        else                   { screen = SCR_ABOUT; }
        break;
      }
      if (redraw || millis() - lastDraw > 500) { drawMenu(); lastDraw = millis(); }
      break;
    }

    // ===== SETTING =====
    case SCR_SETTINGS: {
      bool redraw = false;
      if (btnPressed(B_UP))   { setSel = (setSel + 4) % 5; redraw = true; }
      if (btnPressed(B_DOWN)) { setSel = (setSel + 1) % 5; redraw = true; }
      if (btnPressed(B_OK)) {
        switch (setSel) {
          case 0: cfg.autoScan = !cfg.autoScan; break;
          case 1: cfg.interval = (cfg.interval + 1) % 3; break;
          case 2: cfg.bright = (cfg.bright + 1) % 3; applyBrightness(); break;
          case 3: cfg.sleep = (cfg.sleep + 1) % 4; break;
          case 4: cfgDefault(); applyBrightness(); break;
        }
        redraw = true;
      }
      if (btnPressed(B_BACK)) { cfgSave(); screen = SCR_MENU; break; }
      if (redraw || millis() - lastDraw > 500) { drawSettings(); lastDraw = millis(); }
      break;
    }

    // ===== ABOUT =====
    case SCR_ABOUT: {
      if (btnPressed(B_BACK) || btnPressed(B_OK)) { screen = SCR_MENU; break; }
      if (millis() - lastDraw > 500) { drawAbout(); lastDraw = millis(); }
      break;
    }

    // ===== SCANNING (scan pertama) =====
    case SCR_SCANNING: {
      if (btnPressed(B_BACK)) {
        WiFi.scanDelete();
        screen = SCR_MENU;
        break;
      }
      drawScanning();
      int r = WiFi.scanComplete();
      if (r >= 0) collectScan(r, false);
      else if (r == WIFI_SCAN_FAILED) { netCount = 0; lastScanDone = millis(); screen = SCR_LIST; }
      break;
    }

    // ===== DAFTAR WIFI =====
    case SCR_LIST: {
      bool redraw = false;
      if (btnPressed(B_UP) && netCount > 0) {
        sel = (sel - 1 + netCount) % netCount;
        redraw = true;
      }
      if (btnPressed(B_DOWN) && netCount > 0) {
        sel = (sel + 1) % netCount;
        redraw = true;
      }
      if (btnPressed(B_OK) && netCount > 0) { liveStart(); break; }
      if (btnPressed(B_BACK)) {
        WiFi.scanDelete();
        bgScanning = false;
        startDownAt = 0;
        screen = SCR_MENU;
        break;
      }

      // Start: tap = scan sekarang, tahan 0,9 detik = AUTO ON/OFF
      if (btnPressed(B_START)) { startDownAt = millis(); startLong = false; }
      if (btnState[B_START]) {
        if (!startLong && startDownAt && millis() - startDownAt >= 900) {
          startLong = true;
          cfg.autoScan = !cfg.autoScan;
          cfgSave();
          lastScanDone = millis();
          redraw = true;
        }
      } else if (startDownAt) {
        if (!startLong && !bgScanning) { startScan(false); redraw = true; }
        startDownAt = 0;
      }

      // hasil scan di latar belakang / auto scan
      if (bgScanning) {
        int r = WiFi.scanComplete();
        if (r >= 0) { collectScan(r, true); redraw = true; }
        else if (r == WIFI_SCAN_FAILED) { bgScanning = false; lastScanDone = millis(); }
      } else if (cfg.autoScan && millis() - lastScanDone >= INTERVAL_MS[cfg.interval]) {
        startScan(false);
        redraw = true;
      }

      if (sel < topIdx) topIdx = sel;
      if (sel >= topIdx + ROWS) topIdx = sel - ROWS + 1;
      if (redraw || millis() - lastDraw > 500) {
        drawList();
        lastDraw = millis();
      }
      break;
    }

    // ===== LIVE METER =====
    case SCR_DETAIL: {
      if (btnPressed(B_BACK)) {
        WiFi.scanDelete();
        bgScanning = false;
        lastScanDone = millis();
        startDownAt = 0;
        screen = SCR_LIST;
        drawList();
        break;
      }
      if (btnPressed(B_START)) {  // reset min/max
        liveMin = liveMax = liveRssi;
      }
      liveUpdate();
      if (millis() - liveDrawAt > 100) {
        drawDetail();
        liveDrawAt = millis();
      }
      break;
    }

    default: break;
  }
  delay(1);
}
