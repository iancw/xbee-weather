#include "24lc16b_eeprom.h"
#include <zneo.h>

#include "clock.h"
#include "i2c.h"
#include <stdio.h>

#define MAX_CAP_24LC16B 0x800 //maximum meomory address
#define I2C_ADDR_24LC16B 0xa0

/*
 * Wired it thusly to my ZNEO board
 * 24LC16B
 *       ____                  ^
 * A0---|    |---VCC           \
 * A1---|    |---WP/VSS        /  10 k Ohms
 * A2---|    |---SCL/PA6       \
 *VSS---|____|---SDA-----------+------PA7
 *
 */
 void init_24LC16B()
 {

 }

 /*
 * Does a random read on the 24LC16B, this involves
 * 
 * START Control byte (ACK) word address (ACK)
 * START contrl byte (ACK) data (NACK) STOP
 */
unsigned char _random_read(int addr, int word)
{
    unsigned char data=0;
//	printf(R"Reading 24LC16B from address 0x%02x\n", addr);
	if(!i2c_wait_BUSY())
	{
		printf(R"I2C bus is busy, failed to read\n");
		return 0;
	}
//	DI();
    I2CCTL |= I2CCTL_START; //send start bit
	i2c_wait_TDRE();
    if(!i2c_send_byte_wait_ack((addr & 0xFE))) //wait for Tx ready
	{
		return 0;
	}
	i2c_wait_TDRE();
    if(!i2c_send_byte_wait_ack(word)) //wait for Tx ready
	{
		return 0;
	}
	i2c_wait_TDRE();
    I2CCTL |= I2CCTL_START;

    if(!i2c_send_byte((addr | 0x01))) //wait for Tx ready
	{
		return 0;
	}
    I2CCTL |= I2CCTL_STOP;
    i2c_wait_RDRF();

    data = I2CDATA;

//	EI();
    return data;
}

/*
 * Writes a byte to teh 24LC16B I2C EEPROM chip
 * Start | control byte | ACK | word address | ACK | data | ACK | STOP
 */
int _write_byte(unsigned char addr, unsigned char word, unsigned char data)
{
//	printf(R"Writing 0x%02x to 24LC16B address 0x%02x\n", data, word);
	DI();
    I2CCTL |= I2CCTL_START;
    I2CDATA = (addr & 0xFE);//clear the LSB for a write
    if(!i2c_wait_TDRE())
	{
		printf(R"Timed out writing device address 0x%02x\n", (addr & 0xFE));
		return 0;
	}
    I2CDATA = word;
    if(!i2c_wait_TDRE())
	{
		printf(R"Timed out writing word address 0x%02x\n", word);
		return 0;
	}
    I2CDATA = data;
    if(!i2c_wait_TDRE())
	{
		printf(R"Timed out writing data 0x%02x\n", data);
		return 0;
	}

    I2CCTL |= I2CCTL_STOP;
	EI();
	return 1;
}

void read_24LC16B(unsigned char* data, int start, int len)
{
	int i=0, address=0;
	
    for(i=start; i<(start+len) && i < MAX_CAP_24LC16B; i++)
    {
        address = I2C_ADDR_24LC16B;
        address |= ((0x0700 & i) >> 7); // pull off the 3 MSBs as block address
        *data = _random_read(address, i);
		printf(R"Read word 0x%02x\n", *data);
        data++;
    }
}

void write_24LC16B(int start, int len, unsigned char *data)
{
	int i=0, address;
    int val=0;
	
	for(i=start; i<(start+len) && i < MAX_CAP_24LC16B; i++)
    {
        address = I2C_ADDR_24LC16B;
        address |= ((0x0700 & i) >> 7); // pull off the 3 MSBs as block address
		_write_byte(address, i, data[val++]);

        delay(6);
    }
}
