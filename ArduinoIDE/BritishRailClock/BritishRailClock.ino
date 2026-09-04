/*
   British Railways "new style" clock — Arduino IDE version
   Original PlatformIO project: https://github.com/mgaman/new-british-rail-clock-arduino-tft
   https://www.youtube.com/watch?v=k9GNMepc9xs
   https://youtube.com/shorts/Q88IBHg2Aqc

   Target hardware: ESP32-WROOM-32 + 1.28" round GC9A01 TFT, 240x240 px
   Wiring: RST=GPIO22  CS=GPIO5  DC=GPIO21  SDA(MOSI)=GPIO23  SCL(SCK)=GPIO18

   IMPORTANT: the actual pin/driver configuration lives in the TFT_eSPI
   library's own setup file, not in this sketch. See Setup_GC9A01_ESP32.h
   in this folder and the README next to it for the one-time setup.

   Shows the real time (24h format) for Europe/Berlin, synced over WiFi/NTP.
   If WiFi or the time sync fails at boot, falls back to a free-running
   fake clock (like the original project) so the display still animates.
*/

#include <Arduino.h>
#include <math.h>
#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <esp_arduino_version.h>
#include <WiFi.h>
#include <time.h>
#include "secrets.h" // defines WIFI_SSID / WIFI_PASSWORD - copy secrets.h.example, not committed to git

// NTP / Zeitzone Europe/Berlin (CET/CEST mit automatischer Sommerzeit-Umschaltung)
const char* NTP_SERVER1 = "pool.ntp.org";
const char* NTP_SERVER2 = "de.pool.ntp.org";
const char* TZ_BERLIN = "CET-1CEST,M3.5.0,M10.5.0/3";

#define BASE_COLOR TFT_BLACK

#define CIRCLE_COLOR TFT_RED
#define CIRCLE_THICKNESS 4

#define ARROW_WIDTH 10
#define BASE_BORDER 1   // gap around base sprite and screen so base is 239x239
#define DOT_STEPS 480  // steps per 360 degrees i.e. up/down 240 pixels

TFT_eSPI tft = TFT_eSPI()  ;    // Invoke custom library
TFT_eSprite baseSprite = TFT_eSprite(&tft);  // contains circles
TFT_eSprite GtArrowSprite = TFT_eSprite(&tft);  // > arrow
TFT_eSprite LtArrowSprite = TFT_eSprite(&tft);  // < arrow
TFT_eSprite timeSprite = TFT_eSprite(&tft);  // time as text
hw_timer_t *Timer0_Cfg = NULL;

unsigned int sWidth,sHeight,currentMinute;
volatile bool timerTicked = false;
unsigned int OUTER_CIRCLE_INNER_RADIUS,OUTER_CIRCLE_OUTER_RADIUS,CURRENT_STEP;
unsigned int INNER_CIRCLE_INNER_RADIUS,INNER_CIRCLE_OUTER_RADIUS;
float hypotenuse;
bool rtcSynced = false;   // true once we have real time from NTP at least once

/**
 * @brief Connect to WiFi (bounded wait) and start the SNTP sync for the
 * Berlin timezone. Non-fatal on failure - the clock keeps running with
 * fake time in that case.
 */
void connectWiFiAndSyncTime() {
  Serial.printf("WiFi: connecting to '%s' ...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi: connection failed, falling back to fake time.");
    return;
  }
  Serial.printf("WiFi: connected, IP=%s\n", WiFi.localIP().toString().c_str());

  configTzTime(TZ_BERLIN, NTP_SERVER1, NTP_SERVER2);

  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 10000)) {
    rtcSynced = true;
    Serial.printf("Time synced: %02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  } else {
    Serial.println("NTP: time sync failed, falling back to fake time.");
  }
}

/**
 * @brief Read the current local time (Berlin) and return minutes since
 * midnight, matching the format displayTime()/currentMinute already use.
 *
 * @param outMinutes receives hours*60+minutes on success
 * @return true if a valid time could be read
 */
bool getMinutesSinceMidnight(unsigned int &outMinutes) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 100)) return false;
  outMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
  return true;
}

void IRAM_ATTR Timer0_ISR()
{
    timerTicked = true;
}

/**
 * @brief Draw both arrows in the same color as the rings. The containing sprite is twice as high as
 * it is wide. For the > shaped arrow, 2 sloped lines are drawn, from (0,0) to (width,width) and (width,width)
 * to (0,width*2). The < shaped arrow is drawn as a mirror of <. The for loop is to make the line thicker.
 *
 */
