#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Fonts/TomThumb.h>
#include <Wire.h>
#include <RTClib.h>

#define PANEL_RES_X 80
#define PANEL_RES_Y 40
#define PANEL_CHAIN 1

MatrixPanel_I2S_DMA *dma_display = nullptr;

RTC_DS3231 rtc;

bool rtcOK = false;

// ======================================================
// UART
// ======================================================

#define RXD2 4
#define TXD2 -1

#define BUZZER_PIN 15

// ======================================================
// MODE PANEL
// ======================================================

enum ModePanel {

  MODE_MAINTENANCE,
  MODE_SAFE,
  MODE_DANGER,
  MODE_ERROR

};

ModePanel modeAktif = MODE_SAFE;
ModePanel lastMode = MODE_SAFE;

// ======================================================
// SCROLL
// ======================================================

int scrollX;

unsigned long lastUART = 0;
unsigned long startupDelay = 0;

String teksScroll =
"  WAITING SYSTEM  ";

// ======================================================
// SETUP
// ======================================================

void setup() {

  Serial.begin(115200);

  Serial2.begin(
    2400,
    SERIAL_8N1,
    RXD2,
    -1
  );

  Serial2.setRxBufferSize(1024);

  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);

  // ======================================================
  // RTC
  // ======================================================

  Wire.begin(12, 13);

  if(rtc.begin(&Wire)){

    rtcOK = true;

    if(rtc.lostPower()){

      rtc.adjust(
        DateTime(
          F(__DATE__),
          F(__TIME__)
        )
      );
    }
  }

  // ======================================================
  // PANEL CONFIG
  // ======================================================

  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,
    PANEL_RES_Y,
    PANEL_CHAIN
  );

  mxconfig.driver = HUB75_I2S_CFG::SHIFTREG;

  mxconfig.mx_height = 40;

  // ======================================================
  // ADDRESS
  // ======================================================

  mxconfig.gpio.a = 21;
  mxconfig.gpio.b = 19;
  mxconfig.gpio.c = 18;
  mxconfig.gpio.d = 23;
  mxconfig.gpio.e = 5;

  // ======================================================
  // CONTROL
  // ======================================================

  mxconfig.gpio.lat = 17;
  mxconfig.gpio.oe  = 16;
  mxconfig.gpio.clk = 14;

  // ======================================================
  // RGB
  // ======================================================

  mxconfig.gpio.r1 = 25;
  mxconfig.gpio.g1 = 26;
  mxconfig.gpio.b1 = 27;

  mxconfig.gpio.r2 = 33;
  mxconfig.gpio.g2 = 32;
  mxconfig.gpio.b2 = 22;

  // ======================================================
  // START DISPLAY
  // ======================================================

  dma_display =
  new MatrixPanel_I2S_DMA(mxconfig);

  if(!dma_display->begin()){

    while(1);
  }

  dma_display->setBrightness8(80);

  dma_display->setTextWrap(false);

  dma_display->setFont(&TomThumb);

  dma_display->setTextSize(2);

  scrollX = PANEL_RES_X;

  Serial.println("PANEL START");

  startupDelay = millis();
}

// ======================================================
// BUZZER FUNCTION
// ======================================================

void buzzerBeep(
  int jumlah,
  int delayOn,
  int delayOff
){

  for(int i = 0; i < jumlah; i++){

    digitalWrite(BUZZER_PIN, HIGH);

    delay(delayOn);

    digitalWrite(BUZZER_PIN, LOW);

    delay(delayOff);
  }
}

// ======================================================
// LOOP
// ======================================================

