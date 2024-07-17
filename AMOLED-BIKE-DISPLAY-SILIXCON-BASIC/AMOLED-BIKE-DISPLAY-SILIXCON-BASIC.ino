//#include <lvgl.h>

#include "rm67162.h"
#include <TFT_eSPI.h>

#include "OrbitronBlack130.h"
#include "OrbitronBlack35.h"
#include "OrbitronBlack30.h"
#include "OrbitronBlack15.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);


#define lineColor 0x0B91 // Color of the grid lines
#define lineAccColor 0x7E7C // Color of the grid line corner accents
#define speedTextColor 0xFFFF // Color of the grid line corner accents
#define boxBGColor 0xFFFF // Color of the grid line corner accents

void setup()
{
  pinMode(38, OUTPUT); // enables the AMOLED screen
  digitalWrite(38, HIGH);
  

  sprite.createSprite(240,536);
  sprite.setSwapBytes(1);

  /** Setup UART port  */
  Serial.begin(115200);
  
  rm67162_init();  // amoled lcd initialization
  lcd_brightness(200);
  
}

void draw()
 {

// Text datum examples
// #define TL_DATUM 0  // Top left (default)
// #define TC_DATUM 1  // Top centre
// #define TR_DATUM 2  // Top right
// #define ML_DATUM 3  // Middle left
// #define CL_DATUM 3  // Centre left, same as above
// #define MC_DATUM 4  // Middle centre
// #define CC_DATUM 4  // Centre centre, same as above
// #define MR_DATUM 5  // Middle right
// #define CR_DATUM 5  // Centre right, same as above
// #define BL_DATUM 6  // Bottom left
// #define BC_DATUM 7  // Bottom centre
// #define BR_DATUM 8  // Bottom right
  
  sprite.fillSprite(TFT_BLACK);

  sprite.setTextDatum(4);
  sprite.setTextColor(speedTextColor);
  sprite.loadFont(OrbitronBlack130);
  sprite.drawString(String("12"),120,95);

  sprite.setTextDatum(2);
  sprite.setTextColor(speedTextColor);
  sprite.loadFont(OrbitronBlack15);
  sprite.drawString(String("KPH"),229,10);

  sprite.setTextDatum(3);
  sprite.drawString(String("SOC"),20,230);
  sprite.drawString(String("SOC"),20,250);


  // draw the grid
  // outline
  sprite.drawLine(4, 4, 236, 4, lineColor);
  sprite.drawLine(4, 4, 4, 532, lineColor);
  sprite.drawLine(4, 532, 236, 532, lineColor);
  sprite.drawLine(236, 532, 236, 4, lineColor);

  // horizontal lines
  sprite.drawLine(4, 143, 236, 143, lineColor);
  sprite.drawLine(4, 198, 236, 198, lineColor);
  //sprite.drawLine(4, 314, 236, 314, lineColor);
  //sprite.drawLine(4, 430, 236, 430, lineColor);

  // vertical lines
  //sprite.drawLine(120, 198, 120, 430, lineColor);

  // draw the accent corners
  sprite.fillRect(2, 2, 4, 8, lineAccColor);
  sprite.fillRect(2, 2, 8, 4, lineAccColor);

  sprite.fillRect(230, 2, 8, 4, lineAccColor);
  sprite.fillRect(234, 2, 4, 8, lineAccColor);

  sprite.fillRect(2, 526, 4, 8, lineAccColor);
  sprite.fillRect(2, 530, 8, 4, lineAccColor);  

  sprite.fillRect(234, 526, 4, 8, lineAccColor);
  sprite.fillRect(230, 530, 8, 4, lineAccColor);

  //LHS
  sprite.fillRect(2, 137, 4, 12, lineAccColor);
  sprite.fillRect(2, 141, 8, 4, lineAccColor);

  sprite.fillRect(2, 192, 4, 12, lineAccColor);
  sprite.fillRect(2, 196, 8, 4, lineAccColor);

  sprite.fillRect(2, 308, 4, 12, lineAccColor);
  sprite.fillRect(2, 312, 8, 4, lineAccColor);

  sprite.fillRect(2, 424, 4, 12, lineAccColor);
  sprite.fillRect(2, 428, 8, 4, lineAccColor);

  //MID
  // sprite.fillRect(114, 196, 12, 4, lineAccColor);
  // sprite.fillRect(118, 196, 4, 8, lineAccColor);

  // sprite.fillRect(114, 312, 12, 4, lineAccColor);
  // sprite.fillRect(118, 308, 4, 12, lineAccColor);

  // sprite.fillRect(114, 428, 12, 4, lineAccColor);
  // sprite.fillRect(118, 424, 4, 8, lineAccColor);

  //RHS
  sprite.fillRect(234, 137, 4, 12, lineAccColor);
  sprite.fillRect(230, 141, 8, 4, lineAccColor);

  sprite.fillRect(234, 192, 4, 12, lineAccColor);
  sprite.fillRect(230, 196, 8, 4, lineAccColor);

  sprite.fillRect(234, 308, 4, 12, lineAccColor);
  sprite.fillRect(230, 312, 8, 4, lineAccColor);

  sprite.fillRect(234, 424, 4, 12, lineAccColor);
  sprite.fillRect(230, 428, 8, 4, lineAccColor);
  // end grid

  //sprite.loadFont(middleFont);
  //sprite.setTextColor(0x0B91);

  lcd_PushColors(0, 0, 240,536, (uint16_t*)sprite.getPointer());

 }






void loop() {

    draw();
}



