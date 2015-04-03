#include "clock.h"
#include <zneo.h>

unsigned long clock = 5529600l;

void init_clock()
{
    int osc;
    osc = OSCCTL & 0x03;
    if(osc == 0)
        clock = 5529600l;//internal
    if(osc == 1)
        clock = 18432000l;//external
    if(osc == 3)
        clock = 10000l;//watchdog
}

void set_clock_18_432()
{
    OSCCTL = 0xE7;
    OSCCTL = 0x18;
    OSCCTL = 0x61;
    clock = 18432000l;
}

void set_clock_5_5()
{
    OSCCTL = 0xE7;
    OSCCTL = 0x18;
    OSCCTL = 0xA0;
    clock = 5529600l;
}

unsigned long get_clock()
{
	return clock;
}

void delay(int ms)
{
    unsigned long cycles;
    cycles = (clock * ms) / 1000;
    while(cycles > 0)
    {
        cycles--;
    }
}
