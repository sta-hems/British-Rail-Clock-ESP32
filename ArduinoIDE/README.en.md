# British Rail Clock — Arduino IDE Version

*[Deutsche Version](README.md)*

Adapted for: **ESP32-WROOM-32** + **1.28" GC9A01 TFT, 240x240 px, round**

## Wiring

| Display pin | ESP32 GPIO |
|---|---|
| RST | 22 |
| CS  | 5  |
| DC  | 21 |
| SDA (MOSI) | 23 |
| SCL (SCK)  | 18 |
| VCC | 3.3V |
| GND | GND |
| BLK (backlight) | 3.3V or a free GPIO (see below) |

SDA/SCL match the ESP32's default VSPI pins (MOSI=23, SCK=18), CS=5 is also the VSPI default.

## One-time setup

1. **Install the ESP32 board package** in the Arduino IDE (File → Preferences → Additional Boards Manager URLs: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`, then Tools → Board → Boards Manager → install "esp32").
2. **Install the TFT_eSPI library**: Tools → Manage Libraries → search for `TFT_eSPI` (by Bodmer) and install it.
3. **Apply the pin configuration** — TFT_eSPI is configured through a setup file inside the library itself, not in the sketch:
   - Copy `Setup_GC9A01_ESP32.h` from this folder to
     `<Arduino sketchbook>/libraries/TFT_eSPI/User_Setups/Setup_GC9A01_ESP32.h`
     (find the sketchbook folder under Arduino IDE → Settings → "Sketchbook location", e.g. `~/Documents/Arduino`).
   - In that library folder, open `User_Setup_Select.h` and:
     - Comment out the line `#include <User_Setup.h>` (prefix it with `//`).
     - Add a new line: `#include <User_Setups/Setup_GC9A01_ESP32.h>`
4. **Set your WiFi credentials**: copy `BritishRailClock/secrets.h.example` to `BritishRailClock/secrets.h` and fill in your own `WIFI_SSID` / `WIFI_PASSWORD`. `secrets.h` is gitignored and never committed.
5. Open the sketch `BritishRailClock/BritishRailClock.ino` in the Arduino IDE.
6. Select the board: Tools → Board → ESP32 Arduino → **ESP32 Dev Module**.
7. Upload.

## Notes

- The clock shows the real time for Europe/Berlin (24h format, automatic CET/CEST daylight-saving switch), synced over WiFi/NTP at boot and resynced every minute so it doesn't drift. If WiFi or the time sync fails at boot, it falls back to a free-running fake clock (like the original project) so the display still animates.
- The sketch auto-detects whether the installed ESP32 core (2.x or 3.x) uses the old or new hardware-timer API, since that API changed between versions.
- If the display shows inverted colors or a mirror image, try adding `TFT_INVERSION_ON`/`TFT_INVERSION_OFF` in `Setup_GC9A01_ESP32.h`, or adjust `tft.setRotation()` in the sketch.

## Known issue: display stays black

On a classic ESP32 (not S2/S3), the free DRAM heap is split into two blocks
by statically reserved WiFi/BT buffers — even when WiFi/BT isn't used. The
largest contiguous free block is therefore often just over 100KB. The
`baseSprite` (238×238 pixels) needs 113KB in 16-bit color and no longer fits:
`createSprite()` silently returns a null pointer, nothing gets drawn anymore,
and the display stays completely black (no crash, no error message).

Fix: `baseSprite.setColorDepth(8);` right before the `createSprite()` call in
`setup()`. Halves the memory requirement to 57KB, which reliably fits the
available block. For the flat colors used here (black/red/white), the visual
difference from the 8-bit color depth isn't noticeable. Already included in
the shipped `BritishRailClock.ino`.
