/* 
 * DS1722 SPI sensor specific conditions
 * February 26, 2009
 *
 */

#include "spi.h"

#define ADDR_DS1722_CONFIG_READ   0x00
#define ADDR_DS1722_CONFIG_WRITE  0x80
#define ADDR_DS1722_TEMP_LSB      0x01
#define ADDR_DS1722_TEMP_MSB      0x02

#define DS1722_REQUIRED           0xE0
#define DS1722_ONESHOT            0x10
#define DS1722_CONTINUOUS         0x00
#define DS1722_SHUTDOWN           0x01
#define DS1722_8BIT               0x00
#define DS1722_9BIT               0x02
#define DS1722_10BIT              0x04
#define DS1722_11BIT              0x06
#define DS1722_12BIT              0x08


/* 
 * Read a byte from the DS1722
 */

unsigned char ds1722_read_byte(unsigned char addr) { 
    unsigned char data;
    data = spi_read(addr);
    return data;
}


/*
 * Initialize the DS1722
 */

void ds1722_init(void) { 
    // write E0 (1110_0000) to sensor addr 80h
    // to set it up for 10 bit, cotinuous conversion, 

    spi_write( DS1722_REQUIRED | DS1722_10BIT, ADDR_DS1722_CONFIG_WRITE );
}


/*
 * Get the DS1722 configuration byte
 */

unsigned char ds1722_get_config(void) { 
    unsigned char settings;
    settings = ds1722_read_byte(ADDR_DS1722_CONFIG_READ);
    return settings;
}


/*
 * Get the DS1722 temperature (as an int)
 */

int ds1722_get_temp(void) { 
    int temp;
    unsigned char low,high;

    // FIXED from the previous version
    high=spi_read(ADDR_DS1722_TEMP_MSB);
     // we don't get the MSB for an int

    temp = high;
    return temp;
}


/*
 * Get the DS1722 temperature (as afloat)
 */

float ds1722_get_ftemp(void) { 
    float temp;
    unsigned char low,high;

    low=spi_read(ADDR_DS1722_TEMP_LSB);
    high=spi_read(ADDR_DS1722_TEMP_MSB);

    temp = high + (low/256.0);  // yup, thats what you do with the LSB
    return temp;
}

