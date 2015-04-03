#include <zneo.h>
#include <stdio.h>
#include <sio.h>

#include "MPL115A.h"
#include "clock.h"
#include "i2c.h"
#include "debug_flag.h"

#define I2C_ADDR 0xC0
#define BAUD 400000l  //100 KHz, 400 KHz is the part max

#define A0 0
#define B1 1
#define B2 2
#define C12 3
#define C11 4
#define C22 5

#define NUM_COEFF 6

float do_pressure_comp(unsigned short p_adc, unsigned short t_adc);
float coeff_to_float(unsigned short coeff,short bitCount,short iBits, short fracBits, short padBits);
int read_data(unsigned char command, unsigned char* buf, int len);
int send_command(unsigned char cmd);

float coefficients[NUM_COEFF];

short num_bits[NUM_COEFF] = {16, 16, 16, 14, 11, 11},
 int_bits[NUM_COEFF]={12, 2, 1, 0, 0, 0},
 frac_bits[NUM_COEFF]={3, 13, 14, 13, 10, 10},
 pad_bits[NUM_COEFF]={0, 0, 0, 9, 11, 15};

int send_command(unsigned char cmd)
{
	int valid;
//	DI();
	I2CCTL |= 0x40; //start
    if(!i2c_send_byte(I2C_ADDR))
	{
		if(is_debug())
		{
			printf(R"ACK failed after 0x%02x\n", I2C_ADDR);
		}
		return 0;
	}
	//send a command, followed by stop
	//valid commands are: 0x10, 0x11, and 0x12

	if(!i2c_send_byte(cmd))
	{
		if(is_debug()){
			printf(R"ACK failed after 0x%02x\n", cmd);
		}

		return 0;
	}

	if(!i2c_send_byte(0x01))
	{
		if(is_debug()){
			printf(R"ACK failed after 0x%02x\n", 0x01);
		}
		return 0;
	}
	I2CCTL |= 0x20; //stop
//	EI();
	return 1;
}

int read_data(unsigned char command, unsigned char* buf, int len)
{
	int valid, i;
	unsigned char data1, data2;
	//DI();
	I2CCTL |= 0x40; //start
	if(!i2c_send_byte_wait_ack(I2C_ADDR))
	{
		if(is_debug()){
			printf(R"ACK failed after 0x%02x\n", I2C_ADDR);
		}

		return 0;
	}
	if(!i2c_send_byte_wait_ack(command))
	{
		if(is_debug()){
			printf(R"ACK failed after 0x%02x\n", 0x00);
		}
		return 0;
	}
	I2CCTL |= 0x40; //restart
	if(!i2c_send_byte((I2C_ADDR | 0x01)))
	{
		if(is_debug())
		{
			printf(R"ACK failed after 0x%02x\n", (I2C_ADDR | 0x01));
		}
		return 0;
	}
	//Read len bytes
	for(i=0; i<len; i++)
	{
		if(!i2c_wait_RDRF())
		{
			if(is_debug()){
				printf(R"Failed waiting for RDRF on byte %d\n", i);
			}
			return 0;
		}
		buf[i] = I2CDATA;		
	}
	I2CCTL |= 0x20; //Send stop after reading the next byte
	//EI();
	return 1;
}
/*
 * Code is modified from a post by Freescale forum user
 * PJH, see: http://forums.freescale.com/t5/Other-Microcontrollers/MPL115A-fixed-point-question/td-p/47633
 */
