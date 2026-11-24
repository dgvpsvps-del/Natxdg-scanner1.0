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
- **Start** scan ulang (di live meter: reset MIN/MAX)

## Build .bin
```
pip install platformio
pio run
# hasil: .pio/build/d1_mini/firmware.bin
```
Atau push folder ini ke GitHub, lalu ambil `NatXDgScanner.bin` dari tab Actions > Artifacts.

## Flash
```
esptool.py --port COMx write_flash 0x0 NatXDgScanner.bin
```
(Windows: ganti COMx, Linux: /dev/ttyUSB0). Atau pakai ESP Web Flasher / NodeMCU PyFlasher, alamat 0x0.

## Arduino IDE
Salin `src/main.cpp` ke sketch `.ino`. Board: LOLIN(WEMOS) D1 R2 & mini. Library: Adafruit GFX, Adafruit SSD1306, Adafruit BusIO.

## Flash dari HP Android (app ESP Flasher)
1. Dapatkan `NatXDgScanner.bin` (lihat "Build .bin" di atas; paling gampang lewat GitHub Actions).
2. Sambungkan Wemos ke HP pakai kabel USB OTG.
3. Buka ESP Flasher -> pilih file `NatXDgScanner.bin`, alamat **0x0**.
4. Mode flash: **DIO**, ukuran 4MB, kecepatan 921600 (kalau gagal turunkan ke 115200).
5. Tekan Flash, tunggu 100%, lalu reset/cabut-colok board.

## Durasi boot
Atur `BOOT_STRIKE_MS` dan `BOOT_LOAD_MS` di `src/main.cpp` (default total ±3 detik).
