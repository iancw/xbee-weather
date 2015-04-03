#ifndef ___XBEE__H__
#define ___XBEE__H__

#include <zneo.h>
#include "uart.h"

void init_xbee();
int xbee_keypressed();
void xbee_putchar(char c);
void xbee_print(char * text);
unsigned char xbee_getchar();

#endif