void drawArrowSprite() {
  GtArrowSprite.fillSprite(TFT_BLACK);
  LtArrowSprite.fillSprite(TFT_BLACK);
  for (int l=0;l<5;l++) {
    GtArrowSprite.drawLine(0,l,ARROW_WIDTH-l,ARROW_WIDTH-1,CIRCLE_COLOR); // fixed x1 and y2
    GtArrowSprite.drawLine(ARROW_WIDTH-l,ARROW_WIDTH,0,(2*ARROW_WIDTH)-l,CIRCLE_COLOR); // fixed y1 and x2

    LtArrowSprite.drawLine(l,ARROW_WIDTH,ARROW_WIDTH-1,l,CIRCLE_COLOR); // fixed y1 and x2
    LtArrowSprite.drawLine(l,ARROW_WIDTH,ARROW_WIDTH-1,(2*ARROW_WIDTH)-l,CIRCLE_COLOR); // fixed y1 and x2
  }
}

/**
 * @brief Format the elasped time as the string HH:MM and draw it into timeSprite
 *
 * @param time Current time in seconds
 */
void displayTime(unsigned int time) {
  unsigned minutes = time % 60;
  unsigned hours = time / 60;
  hours %= 24;   // limit to 23 hours
  char timeString[10];
  sprintf(timeString,"%02d:%02d",hours,minutes);
  timeSprite.fillSprite(TFT_BLACK);
  timeSprite.drawString(timeString,0,0,6);
}

/**
 * @brief Draw the base sprite which was overwritten by other sprites every timer tick.
 * Paint the sprite in its background color then draw 2 concentric circles
 *
 */
void drawBase() {
  baseSprite.fillSprite(BASE_COLOR);
  baseSprite.drawArc(baseSprite.width()/2,baseSprite.height()/2,OUTER_CIRCLE_OUTER_RADIUS,OUTER_CIRCLE_INNER_RADIUS,0,360,CIRCLE_COLOR,BASE_COLOR,true);
  baseSprite.drawArc(baseSprite.width()/2,baseSprite.height()/2,INNER_CIRCLE_OUTER_RADIUS,INNER_CIRCLE_INNER_RADIUS,0,360,CIRCLE_COLOR,BASE_COLOR,true);
}

/**
 * @brief Calculate where to place the 2 arrow sprites on the base sprite. This is calculated relative to
 * the center of the base sprite. The first half calculates the position of the > arrow on the outer circle,
 * the second half the < arrow on the inner circle.
 * Note how I make > go clockwize and < anti-clockwize
 *
 * @param angle The angle between 12 and current second, expressed in radians. e.g 15 seconds is at 90 degrees
 * or PI/2 radians.
 */
void locateArrows(float angle) {
  // calculate pivot points for locating arrows
  int16_t xOuter = hypotenuse*sin(angle);
  int16_t yOuter = hypotenuse*cos(angle);
  // xOuter,yOuter calculated from center of baseSprite, correct for origin in top left
  baseSprite.setPivot((baseSprite.width()/2)+xOuter,(baseSprite.width()/2)-yOuter);
  GtArrowSprite.pushRotated(&baseSprite,(int16_t)(angle*180/PI),TFT_BLACK);  // rotate clockwize
  // repeat for 2nd arrow going anticlockwize
  int16_t xInner = (hypotenuse-ARROW_WIDTH-(CIRCLE_THICKNESS/2)-1)*sin(angle);
  int16_t yInner = (hypotenuse-ARROW_WIDTH-(CIRCLE_THICKNESS/2)-1)*cos(angle);
  baseSprite.setPivot((baseSprite.width()/2)-xInner,(baseSprite.width()/2)-yInner);
  LtArrowSprite.pushRotated(&baseSprite,360-(int16_t)(angle*180/PI),TFT_BLACK);
}

/**
 * @brief Arduino initialization code.
 * currentMinute is seeded from NTP (connectWiFiAndSyncTime(), called above)
 * when available. If WiFi/NTP failed, falls back to a fake starting time
 * so the clock still animates.
 *
 */
