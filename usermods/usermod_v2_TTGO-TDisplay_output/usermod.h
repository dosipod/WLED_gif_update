/*
  -----------------------------------------------------------------------------------------------
  Lilygo/TTGO T-Display screen output
  -----------------------------------------------------------------------------------------------

  Ideas/todos:
    - add rotation support
    - choose between square and nonsquare pixels (nonsquare fills the screen, square goes to center)
    - add reserved pins (tft) to xml.cpp
    - usermod configuration instead of led and 2D settings ?
    - add "display" as led type 

  Measurement:
    Time to draw = ~19200 microsec / fullscreen + ~15 microsec / "led"
    60x34 = 2400 led = 52160 microsec
    48x27 = 1296 led = 39050 microsec
    40x22 =  880 led = 32580 microsec
    16x9  =  144 led = 19430 microsec
    (all with margins)
*/

#pragma once

#include "wled.h"
#include <TFT_eSPI.h>

#define ADC_EN 14  // Used for enabling battery voltage measurements - not used in this program

class TTGOTDisplayOutputUsermod : public Usermod {
  private:
    TFT_eSPI tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT); // Setup display

    uint16_t blocksW = 0;
    uint16_t blocksH = 0;
    uint16_t top = 0;
    uint16_t left = 0;
    uint16_t oneWidth = 0;
    uint16_t oneHeight = 0;
    uint16_t margin = 1;

    // uint32_t milliSum    = 0;
    // uint32_t sampleCount = 0;

    bool checkSettings(){
      if(!strip.isMatrix) return false;
      if(strip.panels!=1) return false;
      bool marginSame = (strip.panel[0].serpentine?0:1) == margin;
      if(blocksW==strip.panel[0].width && blocksH==strip.panel[0].height && marginSame) return true;

      tft.fillScreen(TFT_BLACK); // clear screen on change

      blocksW = strip.panel[0].width;
      blocksH = strip.panel[0].height;
      margin  = strip.panel[0].serpentine?0:1;

      oneWidth  = floor((tft.width()  - (blocksW-1)*margin) / blocksW);
      oneHeight = floor((tft.height() - (blocksH-1)*margin) / blocksH);
      top  = (tft.height() - (blocksH-1)*margin - oneHeight * blocksH) / 2;
      left = (tft.width()  - (blocksW-1)*margin - oneWidth  * blocksW) / 2;

      // Serial.printf("blocksW: %d, blocksH: %d, oneWidth: %d, oneHeight: %d, top: %d, left: %d\n",blocksW,blocksH,oneWidth,oneHeight,top,left);

      return true;
    }

  public:

    void setup() {
      //Serial.println("TTGO usermod - Setup");

      pinMode(TFT_BL, OUTPUT); // Set backlight pin to output mode
      digitalWrite(TFT_BL, HIGH); // Turn backlight on. 

      tft.init();
      tft.setRotation(3);  //Rotation here is set up for the text to be readable with the port on the left.
      tft.fillScreen(TFT_BLACK);
      tft.setTextSize(2);
      tft.setTextColor(TFT_WHITE);
      tft.setCursor(1, 10);
      tft.setTextDatum(MC_DATUM);
      tft.println("WLED - TTGO");
    }

    void onStateChange(uint8_t mode) {
      digitalWrite(TFT_BL, bri==0?LOW:HIGH );
    }

    void loop() {
      // unsigned long msStart = micros();
      if(bri==0) return;
      if(!checkSettings()) return;

      for(uint16_t h = 0; h<blocksH; h++){
        for(uint16_t w = 0; w<blocksW; w++){
          tft.fillRect(left + w * (oneWidth + margin), top + h * (oneHeight + margin), oneWidth, oneHeight, tft.color24to16(strip.getPixelColorXY(w,h)));
        }
      }

      // if(micros() > msStart){
      //   milliSum += micros() - msStart;
      //   sampleCount++;
      //   if(sampleCount==100){
      //     Serial.printf("Display time: %d microsecond => %d fps\n", milliSum / sampleCount, sampleCount * 1000000 / milliSum);
      //     milliSum = 0;
      //     sampleCount = 0;
      //   }
      // }
    }

};