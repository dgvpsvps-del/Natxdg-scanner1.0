/*
 * Nat X Dg Scanner
 * ESP8266 (Wemos D1 mini) + OLED SSD1306 128x64 I2C + 5 tombol
 *
 * Fitur: scan WiFi + lihat kekuatan sinyal (dBm)
 * Bonus (1 fitur): LIVE SIGNAL METER - tekan Confirm di salah satu WiFi,
 *                  dBm diperbarui terus (cocok untuk cari arah antena/AP),
 *                  lengkap dengan nilai MIN / MAX.
 *
 * Pin (sesuai skematik NETHERCAP):
 *   OLED SDA -> D2 | OLED SCL -> D1
 *   Up       -> D5 | Down     -> D6
 *   Confirm  -> D7 (TJP RIGHT)
 *   Back     -> D3 (TJP LEFT)
 *   Start    -> D4 (TJP START)
 *   Semua tombol ke GND (INPUT_PULLUP)
 *
 * Kontrol:
 *   Up/Down : pilih WiFi (tahan = scroll cepat)
 *   Confirm : buka live meter WiFi terpilih
 *   Back    : kembali ke daftar
 *   Start   : scan ulang
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define FW_NAME "Nat X Dg Scanner"
#define FW_VER  "1.0"

// ---------- OLED ----------
#define SCREEN_W 128
#define SCREEN_H 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// ---------- Tombol ----------
enum { B_UP, B_DOWN, B_OK, B_BACK, B_START, B_COUNT };
const uint8_t BTN_PIN[B_COUNT] = { D5, D6, D7, D3, D4 };
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

enum Screen { SCR_BOOT, SCR_SCANNING, SCR_LIST, SCR_DETAIL };
Screen screen = SCR_BOOT;

int sel = 0, topIdx = 0;
const int ROWS = 5;

// auto scan
#define AUTO_SCAN_MS 2000     // jeda antar scan otomatis (setelah scan sebelumnya selesai)
bool     autoScan = true;     // AUTO aktif secara default
bool     bgScanning = false;  // scan latar belakang sedang berjalan
uint32_t lastScanDone = 0;
uint32_t startDownAt = 0;     // untuk deteksi tap / tahan tombol Start
bool     startLong = false;

// live meter
int      liveRssi = 0, liveMin = 0, liveMax = -200;
bool     liveFound = false;
bool     liveScanRunning = false;
uint32_t liveDrawAt = 0;

// ---------- Util ----------
const char* encName(uint8_t e) {
  switch (e) {
    case ENC_TYPE_NONE: return "OPEN";
    case ENC_TYPE_WEP:  return "WEP";
    case ENC_TYPE_TKIP: return "WPA";
    case ENC_TYPE_CCMP: return "WPA2";
    case ENC_TYPE_AUTO: return "WPA/WPA2";
    default:            return "?";
  }
}

int quality(int rssi) {  // -100..-40 dBm -> 0..100 %
  return constrain(map(rssi, -100, -40, 0, 100), 0, 100);
}

const char* strengthName(int rssi) {
  // maksimal 6 huruf supaya muat di layar (tidak turun ke baris bawah)
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

// ---------- Scan ----------
// showScreen=true : tampilkan layar SCANNING (scan pertama)
// showScreen=false: scan di latar belakang, daftar lama tetap tampil
void startScan(bool showScreen = true) {
  WiFi.scanDelete();
  WiFi.scanNetworks(true, true);  // async, tampilkan hidden SSID
  if (showScreen) screen = SCR_SCANNING;
  else            bgScanning = true;
}

void collectScan(int n, bool keepSel) {
  // ingat WiFi yang sedang dipilih supaya posisi kursor tidak loncat
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

// ---------- Tampilan ----------
// ---------- Boot bertema petir (Zeus) ----------
void drawBoltIcon(int x, int y) {  // ikon petir 10x16
  display.fillTriangle(x + 6, y, x, y + 9, x + 5, y + 9, SSD1306_WHITE);
  display.fillTriangle(x + 4, y + 7, x + 9, y + 7, x + 3, y + 15, SSD1306_WHITE);
}

void flashScreen(int ms) {
  display.invertDisplay(true);
  delay(ms);
  display.invertDisplay(false);
}

// petir zig-zag menyambar dari atas layar, acak setiap kali
void strikeBolt() {
  display.clearDisplay();
  int x = 64 + random(-10, 11), y = 0;
  while (y < SCREEN_H) {
    int nx = constrain(x + random(-14, 15), 20, 108);
    int ny = y + random(7, 13);
    display.drawLine(x, y, nx, ny, SSD1306_WHITE);
    display.drawLine(x + 1, y, nx + 1, ny, SSD1306_WHITE);
    if (random(3) == 0)  // cabang petir
      display.drawLine(nx, ny, nx + random(-16, 17), ny + random(6, 12), SSD1306_WHITE);
    x = nx;
    y = ny;
  }
  display.display();
  flashScreen(50);
}

void drawBoot(int pct) {
  pct = constrain(pct, 0, 100);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  centerText("NAT X DG", 3, 2);
  centerText("SCANNER", 21, 2);
  drawBoltIcon(1, 8);    // petir kiri
  drawBoltIcon(117, 8);  // petir kanan
  // bar loading
  int fillW = pct * 96 / 100;
  display.drawRect(14, 42, 100, 8, SSD1306_WHITE);
  display.fillRect(16, 44, fillW, 4, SSD1306_WHITE);
  if (pct > 0 && pct < 100) {  // percikan di ujung bar
    int sx = 16 + fillW;
    display.drawLine(sx, 40, sx + 3, 44, SSD1306_WHITE);
    display.drawLine(sx + 3, 44, sx, 48, SSD1306_WHITE);
    display.drawLine(sx, 48, sx + 3, 52, SSD1306_WHITE);
  }
  char t[20];
  snprintf(t, sizeof(t), "POWER UP %d%%", pct);
  centerText(t, 55, 1);
  display.display();
}

// Durasi boot total sekitar 3 detik (ubah angka ini sesuai selera)
#define BOOT_STRIKE_MS 900   // fase petir menyambar
#define BOOT_LOAD_MS   2000  // fase judul + loading

void bootAnimation() {
  uint32_t t0 = millis();
  while (millis() - t0 < BOOT_STRIKE_MS) {  // sambaran petir acak
    strikeBolt();
    delay(60);
    display.clearDisplay();
    display.display();
    delay(40 + random(80));
  }
  t0 = millis();
  bool f1 = false, f2 = false, f3 = false;
  while (true) {
    uint32_t el = millis() - t0;
    int p = constrain((int)(el * 100 / BOOT_LOAD_MS), 0, 100);
    drawBoot(p);
    if (!f1 && p >= 40)  { f1 = true; flashScreen(40); }  // kilatan
    if (!f2 && p >= 80)  { f2 = true; flashScreen(40); }
    if (p >= 100) { if (!f3) { f3 = true; flashScreen(40); } break; }
  }
  delay(150);
}

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

void drawList() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // header
  display.fillRect(0, 0, SCREEN_W, 10, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(2, 1);
  display.print("WIFI LIST");
  if (autoScan) {  // penanda AUTO
    display.setCursor(62, 1);
    display.print("AUTO");
  }
  char cnt[8];
  snprintf(cnt, sizeof(cnt), "%d", netCount);
  int cx = SCREEN_W - 2 - strlen(cnt) * 6;
  display.setCursor(cx, 1);
  display.print(cnt);
  if (bgScanning) {  // penanda sedang scan
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
    int y = 12 + r * 10;
    bool active = (idx == sel);
    if (active) display.fillRect(0, y - 1, SCREEN_W, 10, SSD1306_WHITE);
    display.setTextColor(active ? SSD1306_BLACK : SSD1306_WHITE);

    char name[20];
    shortText(name, netList[idx].ssid, 14);
    display.setCursor(2, y);
    display.print(name);

    char rs[10];
    snprintf(rs, sizeof(rs), "[%d]", (int)netList[idx].rssi);
    display.setCursor(SCREEN_W - 2 - strlen(rs) * 6, y);
    display.print(rs);
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

  // dBm besar
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

  // bar sinyal
  int q = liveFound ? quality(liveRssi) : 0;
  display.drawRect(0, 39, SCREEN_W, 7, SSD1306_WHITE);
  display.fillRect(2, 41, q * 124 / 100, 3, SSD1306_WHITE);

  // info
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
    WiFi.scanNetworks(true, true, n.ch);  // scan 1 channel saja = cepat
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

  Wire.begin(D2, D1);  // SDA, SCL
  Wire.setClock(400000);  // I2C cepat: layar lebih mulus
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();

  display.setTextWrap(false);  // teks tidak turun ke baris bawah
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  randomSeed(analogRead(A0) + micros());
  bootAnimation();
  startScan();
}

void loop() {
  switch (screen) {
    case SCR_SCANNING: {
      drawScanning();
      int r = WiFi.scanComplete();
      if (r >= 0) collectScan(r, false);
      else if (r == WIFI_SCAN_FAILED) { netCount = 0; lastScanDone = millis(); screen = SCR_LIST; }
      break;
    }

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

      // Start: tap = scan sekarang, tahan 0,9 detik = AUTO ON/OFF
      if (btnPressed(B_START)) { startDownAt = millis(); startLong = false; }
      if (btnState[B_START]) {
        if (!startLong && startDownAt && millis() - startDownAt >= 900) {
          startLong = true;
          autoScan = !autoScan;
          lastScanDone = millis();
          redraw = true;
        }
      } else if (startDownAt) {
        if (!startLong && !bgScanning) { startScan(false); redraw = true; }
        startDownAt = 0;
      }

      // hasil scan di latar belakang / scan otomatis tiap 2 detik
      if (bgScanning) {
        int r = WiFi.scanComplete();
        if (r >= 0) { collectScan(r, true); redraw = true; }
        else if (r == WIFI_SCAN_FAILED) { bgScanning = false; lastScanDone = millis(); }
      } else if (autoScan && millis() - lastScanDone >= AUTO_SCAN_MS) {
        startScan(false);
        redraw = true;
      }

      if (sel < topIdx) topIdx = sel;
      if (sel >= topIdx + ROWS) topIdx = sel - ROWS + 1;
      static uint32_t lastDraw = 0;
      if (redraw || millis() - lastDraw > 500) {
        drawList();
        lastDraw = millis();
      }
      break;
    }

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
