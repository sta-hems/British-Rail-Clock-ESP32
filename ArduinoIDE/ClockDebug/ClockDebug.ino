/*
   Debug-Variante von BritishRailClock.ino mit Zwischenausgaben,
   um zu sehen, wo genau die Uhr haengen bleibt / schwarz bleibt.
*/

#include <Arduino.h>
#include <math.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <esp_arduino_version.h>

#define BASE_COLOR TFT_BLACK
#define CIRCLE_COLOR TFT_RED
#define CIRCLE_THICKNESS 4
#define ARROW_WIDTH 10
#define BASE_BORDER 1
#define DOT_STEPS 480

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite baseSprite = TFT_eSprite(&tft);
TFT_eSprite GtArrowSprite = TFT_eSprite(&tft);
TFT_eSprite LtArrowSprite = TFT_eSprite(&tft);
TFT_eSprite timeSprite = TFT_eSprite(&tft);
hw_timer_t *Timer0_Cfg = NULL;

unsigned int sWidth,sHeight,currentMinute;
volatile bool timerTicked = false;
volatile unsigned long isrCount = 0;
unsigned int OUTER_CIRCLE_INNER_RADIUS,OUTER_CIRCLE_OUTER_RADIUS,CURRENT_STEP;
unsigned int INNER_CIRCLE_INNER_RADIUS,INNER_CIRCLE_OUTER_RADIUS;
float hypotenuse;
unsigned long frameCount = 0;

void IRAM_ATTR Timer0_ISR() {
    timerTicked = true;
    isrCount++;
}

void drawArrowSprite() {
  GtArrowSprite.fillSprite(TFT_BLACK);
  LtArrowSprite.fillSprite(TFT_BLACK);
  for (int l=0;l<5;l++) {
    GtArrowSprite.drawLine(0,l,ARROW_WIDTH-l,ARROW_WIDTH-1,CIRCLE_COLOR);
    GtArrowSprite.drawLine(ARROW_WIDTH-l,ARROW_WIDTH,0,(2*ARROW_WIDTH)-l,CIRCLE_COLOR);
    LtArrowSprite.drawLine(l,ARROW_WIDTH,ARROW_WIDTH-1,l,CIRCLE_COLOR);
    LtArrowSprite.drawLine(l,ARROW_WIDTH,ARROW_WIDTH-1,(2*ARROW_WIDTH)-l,CIRCLE_COLOR);
  }
}

void displayTime(unsigned int time) {
  unsigned minutes = time % 60;
  unsigned hours = time / 60;
  hours %= 24;
  char timeString[10];
  sprintf(timeString,"%02d:%02d",hours,minutes);
  timeSprite.fillSprite(TFT_BLACK);
  timeSprite.drawString(timeString,0,0,6);
}

void drawBase() {
  baseSprite.fillSprite(BASE_COLOR);
  baseSprite.drawArc(baseSprite.width()/2,baseSprite.height()/2,OUTER_CIRCLE_OUTER_RADIUS,OUTER_CIRCLE_INNER_RADIUS,0,360,CIRCLE_COLOR,BASE_COLOR,true);
  baseSprite.drawArc(baseSprite.width()/2,baseSprite.height()/2,INNER_CIRCLE_OUTER_RADIUS,INNER_CIRCLE_INNER_RADIUS,0,360,CIRCLE_COLOR,BASE_COLOR,true);
}

void locateArrows(float angle) {
  int16_t xOuter = hypotenuse*sin(angle);
  int16_t yOuter = hypotenuse*cos(angle);
  baseSprite.setPivot((baseSprite.width()/2)+xOuter,(baseSprite.width()/2)-yOuter);
  GtArrowSprite.pushRotated(&baseSprite,(int16_t)(angle*180/PI),TFT_BLACK);
  int16_t xInner = (hypotenuse-ARROW_WIDTH-(CIRCLE_THICKNESS/2)-1)*sin(angle);
  int16_t yInner = (hypotenuse-ARROW_WIDTH-(CIRCLE_THICKNESS/2)-1)*cos(angle);
  baseSprite.setPivot((baseSprite.width()/2)-xInner,(baseSprite.width()/2)-yInner);
  LtArrowSprite.pushRotated(&baseSprite,360-(int16_t)(angle*180/PI),TFT_BLACK);
}

