/*
 *  I2C API
 *
 *  This is a very basic I2C library.
 *  Large portions were lifted from the Zilog application notes
 *  and examples. Its not very well thought out or debugged yet. 
 *  It works for some things I have tested it with, like a DS1307.
 *
 *  Its not nice and tidy the way I think it should be but it
 *  does work and it gets the job done.
 *
 *  So far this ONLY works with 7-bit addresses. I don't have any
 *  10-bit address I2C devices to play with. 
 *
 */


#include <zneo.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "i2c.h"
#include "clock.h"
#include "debug_flag.h"


void empty(char* empt){}

/*
 * Initialize I2C master
 */

void i2c_init(unsigned long baud)
{

    // Set baud rate
    // BRG = systemclock/(4 * baudrate)

    I2CBR = 4 * (get_clock()/baud);                     // about 48,000 @ 5.5296 MHz

    PAAF |= 0xC0;                    // Enable I2C alternate function

    I2CMODE = 0x00;                   // polling, 7bit, auto ack
                                      // no GCE, no slave addr
                                      // no diags
	
    I2CCTL = I2CCTL_ENABLE;           // Enable Rx/Tx; 
}


/*
 * Wait for transmission to complete
 */ 

int i2c_wait_TDRE() {
    int timeout = 0;
    while((I2CISTAT & TRANSMIT_DATA_REG_EMPTY) == 0x00) {
        if(++timeout == TIMEOUT)
			break;
        ;
    }
    if(timeout == TIMEOUT)
    {
		if(is_debug()){
        	printf(R"Timed out waiting for TDRE\n");
		}
        return 0;
        
    }
    return 1;
}


/* 
 * Wait for data to arrive.
 */

int i2c_wait_RDRF() {
    int timeout = 0;
    while((I2CISTAT & RECEIVE_DATA_REG_FULL) == 0x00) {
        if(++timeout == TIMEOUT)
			break;
        ;
    }
    if(timeout == TIMEOUT)
    {
		if(is_debug()){
        	printf(R"Timed out waiting for RDRF\n");			
		}
        return 0;
    }
    return 1;
}

/*
 * Wait for valid ACK bit.  return 1
 * for success, 0 for failure
 */
int i2c_wait_ACKV() {
    int timeout = 0;
    while(!(I2CSTATE & 0x80)) {
        if(++timeout == TIMEOUT)
			break;
    }
    if(timeout == TIMEOUT)
    {
		if(is_debug()){
        	printf(R"Timed out waiting for ACKV\n");
		}
        return 0;
    }

//	printf(R"Waited %d cycles for ACKV\n", timeout);

    return 1;
}



int i2c_wait_BUSY()
{
	int timeout = 0;
    while((I2CSTATE & 0x01)) {
        if(++timeout == TIMEOUT)
			break;
    }
    if(timeout == TIMEOUT)
    {
		if(is_debug())
		{
        	printf(R"Timed out waiting for BUSY to clear\n");
		}
        return 0;
    }
	
    return 1;
}

/*
 * Returns a 1 if the ACK bit is 1, or a 0 if not
*/
int i2c_get_ACK()
{
	return ((I2CSTATE & 0x40)>>6);
}


/*
 * Flush the buffer and toggle the I2C on and off.
 */

void i2c_clear() {
    I2CCTL |= I2CCTL_FLUSH;
    I2CCTL &= ~I2CCTL_ENABLE;
    I2CCTL |=I2CCTL_ENABLE;
}

int i2c_send_byte(unsigned char byte)
{
	I2CDATA = byte;
	if(!i2c_wait_TDRE())
	{
		if(is_debug())
		{
			printf(R"Timeout out sending byte 0x%02x\n", byte);
		}
		return 0;
	}
	return 1;
}

int i2c_send_byte_wait_ack(unsigned char byte)
{
	int state, valid;
	I2CDATA = byte;
    if(!i2c_wait_ACKV())
	{
		if(is_debug())
		{
			printf(R"Timed out waiting for ACKV from 0x%02x\n", byte);
		}
		return 0;
	}

    state = I2CSTATE; //check bit 6 (ACK value)
	valid = ((state & 0x40)>>6);
	if(!valid)
	{
		if(is_debug())
		{
			printf(R"Received NACK for byte 0x%02x {I2CSTATE=0x%02x}\n", byte, state);
		}
		return 0;
	}
	return 1;
}

int i2c_check_device(unsigned char address)
{
	int success=0;
	int timeout=0;
	unsigned char state, valid;
	//ping the eeprom just to see if its alive
	if(!i2c_wait_BUSY())
	{
		i2c_print_diag();
		return 0;
	}
	I2CCTL |= 0x40; //start
	I2CCTL |= 0x20; //stop
	I2CDATA = address; //send the eeprom address
	if(i2c_send_byte_wait_ack(address))
	{
		printf(R"Received an ACK from 0x%02x\n", address);
		success=1;		
	}else
	{
		success=0;
	}

	return success;
}

void i2c_print_diag()
{
	printf(R"I2CMODE: 0x%02x\n", I2CMODE);
	printf(R"I2CCTL: 0x%02x\n", I2CCTL);
	printf(R"I2CSTATE: 0x%02x\n", I2CSTATE);
	printf(R"I2CISTAT: 0x%02x\n", I2CISTAT);
	printf(R"I2CBR = %d (0x%04x)\n", I2CBR, I2CBR);
}
