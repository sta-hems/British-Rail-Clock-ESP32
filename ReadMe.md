<p align="center"><img src="images/IMG_0668.png"></p>  

# British Railways new style clock
In Autumn 2025 the management of British Rail proudly announced the introduction of a [new style clock](https://www.networkrail.co.uk/stories/a-new-timepiece-for-the-railway/).<p>
Reportedly this was achieved at a bargain price of £120,000.<br>
## My Version
Feeling the need for a similar clock, but lacking the necessary funds, I made my own for an estimated cost of less then $5. Instead of the BR 1.8m diameter display I used a 1.28" TFT display and an ESP32 development board for the code.<br>
You can see it [here](https://youtube.com/shorts/Q88IBHg2Aqc). While my displays 240x240 pixels is considerably less than the BR screen it lacks the crisp resolution of the big clock but retains the overall effect.<br>
## Methodology
I wrote the code as an Arduino project in the **VSCode** and **PlatformIO** framework, using the [TFT_eSPi](https://github.com/Bodmer/TFT_eSPI/) library to control the screen. It proved to be a valuable excersize in learning TFT_eSPI and how to use sprites. I hope my code is documented well enough for
others to learn from it.<br>
My project was setup for a 240x240 circular TFT screen using a GC9A01 chip. The choice of TFT is irrelevant to the code of the clock.
**Caveat** The time displayed is entirely fake. It is simple a counter initialized to an arbitary value and incremented by the ESP32 internal crystal.
## Sprites
The heart of the code is the management of 4 sprites. The two arrow sprites only get drawn once as they never get over-drawn. The time sprite needs to be redrawn every time the time changes (every minute). The base sprite needs to be redrawn every clock tick as it has been over-drawn by the other sprites.
## Timer
The basic cycle of the clock is 1 minute. The two arrows complete a circuit of the clock which is a distance of 240 pixels up & down i.e 480 pixels in 60 seconds. The clock is therefore programmed to tick every 60M/480 microsecs.<br>

## This fork: Arduino IDE version (ESP32-WROOM-32)

This fork adds an [Arduino IDE](https://www.arduino.cc/en/software) build of the clock (the original project uses PlatformIO), plus real time via WiFi/NTP instead of the fake counter. See [ArduinoIDE/README.md](ArduinoIDE/README.md) for full setup instructions (in German).

**New in this fork:**
- **Arduino IDE compatibility** - ready-to-open sketch in [ArduinoIDE/BritishRailClock/](ArduinoIDE/BritishRailClock/) (no PlatformIO needed), with an ESP32 core 2.x/3.x compatible timer setup and a TFT_eSPI setup file matching the wiring below
- **WiFi connectivity** - connects to your network at boot, with a bounded timeout and a fallback to the original fake clock if the connection fails
- **NTP time sync** - real time for Europe/Berlin (24h format, automatic CET/CEST daylight-saving switch), resynced every minute so it doesn't drift; no more fake counter
- **Fix for a classic-ESP32 heap issue** - the base sprite uses 8-bit color instead of 16-bit so it reliably fits in the largest free heap block (see [ArduinoIDE/README.md](ArduinoIDE/README.md#bekanntes-problem-display-bleibt-schwarz) for details), otherwise the display can stay black
- WiFi credentials are kept in a local, gitignored `secrets.h` (see `secrets.h.example`), not committed to the repo

**Hardware:** ESP32-WROOM-32 + 1.28" round GC9A01 TFT, 240x240 px

**Wiring:**

| Display pin | ESP32 GPIO |
|---|---|
| RST | 22 |
| CS  | 5  |
| DC  | 21 |
| SDA (MOSI) | 23 |
| SCL (SCK)  | 18 |
| VCC | 3.3V |
| GND | GND |

SDA/SCL match the ESP32's default VSPI pins (MOSI=23, SCK=18), CS=5 is also the VSPI default.

The sketch lives in [ArduinoIDE/BritishRailClock/](ArduinoIDE/BritishRailClock/). It shows the real time for Europe/Berlin (24h, automatic DST) synced over WiFi/NTP, falling back to the original free-running fake clock if WiFi/NTP is unavailable at boot.