float coeff_to_float(unsigned short coeff,short bitCount,short iBits, short fracBits, short padBits)
{
// Utility function to convert the coeff's into floating point parameters.
// See Freescale App note AN3785 for details of how the parameters are stored.

    float result=0, float_sum=0;
    int s=0;
    int bit, b;
    int fb=0;
	unsigned short mask;

//	printf(R"Converting 0x%04x (%hd bits, %hd int bits, %hd frac bits, %hd padding)\n", coeff, bitCount, iBits, fracBits, padBits);
// We start working our way through the bits from high to low.
// The incoming data is a 16bit integer which we have to shorten since the lower LSB's are empty if <16 bits. (see pg 15 of AN3785).
    if (bitCount<16)
    {// Make coeff the appropriate size
        coeff=coeff>>(16-bitCount);
    }

// Sanity check that the parameters passed into the function make sense. We can't have more or less bits than the amount of data.
// Add 1 to account for sign bit.
    if((iBits+fracBits+1)!=bitCount)
    {
        printf(R"Error in parameters, fractional bits and integer bits don't add up!!\n");
        return 0;
    }
//Sanity check that if we are padding the decimal point that we can't also have integer bits.
    if((padBits>0)&&(iBits>0))
    {
        printf(R"Error in parameters. iBits and padBits not consistent!!\n");
        return 0;
    }
	
    for(b=bitCount;b>0;b--)
    {
		mask = (0x0001 << (b-1));
        bit=((mask & coeff)>>(b-1)); // Get each bit in turn as a 1 or a 0.
        if (b==bitCount) // if this is the first bit (sign bit check if it is set)
        {
            s=bit;
            if(s)
            {
                coeff=~coeff+1; //Calculate 2's complement; //(See App note AN3785 middle of Pg15 for explanation.
            }
        }
        else // process the bits in turn.
        {
            if(iBits>0)
            { // we have an integer bit;
                if(bit)
                {
                    result+=(0x0001 << (iBits-1));
                }
                iBits--;
            }
            else // we have a fractional bit
            {
                fb++;
                if(bit)
                {
                    result+= (float)(1.0/(float)(0x01 << (fb+padBits)));
                }
                fracBits--;
            }
        }
    }
    if(s)
    {
        result*=-1;
    }
    return result;
}

void init_MPL115A()
{
    I2CBR = get_clock()/(4l * BAUD);
    PAAF |= 0xC0;            
    I2CMODE = 0x00;                   // polling, 7bit, auto ack
                                      // no GCE, no slave addr
                                      // no diags
    I2CCTL = 0x80; // I2C Enable bit  
	MPL115A_read_coefficients();
}

/*
 * Converts the adc pressure to compensated pressure per the spec
 * Pcomp = a0 + (b1 + c11*Padc + c12*Tadc) * Padc + (b2 + c22*Tadc) * Tadc
 */
float do_pressure_comp(unsigned short p_adc, unsigned short t_adc)
{
	float p_comp, c11x1, a11, c12x2, a1, c22x2, a2, a1x1, y1, a2x2;
	c11x1 = coefficients[C11] * (float)p_adc;
	a11 = coefficients[B1] + c11x1;
	c12x2 = coefficients[C12] * t_adc;
	a1 = a11 + c12x2;
	c22x2 = coefficients[C22] * t_adc;
	a2 = coefficients[B2] + c22x2;
	a1x1 = a1 * p_adc;
	y1 = coefficients[A0] + a1x1;
	a2x2 = a2 * t_adc;
	p_comp = y1 + a2x2;
	return p_comp;
}

float MPL115A_read_pressure()
{
	unsigned char buf[50];
	int len=4, j=0;
	unsigned short p_adc, t_adc;
	float pcomp;
	if(send_command(0x12))
	{      
		delay(3); //3 ms conversion time after a command
		if(!read_data(0x04, buf, len))
		{
			if(is_debug())
			{
				printf(R"Read failed\n");
			}
			return 0.0;
		}
		p_adc = (buf[j++] << 8);
		p_adc += (buf[j++] & 0x00FF);
		
		t_adc = (buf[j++] << 8);
		t_adc += (buf[j++] & 0x00FF);

        p_adc = (p_adc >> 6);
        t_adc = (t_adc >> 6);
		pcomp= do_pressure_comp(p_adc, t_adc);
		//kPa to inches hg conversion
		return 0.2953 * pcomp;
	}
	if(is_debug())
	{
		printf(R"Failed to send 0x12 command, clearing i2c...\n");
	}
	i2c_clear();
	return 0.0;
}

void MPL115A_read_coefficients()
{
	unsigned char buf[50];
	unsigned short coeff[NUM_COEFF];
	char * names[8] = {"A0", "B1", "B2", "C12", "C11", "C22"};
	
	int i,j=0;
	int len=2*NUM_COEFF;
	if(send_command(0x12))
	{      
		delay(3); //3 ms conversion time after a command
		if(!read_data(0x04, buf, len))
		{
			if(is_debug())
			{
				printf(R"Read failed\n");
			}
			return;
		}
		for(i=0; i<NUM_COEFF; i++)
		{
			coeff[i] = (buf[j++] << 8);
			coeff[i] += (buf[j++] & 0x00FF);
			coefficients[i] = coeff_to_float(coeff[i], num_bits[i], int_bits[i], frac_bits[i], pad_bits[i]);
		}
        return;
	}
	if(is_debug())
	{
    	printf(R"Error sending command 0x12\n");
	}
}
