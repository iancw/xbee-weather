#ifndef __PCD_8544_H__
#define __PCD_8544_H__

/*
$Id:$

PCD8544 LCD library!

Copyright (C) 2010 Limor Fried, Adafruit Industries

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#define BLACK 1
#define WHITE 0

#define LCDWIDTH 84
#define LCDHEIGHT 48

#define PCD8544_POWERDOWN 0x04
#define PCD8544_ENTRYMODE 0x02
#define PCD8544_EXTENDEDINSTRUCTION 0x01

#define PCD8544_DISPLAYBLANK 0x0
#define PCD8544_DISPLAYNORMAL 0x4
#define PCD8544_DISPLAYALLON 0x1
#define PCD8544_DISPLAYINVERTED 0x5

// H = 0
#define PCD8544_FUNCTIONSET 0x20
#define PCD8544_DISPLAYCONTROL 0x08
#define PCD8544_SETYADDR 0x40
#define PCD8544_SETXADDR 0x80

// H = 1
#define PCD8544_SETTEMP 0x04
#define PCD8544_SETBIAS 0x10
#define PCD8544_SETVOP 0x80

  void init(void);
  void begin(unsigned char contrast);
  
  void command(unsigned char c);
  void data(unsigned char c);
  
void setContrast(unsigned char val);
  void clearDisplay(void);
  void clear();
  void display();
  
  void setPixel(unsigned char x, unsigned char y, unsigned char color);
  unsigned char getPixel(unsigned char x, unsigned char y);
  void fillcircle(unsigned char x0, unsigned char y0, unsigned char r, 
		  unsigned char color);
  void drawcircle(unsigned char x0, unsigned char y0, unsigned char r, 
		  unsigned char color);
  void drawrect(unsigned char x, unsigned char y, unsigned char w, unsigned char h, 
		unsigned char color);
  void fillrect(unsigned char x, unsigned char y, unsigned char w, unsigned char h, 
		unsigned char color);
  void drawline(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, 
		unsigned char color);
  
  void drawchar(unsigned char x, unsigned char line, char c);
  void drawstring(unsigned char x, unsigned char line, char *c);
  void drawstring_P(unsigned char x, unsigned char line, const char *c);
  void drawbitmap(unsigned char x, unsigned char y, 
		  const unsigned char *bitmap, unsigned char w, unsigned char h,
		  unsigned char color);
#endif
