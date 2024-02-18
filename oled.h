#ifndef OLED_H
#define OLED_H


#include "SSD1306Wire.h"
#include "qrcode.h"

#define OLED_WIDTH 128 // OLED display width, in pixels
#define OLED_HEIGHT 64 // OLED display height, in pixels

SSD1306Wire display(0x3c, SDA, SCL);   // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h e.g. https://github.com/esp8266/Arduino/blob/master/variants/nodemcu/pins_arduino.h
#define CENTER  display.getWidth()/2

class OLED {
  private:
  public:

    void Header(const char * title) {
      display.setColor(WHITE);
      display.fillRect(0, 0, display.getWidth(), 15);
      display.setColor(BLACK);
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_10);
      display.drawString(CENTER, 1, title);
      display.setColor(WHITE);
      display.display();
    }
#define ALL 0,display.getWidth()
    void Clear(int y=14, int h=display.getHeight()-26) {
      display.setColor(BLACK);
      display.fillRect(0, y, display.getWidth(), h);
      display.setColor(WHITE);
      display.display();
    }

    void ClearLine(int y) {
      display.setColor(BLACK);
      display.fillRect(0, y, display.getWidth(), y + 10);
      display.setColor(WHITE);
      display.display();
    }

    static void Center(int y, const String &text) {
      display.setColor(BLACK);
      display.fillRect(0, y, display.getWidth(), y + 10);
      display.setColor(WHITE);
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(CENTER, y, text);
      display.display();
    }
/*
    static void Wait() {
      display.setColor(BLACK);
      display.fillRect(0, 18, display.getWidth(), 30);
      display.setColor(WHITE);
      Center(27, "Waiting!");
      display.display();
    }
*/
    static void Data(uint8_t command, uint32_t Addr) {
      String tab[] = {"", "My", "Up", "My+Up", "Down", "My+Down", "Up+Down", "My+Up+Down", "Prog"};
      char str[30] = "Unknown";
      if (command < 9) strcpy(&str[0], &tab[command][0]);
      Data(str, Addr);

    }
    static void Data(char*cmd, uint32_t Addr) {
      char buf[8];
      display.setColor(BLACK);
      display.fillRect(0, 18, display.getWidth(), 30);
      display.setColor(WHITE);
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(20, 18, "Command:");
      display.drawString(80, 18, cmd);
      display.drawString(20, 30, "Remote:");
      sprintf(buf, "%06X", Addr);
      display.drawString(80, 30, buf);
      display.display();
    }

    int min(int a, int b) {
      return (a<b?a:b);
    }
    
    void QRcode(const char* text, int shift = -1, char * code1 = "", char * code2 = "", char * code3 = "") {
    //void QRcode(const char* text) {
      QRCode qrcode;
      uint8_t qrcodeData[qrcode_getBufferSize(3)];
      qrcode_initText(&qrcode, qrcodeData, 3, 0, text);
      int scale = min(OLED_WIDTH / qrcode.size, OLED_HEIGHT / qrcode.size);
      int shiftX = shift<0?(OLED_WIDTH - qrcode.size * scale) / 2:shift;
      //int shiftX = (OLED_WIDTH - qrcode.size * scale) / 2;
      int shiftY = (OLED_HEIGHT - qrcode.size * scale) / 2;
      display.setColor(WHITE);
      for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
          if (qrcode_getModule(&qrcode, x, y)) {
            display.fillRect(shiftX + x * scale, shiftY + y * scale, scale, scale);
          }
        }
      }
      int x = OLED_WIDTH - shiftX- qrcode.size;
      Serial.printf("x: %d, y: %d, %s, %s, %s\n",x, (OLED_HEIGHT/2), code1, code2, code3); 
      display.setColor(WHITE);
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(x, OLED_HEIGHT/2-20, code1);
      display.drawString(x, OLED_HEIGHT/2-4, code2);
      display.drawString(x, OLED_HEIGHT/2+12, code3);
      display.display();
    }

    void begin() {
    }

    void loop() {
    }
};

class FIELD{
  private:
    OLED * oled;
    int x;
    int y;
    int h;
    int w;
    OLEDDISPLAY_TEXT_ALIGNMENT a;
    OLEDDISPLAY_COLOR bg;
    OLEDDISPLAY_COLOR fg;
  public:
  FIELD(OLED * _oled, int xpos, int ypos, int height, int width, OLEDDISPLAY_TEXT_ALIGNMENT align, bool inverse = false) {
    oled = _oled; x = xpos; y = ypos; w = width; h = height; a = align; fg = inverse?BLACK:WHITE, bg = inverse?WHITE:BLACK;
  }
  void write(const char * text) {
      display.setColor(bg);
      int pos = 0;
      switch(a) {
        case TEXT_ALIGN_LEFT:
        pos = x;
        break;
        case TEXT_ALIGN_RIGHT:
        pos = x+w;
        break;
        case TEXT_ALIGN_CENTER:
        pos = x+w/2;
        break;
      }
      display.fillRect(x, y, w, h);
      display.setColor(fg);
      display.setTextAlignment(a);
      display.drawString(pos, y, text);
      display.display();
  }
};

#endif
