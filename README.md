# Nat X Dg Scanner

Firmware scan WiFi untuk ESP8266 / Wemos D1 mini + OLED 128x64 + 5 tombol (skematik NETHERCAP).

## Pin
| Fungsi | Pin |
|---|---|
| OLED SDA / SCL | D2 / D1 |
| Up | D5 |
| Down | D6 |
| Confirm (TJP RIGHT) | D7 |
| Back (TJP LEFT) | D3 |
| Start (TJP START) | D4 |

## Kontrol
- **Up/Down** pilih WiFi (tahan = cepat)
- **Confirm** buka Live Signal Meter (dBm real-time + MIN/MAX)
- **Back** kembali ke daftar
- **Start** tap = scan sekarang, **tahan ~1 detik = AUTO ON/OFF** (auto scan tiap 2 detik, default ON; tanda `AUTO` di header, `*` saat scanning). Di live meter: reset MIN/MAX

## Build .bin
```
pip install platformio
pio run
# hasil: .pio/build/d1_mini/firmware.bin
```
Atau push folder ini ke GitHub, lalu ambil `NatXDgScanner_ESP8266.bin` dari tab Actions > Artifacts.

## Flash
```
esptool.py --port COMx write_flash 0x0 NatXDgScanner.bin
```
(Windows: ganti COMx, Linux: /dev/ttyUSB0). Atau pakai ESP Web Flasher / NodeMCU PyFlasher, alamat 0x0.

## Arduino IDE
Salin `src/main.cpp` ke sketch `.ino`. Board: LOLIN(WEMOS) D1 R2 & mini. Library: Adafruit GFX, Adafruit SSD1306, Adafruit BusIO.

## Flash dari HP Android (app ESP Flasher)
1. Dapatkan `NatXDgScanner_ESP8266.bin` (lihat "Build .bin" di atas; paling gampang lewat GitHub Actions).
2. Sambungkan Wemos ke HP pakai kabel USB OTG.
3. Buka ESP Flasher -> pilih file `NatXDgScanner_ESP8266.bin`, alamat **0x0**.
4. Mode flash: **DIO**, ukuran 4MB, kecepatan 921600 (kalau gagal turunkan ke 115200).
5. Tekan Flash, tunggu 100%, lalu reset/cabut-colok board.

## Durasi boot
Atur `BOOT_STRIKE_MS` dan `BOOT_LOAD_MS` di `src/main.cpp` (default total ±3 detik).

## Menu (v1.2)
- **Scan WiFi** - daftar WiFi + dBm, auto-scan, live meter
- **Setting** - Auto Scan ON/OFF, Interval (2/5/10 dtk), Kecerahan, Layar Mati (OFF/30dtk/1mnt/5mnt), Reset. Tersimpan otomatis saat tekan Back.
- **About** - versi, tegangan/persen baterai, RAM bebas
- Indikator baterai di header (A0). Kalau angkanya kurang pas, ubah `BAT_DIV` di `src/main.cpp`.

## Chip yang didukung (v1.3)
| Chip | File .bin | Flash di alamat |
|---|---|---|
| ESP8266 / Wemos D1 mini | `NatXDgScanner_ESP8266.bin` | `0x0` |
| ESP32 (DevKit biasa) | `NatXDgScanner_ESP32.bin` | `0x0` |
| ESP32-S3 | `NatXDgScanner_ESP32-S3.bin` | `0x0` |
| ESP32-C3 | `NatXDgScanner_ESP32-C3.bin` | `0x0` |
| ESP32-C6 | `NatXDgScanner_ESP32-C6.bin` | `0x0` |

File ESP32 adalah image gabungan (bootloader + partisi + firmware), jadi cukup satu file di alamat 0x0.
Versi ESP32 **belum diuji di perangkat asli** (baru lolos build), mohon laporkan lewat Issues.

### Pin per chip (ubah di bagian "Pin per chip" pada `src/main.cpp`)
| Fungsi | ESP8266 | ESP32 | ESP32-S3 | ESP32-C3 | ESP32-C6 |
|---|---|---|---|---|---|
| OLED SDA | D2 | 21 | 8 | 6 | 6 |
| OLED SCL | D1 | 22 | 9 | 7 | 7 |
| Up | D5 | 25 | 4 | 1 | 1 |
| Down | D6 | 26 | 5 | 3 | 2 |
| Confirm | D7 | 27 | 6 | 4 | 3 |
| Back | D3 | 14 | 7 | 10 | 10 |
| Start | D4 | 13 | 15 | 5 | 11 |
| Baterai (ADC) | A0 | 34 | 1 | 0 | 0 |

Baterai pada ESP32: Vbat - 100k - pin ADC - 100k - GND (rasio 2:1, tegangan dibaca langsung dalam mV).

## Build
`pio run` (semua chip) atau `pio run -e esp32s3` (satu chip: esp8266, esp32, esp32s3, esp32c3, esp32c6).

## Versi Web (ESP32 tanpa OLED, tanpa tombol)
Firmware `NatXDgScanner-Web_*.bin` membuat ESP32 jadi scanner WiFi yang dibuka lewat browser, **tanpa internet** dan **tanpa pin apa pun** (cukup colok USB / power bank).

| Chip | File .bin | Alamat flash |
|---|---|---|
| ESP32 | `NatXDgScanner-Web_ESP32.bin` | `0x0` |
| ESP32-S3 | `NatXDgScanner-Web_ESP32-S3.bin` | `0x0` |
| ESP32-C3 | `NatXDgScanner-Web_ESP32-C3.bin` | `0x0` |
| ESP32-C6 | `NatXDgScanner-Web_ESP32-C6.bin` | `0x0` |

**Cara pakai**
1. Flash file `.bin` yang sesuai chip, lalu nyalakan ESP32.
2. Di HP/laptop, sambungkan ke WiFi **`NatXDg-Scanner`**, password **`natxdg123`**.
3. Buka **http://192.168.4.1** (atau **http://natxdg.local**). Di banyak HP halaman terbuka otomatis.

**Isi halaman**
- Daftar WiFi + kekuatan sinyal (dBm), urut sinyal / nama / channel, pencarian, filter sinyal lemah
- Tap WiFi: grafik riwayat sinyal real-time + MIN / RATA-RATA / MAX (untuk mencari arah antena)
- Tab **Channel**: grafik keramaian channel 1-13 + rekomendasi channel router
- Auto scan 3 / 5 / 10 detik, export CSV, mode terang / gelap otomatis
- Semua file halaman tertanam di firmware (tanpa CDN)

**Catatan**
- Saat scan berjalan (~2 detik), WiFi milik ESP32 bisa putus-nyambung sebentar. Itu normal untuk chip ber-radio tunggal; halaman menyambung lagi otomatis.
- Ganti nama / password WiFi lewat `AP_SSID` dan `AP_PASS` di `src/web_main.cpp`. **Sebaiknya ganti password bawaan.**
- Ubah tampilan di `web/index.html`, lalu jalankan `python3 tools/gen_web.py` untuk membuat ulang `src/web_page.h`.
- Hanya membaca sinyal WiFi di sekitar (scan pasif), tidak menyambung ke atau mengganggu jaringan lain.
