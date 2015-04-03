#ifndef __PCD8544_SPI_H__
#define __PCD8544_SPI_H__

#include "spi.h"
#include "clock.h"
#include <zneo.h>

void _drop_dc()
{
	PCOUT &= 0xFD;
}

void _raise_dc()
{
	PCOUT |= 0x02;
}

void _drop_cs()
{
    PCOUT &= 0xFE;
}

void _raise_cs()
{
    PCOUT |= 0x01;
    delay(1);
}

void spiwrite(unsigned char c)
{
	spi_cycle(c);
}

void init_pcd8544_spi()
{
	PCAF |= 0x18; //enable pins 3&4 (CS and MOSI aren't necessary) of portc for AF1
	PCDD &= 0xFC; //set PC0 Data direction to output for use as SS and PC1 for D/C
	ESPICTL = 0x00; //disable before setting baud
	    //baud rate = 1 khz
	ESPIBR = get_clock()/(2*1000);
	ESPICTL = 0x43;//IRQ off, TX enable=1, brgctl=0, phase=0, clkpol=0, open drain=0, mmen=1, rx=1
	ESPIMODE = 0x03; //SSIO=as an input, active high.  wire SS to ground to disable the DS1722.
	_raise_cs();
}


#endif