// TFT_eSPI custom setup for:
//   Board:   ESP32-WROOM-32 (esp32dev)
//   Display: 1.28" round GC9A01, 240x240 px
//   Wiring:  RST=GPIO22  CS=GPIO5  DC=GPIO21  SDA(MOSI)=GPIO23  SCL(SCK)=GPIO18
//
// This file is NOT picked up automatically by the Arduino IDE. It must be
// copied into the TFT_eSPI library folder and selected via
// User_Setup_Select.h. See ../README.md for the exact steps.

#define USER_SETUP_ID 201

#define GC9A01_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 240

#define TFT_MISO -1   // not used by GC9A01 (write-only display)
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   21
#define TFT_RST  22
#define TFT_BL   -1   // set to a GPIO number if you wire the backlight pin, -1 = always on

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