void setup(void) {
  Serial.begin(115200);
  delay(1000);
  Serial.println("D1: start");

  tft.init();
  Serial.println("D2: tft.init done");
  tft.setRotation(0);
  tft.fillScreen(BASE_COLOR);
  Serial.println("D3: fillScreen done");

  sWidth = tft.width();
  sHeight = tft.height();
  Serial.printf("D4: sWidth=%u sHeight=%u\n", sWidth, sHeight);

  Serial.printf("D4b: freeHeap=%u maxAlloc=%u\n", ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  baseSprite.setColorDepth(8);
  void* p1 = baseSprite.createSprite(sWidth-(BASE_BORDER*2),sHeight-(BASE_BORDER*2));
  Serial.printf("D5: baseSprite.createSprite ptr=%p w=%d h=%d freeHeapAfter=%u\n", p1, baseSprite.width(), baseSprite.height(), ESP.getFreeHeap());

  OUTER_CIRCLE_OUTER_RADIUS = (baseSprite.width()/2) - ARROW_WIDTH;
  OUTER_CIRCLE_INNER_RADIUS = OUTER_CIRCLE_OUTER_RADIUS - CIRCLE_THICKNESS;
  INNER_CIRCLE_OUTER_RADIUS = OUTER_CIRCLE_INNER_RADIUS - ARROW_WIDTH;
  INNER_CIRCLE_INNER_RADIUS = INNER_CIRCLE_OUTER_RADIUS - CIRCLE_THICKNESS;
  hypotenuse = (OUTER_CIRCLE_INNER_RADIUS+OUTER_CIRCLE_OUTER_RADIUS)/2.0f;
  Serial.printf("D6: radii OOR=%u OIR=%u IOR=%u IIR=%u hyp=%.2f\n",
                OUTER_CIRCLE_OUTER_RADIUS, OUTER_CIRCLE_INNER_RADIUS,
                INNER_CIRCLE_OUTER_RADIUS, INNER_CIRCLE_INNER_RADIUS, hypotenuse);

  drawBase();
  Serial.println("D7: drawBase (into sprite) done");

  void* p2 = GtArrowSprite.createSprite(ARROW_WIDTH+1,ARROW_WIDTH*2+1);
  void* p3 = LtArrowSprite.createSprite(ARROW_WIDTH+1,ARROW_WIDTH*2+1);
  Serial.printf("D8: arrow sprites ptr Gt=%p Lt=%p\n", p2, p3);
  drawArrowSprite();
  Serial.println("D9: drawArrowSprite done");

  baseSprite.pushSprite(BASE_BORDER,BASE_BORDER);
  Serial.println("D10: FIRST pushSprite to physical screen done - screen should show red rings now");

  void* p4 = timeSprite.createSprite(125,40);
  Serial.printf("D11: timeSprite ptr=%p\n", p4);
  timeSprite.setTextColor(TFT_WHITE);

  CURRENT_STEP = 0;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  Serial.println("D12: using core>=3 timer API");
  Timer0_Cfg = timerBegin(1000000);
  timerAttachInterrupt(Timer0_Cfg, &Timer0_ISR);
  timerAlarm(Timer0_Cfg, 60000000L/DOT_STEPS, true, 0);
#else
  Serial.println("D12: using core<3 timer API");
  Timer0_Cfg = timerBegin(0, 80, true);
  timerAttachInterrupt(Timer0_Cfg, &Timer0_ISR, true);
  timerAlarmWrite(Timer0_Cfg, 60000000L/DOT_STEPS, true);
  timerAlarmEnable(Timer0_Cfg);
#endif
  Serial.println("D13: timer configured");

  currentMinute = 59+(23*60);
  displayTime(currentMinute++);
  Serial.println("D14: setup() complete, entering loop()");
}

void drawScreen() {
  float currentAngle = (PI*CURRENT_STEP*360.0f/DOT_STEPS)/180.0f;
  drawBase();
  locateArrows(currentAngle);
  CURRENT_STEP++;
  if (CURRENT_STEP == DOT_STEPS) {
    CURRENT_STEP = 0;
    displayTime(currentMinute++);
  }
  timeSprite.pushToSprite(&baseSprite,55,100);
  baseSprite.pushSprite(BASE_BORDER,BASE_BORDER);
  frameCount++;
  if (frameCount <= 5 || frameCount % 100 == 0) {
    Serial.printf("D_LOOP: frame=%lu isrCount=%lu step=%u\n", frameCount, isrCount, CURRENT_STEP);
  }
}

unsigned long lastHeartbeat = 0;
void loop() {
    if (timerTicked) {
      drawScreen();
      timerTicked = false;
    }
    if (millis() - lastHeartbeat > 2000) {
      lastHeartbeat = millis();
      Serial.printf("D_HB: alive, isrCount=%lu frameCount=%lu\n", isrCount, frameCount);
    }
}
