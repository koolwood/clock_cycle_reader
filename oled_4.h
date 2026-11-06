#include <U8g2lib.h>
#include <Wire.h>
// #include <temp.h>
// #include <light_sensor.h>
//#include "esp32_bluetooth.h"


// select board LOLIN C3 Mini

// // there is no 72x40 constructor in u8g2 hence the 72x40 screen is mapped in the middle of the 1128x64 pixel buffer of the SSD1306 controller

//This setups up the i2c display to use the i2c made by the wire.begin() function
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
int width = 72 - 1;
int height = 40 - 1;
int xOffset = 28; // = (128-w)/2
int yOffset = 24; // = (64-h)

void oled_setup(void)
{
  // delay(3000);
  // Serial.begin(115200);
  // delay(1000);
  // Serial.println("setup  Started! ");
  //      btsetup();
  Wire.begin(5,6);
  // lsetup();
  u8g2.begin();
  u8g2.setContrast(255); // set contrast to maximum 
  u8g2.setBusClock(400000); //400kHz I2C 
//  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.setFont(u8g2_font_ncenB10_tr); // font
  // temp_setup();
  // pinMode(8, OUTPUT);
}

void oled_loop(int data,String mess)
{
  int bresult = 0;
  int counter = data;
  // static bool led = false;
  // led = !led;
  // digitalWrite(8, led);
  
  
  u8g2.clearBuffer(); // clear the internal memory
  u8g2.drawFrame(xOffset+0, yOffset+0, width, height); //draw a frame around the border
  u8g2.setCursor(xOffset+3, yOffset+12);
  //    u8g2.printf("%dx%d", width, height);
  u8g2.printf("%d", data);
  u8g2.setCursor(xOffset+3, yOffset+24);
  // float t = get_temp();
  // u8g2.printf("%0.1f F", t);
  u8g2.setCursor(xOffset+3, yOffset+36);
  // float t2 = lloop();
  // u8g2.printf("%0.0f lux", t2);
  u8g2.print(mess);
//  Serial.printf("counter = %d,temp = %0.1f F,lux = %0.f\n",counter - 1,t,t2);

  //  if(bresult = bt_get()){
  //   u8g2.printf("%d", bresult);
  //  }
  u8g2.sendBuffer(); // transfer internal memory to the display
  //delay(1000);
}