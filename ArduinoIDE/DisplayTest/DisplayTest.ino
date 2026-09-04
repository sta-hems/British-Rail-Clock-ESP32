// Minimal-Diagnose fuer GC9A01 240x240 an ESP32-WROOM-32
// RST=22 CS=5 DC=21 SDA(MOSI)=23 SCL(SCK)=18
// Zeigt nacheinander Vollfarben + Text, mit Serial-Heartbeat zur Fehlersuche.

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("BOOT: vor tft.init()");

  tft.init();
  Serial.println("BOOT: nach tft.init()");

  tft.setRotation(0);
  Serial.printf("BOOT: width=%d height=%d\n", tft.width(), tft.height());
}

void loop() {
  Serial.println("LOOP: RED");
  tft.fillScreen(TFT_RED);
  delay(1500);

  Serial.println("LOOP: GREEN");
  tft.fillScreen(TFT_GREEN);
  delay(1500);

  Serial.println("LOOP: BLUE");
  tft.fillScreen(TFT_BLUE);
  delay(1500);

  Serial.println("LOOP: WHITE + Text");
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawString("TEST 1234", 20, 100, 4);
  delay(2500);
}