void loop() {

  // ======================================================
  // UART RECEIVE
  // ======================================================

  while(Serial2.available() > 200){

    Serial2.read();
  }

  if(Serial2.available()){

    String data = "";

    while(Serial2.available()){

      char c = Serial2.read();

      if(c == '\n' || c == '\r'){

        continue;
      }

      data += c;
    }

    data.trim();

    if(data.indexOf("SAFE") >= 0){

      data = "SAFE";

    }else if(data.indexOf("DANGER") >= 0){

      data = "DANGER";

    }else if(data.indexOf("ERROR") >= 0){

      data = "ERROR";

    }else if(data.indexOf("MAINTENANCE") >= 0){

      data = "MAINTENANCE";

    }else{

      return;
    }

    lastUART = millis();

    Serial.print("UART RX : ");

    Serial.println(data);

    // ======================================================
    // SAFE
    // ======================================================

    if(data == "SAFE"){

      modeAktif = MODE_SAFE;

      if(lastMode != MODE_SAFE){

        buzzerBeep(1, 80, 80);

        lastMode = MODE_SAFE;
      }

      teksScroll =
      "  ZERO ENERGY SAFE  ";

      scrollX = PANEL_RES_X;
    }

    // ======================================================
    // DANGER
    // ======================================================

    else if(data == "DANGER"){

      modeAktif = MODE_DANGER;

      if(lastMode != MODE_DANGER){

        buzzerBeep(2, 120, 120);

        lastMode = MODE_DANGER;
      }

      teksScroll =
      "  DANGER ENERGY ON  ";

      scrollX = PANEL_RES_X;
    }

    // ======================================================
    // ERROR
    // ======================================================

    else if(data == "ERROR"){

      modeAktif = MODE_ERROR;

      if(lastMode != MODE_ERROR){

        buzzerBeep(1, 700, 200);

        lastMode = MODE_ERROR;
      }

      teksScroll =
      "  SYSTEM ERROR  ";

      scrollX = PANEL_RES_X;
    }

    // ======================================================
    // MAINTENANCE
    // ======================================================

    else if(data == "MAINTENANCE"){

      modeAktif = MODE_MAINTENANCE;

      if(lastMode != MODE_MAINTENANCE){

        buzzerBeep(3, 70, 70);

        lastMode = MODE_MAINTENANCE;
      }

      teksScroll =
      "  MAINTENANCE MODE  ";

      scrollX = PANEL_RES_X;
    }
  }

  // ======================================================
  // UART TIMEOUT
  // ======================================================

  if(millis() - startupDelay > 8000){

    if(millis() - lastUART > 15000){

      modeAktif = MODE_ERROR;

      teksScroll =
      "  UART DISCONNECTED  ";

      if(lastMode != MODE_ERROR){

        buzzerBeep(1, 700, 200);

        lastMode = MODE_ERROR;
      }
    }
  }

  // ======================================================
  // RTC / JAM
  // ======================================================

  char jam[9];

  if(rtcOK){

    DateTime now = rtc.now();

    sprintf(
      jam,
      "%02d:%02d:%02d",
      now.hour(),
      now.minute(),
      now.second()
    );

  }else{

    unsigned long detik =
    millis() / 1000;

    sprintf(
      jam,
      "%02d:%02d:%02d",
      (int)(detik / 3600) % 24,
      (int)(detik / 60) % 60,
      (int)(detik % 60)
    );
  }

  // ======================================================
  // RESET SCROLL
  // ======================================================

  int panjangScroll =
  teksScroll.length() * 12;

  if(scrollX < -panjangScroll){

    scrollX = PANEL_RES_X;
  }

  // ======================================================
  // CLEAR SCREEN
  // ======================================================

  dma_display->clearScreen();

  // ======================================================
  // JAM
  // ======================================================

  dma_display->setTextColor(
    dma_display->color565(255, 220, 0)
  );

  dma_display->setCursor(1, 11);

  dma_display->print(jam);

  // ======================================================
  // WARNA MODE
  // ======================================================

  switch(modeAktif){

    case MODE_MAINTENANCE:

      dma_display->setTextColor(
        dma_display->color565(0, 0, 255)
      );

    break;

    case MODE_SAFE:

      dma_display->setTextColor(
        dma_display->color565(0, 255, 0)
      );

    break;

    case MODE_DANGER:

      dma_display->setTextColor(
        dma_display->color565(255, 0, 0)
      );

    break;

    case MODE_ERROR:

      if((millis() / 300) % 2 == 0){

        dma_display->setTextColor(
          dma_display->color565(255, 0, 0)
        );

      }else{

        dma_display->setTextColor(
          dma_display->color565(255, 255, 255)
        );
      }

    break;
  }

  // ======================================================
  // TEXT SCROLL
  // ======================================================

  dma_display->setCursor(scrollX, 30);

  dma_display->print(teksScroll);

  scrollX -= 2;

  delay(30);

  yield();
}