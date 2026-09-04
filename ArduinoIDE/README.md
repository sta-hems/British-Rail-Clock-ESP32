# British Rail Clock — Arduino IDE Version

*[English version](README.en.md)*

Angepasst für: **ESP32-WROOM-32** + **1,28" GC9A01 TFT, 240x240 px, rund**

## Verkabelung

| Display-Pin | ESP32 GPIO |
|---|---|
| RST | 22 |
| CS  | 5  |
| DC  | 21 |
| SDA (MOSI) | 23 |
| SCL (SCK)  | 18 |
| VCC | 3.3V |
| GND | GND |
| BLK (Backlight) | 3.3V oder frei wählbarer GPIO (siehe unten) |

SDA/SCL entsprechen den Standard-VSPI-Pins des ESP32 (MOSI=23, SCK=18), CS=5 ist ebenfalls der VSPI-Standard.

## Einmalige Einrichtung

1. **ESP32-Boardpaket** in der Arduino IDE installieren (Datei → Voreinstellungen → zusätzliche Boardverwalter-URL `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`, dann Werkzeuge → Board → Boardverwalter → "esp32" installieren).
2. **Bibliothek TFT_eSPI** installieren: Werkzeuge → Bibliotheken verwalten → nach `TFT_eSPI` (von Bodmer) suchen und installieren.
3. **Pin-Konfiguration einspielen** — TFT_eSPI wird über eine Setup-Datei in der Bibliothek selbst konfiguriert, nicht im Sketch:
   - Kopiere `Setup_GC9A01_ESP32.h` aus diesem Ordner nach
     `<Arduino-Skizzenbuch>/libraries/TFT_eSPI/User_Setups/Setup_GC9A01_ESP32.h`
     (Skizzenbuch-Ordner siehe Arduino IDE → Einstellungen → "Sketchbook location", z. B. `~/Documents/Arduino`).
   - Öffne in diesem Bibliotheksordner die Datei `User_Setup_Select.h` und:
     - Kommentiere die Zeile `#include <User_Setup.h>` aus (ein `//` davor).
     - Füge eine neue Zeile hinzu: `#include <User_Setups/Setup_GC9A01_ESP32.h>`
4. **WLAN-Zugangsdaten eintragen**: `BritishRailClock/secrets.h.example` nach `BritishRailClock/secrets.h` kopieren und dort `WIFI_SSID`/`WIFI_PASSWORD` eintragen. `secrets.h` ist per `.gitignore` ausgeschlossen und wird nie committet.
5. Sketch `BritishRailClock/BritishRailClock.ino` in der Arduino IDE öffnen.
6. Board wählen: Werkzeuge → Board → ESP32 Arduino → **ESP32 Dev Module**.
7. Hochladen.

## Hinweise

- Die Uhr zeigt die echte Zeit für Europe/Berlin (24-Stunden-Format, automatische Sommerzeit-Umschaltung), per WiFi/NTP beim Start synchronisiert und danach jede Minute erneut abgeglichen, damit sie nicht driftet. Schlägt WLAN oder die Zeitsynchronisation beim Start fehl, läuft die Uhr als Fallback mit einem simulierten Zähler weiter (wie im ursprünglichen Projekt), damit sie trotzdem animiert.
- Der Sketch erkennt automatisch, ob das installierte ESP32-Core (2.x oder 3.x) die alte oder neue Timer-API benutzt, da sich diese zwischen den Versionen geändert hat.
- Falls das Display invertierte Farben oder ein Spiegelbild zeigt, in `Setup_GC9A01_ESP32.h` ggf. `TFT_INVERSION_ON`/`TFT_INVERSION_OFF` ergänzen bzw. `tft.setRotation()` im Sketch anpassen.

## Bekanntes Problem: Display bleibt schwarz

Auf einem klassischen ESP32 (nicht S2/S3) ist der freie DRAM-Heap durch fest
reservierte WiFi/BT-Puffer in zwei Blöcke gesplittet — auch wenn WiFi/BT gar
nicht benutzt wird. Der größte zusammenhängende freie Block liegt dadurch oft
nur knapp über 100 KB. Das `baseSprite` (238×238 Pixel) benötigt in 16-Bit-Farbe
113 KB und passt dann nicht mehr rein: `createSprite()` gibt einen Nullpointer
zurück, es wird nichts mehr gezeichnet und das Display bleibt komplett schwarz
(ohne Absturz oder Fehlermeldung).

Fix: `baseSprite.setColorDepth(8);` direkt vor dem `createSprite()`-Aufruf in
`setup()`. Halbiert den Speicherbedarf auf 57 KB, passt damit sicher in den
verfügbaren Block. Für die hier verwendeten reinen Farben (Schwarz/Rot/Weiß)
ist der sichtbare Unterschied durch die 8-Bit-Farbtiefe nicht wahrnehmbar.
Bereits im mitgelieferten `BritishRailClock.ino` enthalten.
