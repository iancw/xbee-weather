#include "25lc040a_eeprom.h"
#include <zneo.h>

#include <stdio.h>
#include "spi.h"
#include "clock.h"


/*
 Notes:
 
 I believe the device needs phase=0, clock=0, so that values
 are read on rising edges/written on falling edges and the clock
 idles low.
 
 Chip select is active low, so ESPIMODE's SSPO should be 0

 25CL040A
 ^                 _____        ^
 |  PH0-------/CS-|  U  |-VCC---|    ^
 |  PC5--------SO-|     |-/HOLD------|
 |------------/WP-|     |-SCK-------PC3
         -----VSS-|_____|-SI--------PC4
         |
         V


 */

/*
 *  Wait for the Transmit Data Register Empty bit to indicate 
 *  that its empty (1), meaning that the byte was transmitted
 */


/*
 * Write a byte to an SPI device. Send address then data
 */

void _drop_cs()
{
    PHOUT &= 0xFE;
}

void _raise_cs()
{
    PHOUT |= 0x01;
    delay(1);
}

unsigned char _cycle(unsigned char out) {
    unsigned char in;	

    ESPIDATA = out;              // Write address
	
    spi_wait_TDRE();              // Wait for transmission to complete
    spi_wait_RDRF();              // Wait for recived byte to complete
	
    in = ESPIDATA;             // Get received byte
    spi_wait_TFST();              // Wait for SPI transfer to complete
	            // Clear SS to stop communication
    return in;
}

void write_en_25LC040A()
{
    _drop_cs();
     _cycle(0x06); //Write enable
     _raise_cs();
}

void init_25LC040A()
{
	PCAF |= 0x3C; //enable pins 2-5 of portc for AF1
    PHDD &= 0xFE; //set PH0 Data direction to output for use as SS
    ESPICTL = 0x00; //disable before setting baud
        //baud rate = 1 khz
	ESPIBR = get_clock()/(2*1000);
	ESPICTL = 0x43;//IRQ off, TX enable=1, brgctl=0, phase=0, clkpol=0, open drain=0, mmen=1, rx=1
    ESPIMODE = 0x03; //SSIO=as an input, active high.  wire SS to ground to disable the DS1722.
    _raise_cs();
}

unsigned char read_status_25LC040A()
{
    unsigned char c;
    _drop_cs();
    _cycle(0x05);
    c = _cycle(0x00);
    _raise_cs();
    return c;
}

void read_25LC040A(unsigned char* data, int start, int len)
{
    int i=0;
    char cmd;
    
    for(i=start; i<(start+len) && i< MAX_ADDRESS; i++)
    {
        _drop_cs();
        cmd = 0x03 | ((i & 0x100) >> 5); //pick off the MSB and shift to position 0x08
        _cycle(cmd); // write read command
        _cycle(i); // read address zero
	    *data =  _cycle(0x00);
        _raise_cs();
        data++;
    }   
}

void fill_25LC040A(int start, int len, unsigned char* data)
{
    int i=0;
    int val=0;
    char cmd;

	if(start + len > MAX_ADDRESS)
	{
		printf(R"Error, writing data beyond 4 KB is not allowed.\n");
		return;
	}
    
    write_en_25LC040A();

    for(i=start; i<(start+len) && i < MAX_ADDRESS; i++)
    {
        write_en_25LC040A();
        _drop_cs();
        cmd = 0x02 | ((i & 0x100) >> 5); //pick off the MSB and shift to position 0x08
	    _cycle(cmd); // write read command
        _cycle(i); // write address i
	    _cycle(data[val++]);
        _raise_cs();
        delay(5);
    }
}