void setup(void) {
  Serial.begin(115200);
  delay(1000);

  connectWiFiAndSyncTime();

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(BASE_COLOR);
  // calculate some values
  sWidth = tft.width();
  sHeight = tft.height();
  // create base sprite & fill it
  // If I make a sprite the same size as the tft screen my code freezes.
  // 8-bit color depth: on classic ESP32 the DRAM heap is split by the
  // reserved WiFi/BT buffers, so the largest free block is often just
  // over 100KB - a 238x238 16-bit sprite (113KB) fails to allocate there,
  // while the same sprite at 8-bit (57KB) fits comfortably.
  baseSprite.setColorDepth(8);
  baseSprite.createSprite(sWidth-(BASE_BORDER*2),sHeight-(BASE_BORDER*2));
  // calculate inner,outer radii of the 2 circles
  OUTER_CIRCLE_OUTER_RADIUS = (baseSprite.width()/2) - ARROW_WIDTH;
  OUTER_CIRCLE_INNER_RADIUS = OUTER_CIRCLE_OUTER_RADIUS - CIRCLE_THICKNESS;
  // place inner circle ARROW_WIDTH inside outer circle
  INNER_CIRCLE_OUTER_RADIUS = OUTER_CIRCLE_INNER_RADIUS - ARROW_WIDTH;
  INNER_CIRCLE_INNER_RADIUS = INNER_CIRCLE_OUTER_RADIUS - CIRCLE_THICKNESS;
  // calculate distance from center to outer circle
  hypotenuse = (OUTER_CIRCLE_INNER_RADIUS+OUTER_CIRCLE_OUTER_RADIUS)/2.0f;
  drawBase();
  // create  arrow sprites and draw on the circle
  GtArrowSprite.createSprite(ARROW_WIDTH+1,ARROW_WIDTH*2+1);
  LtArrowSprite.createSprite(ARROW_WIDTH+1,ARROW_WIDTH*2+1);
  drawArrowSprite();  // only need to do this once
  // push base to screen
  baseSprite.pushSprite(BASE_BORDER,BASE_BORDER);  // do not ignore background color
  // create sprite for time display, just guessed the size
  timeSprite.createSprite(125,40);
  timeSprite.setTextColor(TFT_WHITE);

  CURRENT_STEP = 0;
  // set timer to interrupt DOT_STEPS times every 60 secs
  // The ESP32 Arduino timer API changed between core 2.x and core 3.x,
  // so both variants are supported here depending on the installed core.
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  Timer0_Cfg = timerBegin(1000000);  // 1 MHz timer clock
  timerAttachInterrupt(Timer0_Cfg, &Timer0_ISR);
  timerAlarm(Timer0_Cfg, 60000000L/DOT_STEPS, true, 0);
#else
  Timer0_Cfg = timerBegin(0, 80, true);
  timerAttachInterrupt(Timer0_Cfg, &Timer0_ISR, true);
  timerAlarmWrite(Timer0_Cfg, 60000000L/DOT_STEPS, true);
  timerAlarmEnable(Timer0_Cfg);
#endif

  struct tm timeinfo;
  if (rtcSynced && getLocalTime(&timeinfo, 100)) {
    currentMinute = timeinfo.tm_hour * 60 + timeinfo.tm_min;
    // start the sweeping arrows at the right position within the current minute
    CURRENT_STEP = (timeinfo.tm_sec * DOT_STEPS) / 60;
  } else {
    currentMinute = 59+(23*60);  // no real time available - fake starting time so it still animates
  }
  displayTime(currentMinute);
}

/**
 * @brief Called every clock tick to redraw the screen
 *
 */
void drawScreen() {
  // place at correct angle
  float currentAngle = (PI*CURRENT_STEP*360.0f/DOT_STEPS)/180.0f;  // angle in radians
  drawBase();
  locateArrows(currentAngle);
  CURRENT_STEP++;
  if (CURRENT_STEP == DOT_STEPS) {
    CURRENT_STEP = 0;
    unsigned int mins;
    if (getMinutesSinceMidnight(mins)) {
      currentMinute = mins;   // resync from NTP-disciplined RTC every minute
    } else {
      currentMinute++;        // no time source available - keep counting
    }
    displayTime(currentMinute);
  }
  timeSprite.pushToSprite(&baseSprite,55,100);
  // push base to screen
  baseSprite.pushSprite(BASE_BORDER,BASE_BORDER);  // do not ignore background color
}

/**
 * @brief Infinite loop driven by the timer tick. Every time tick redraw the screen
 *
 */
void loop() {
    if (timerTicked) {
      drawScreen();
      timerTicked = false;
    }
}
