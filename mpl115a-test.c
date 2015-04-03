#include <zneo.h>
#include <stdio.h>
#include <sio.h>

#define I2C_ADDR 0xC0
#define BAUD 200000  //100 KHz, 400 KHz is the part max            
#define TIMEOUT 8000


long clock;

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

void delay(int ms)
{
    long cycles;
    cycles = (clock * ms) / 1000;
    while(cycles > 0)
    {
        cycles--;
    }
}

int wait_ackv()
{
	int timeout=0;
	while(!(I2CSTATE & 0x80)) {
        if(++timeout == TIMEOUT)
			break;
    }
	if(timeout == TIMEOUT)
	{
		return 0;
	}
	return 1;
}


int wait_tdre() {
    int timeout = 0;
    while((I2CISTAT & 0x80) == 0x00) {
        if(++timeout == TIMEOUT)
			break;
        ;
    }
    if(timeout == TIMEOUT)
    {
        return 0;
    }
    return 1;
}

int send_byte_wait_ack(unsigned char byte)
{
	int state, valid;
	I2CDATA = byte;
    if(!wait_ackv())
	{
		printf(R"Timed out waiting for TDRE from 0x%02x\n", byte);
		return 0;
	}

    state = I2CSTATE; //check bit 6 (ACK value)
	valid = ((state & 0x40)>>6);
	if(!valid)
	{
		printf(R"Invalid ACK from MPL115A for byte 0x%02x (0x%02x) {0x%02x}\n", byte, valid, state);
		return 0;
	}
	return 1;
}

int send_byte(unsigned char byte)
{
	I2CDATA = byte;
	if(!wait_tdre())
	{
		printf(R"Timeout out sending byte 0x%02x\n", byte);
		return 0;
	}
	return 1;
}

int send_command(unsigned char cmd)
{
	int valid;
	I2CCTL |= 0x40; //start
    if(!send_byte(I2C_ADDR))
	{
		printf(R"ACK failed after 0x%02x\n", I2C_ADDR);
		return 0;
	}
	//send a command, followed by stop
	//valid commands are: 0x10, 0x11, and 0x12

	if(!send_byte(cmd))
	{
		printf(R"ACK failed after 0x%02x\n", cmd);
		return 0;
	}

	if(!send_byte(0x01))
	{
		printf(R"ACK failed after 0x%02x\n", 0x01);
		return 0;
	}
	I2CCTL |= 0x20; //stop
	return 1;
}

int wait_rdrf()
{
	int timeout = 0;
    while((I2CISTAT & 0x40) == 0x00) {
        if(++timeout == TIMEOUT)
			break;
    }
    if(timeout == TIMEOUT)
    {
        printf(R"Timed out waiting for RDRF\n");
        return 0;
    }
    return 1;
}

int read_data(unsigned char* buf, int len)
{
	int valid, i;
	unsigned char data1, data2;
	I2CCTL |= 0x40; //start
	if(!send_byte_wait_ack(I2C_ADDR))
	{
		printf(R"ACK failed after 0x%02x\n", I2C_ADDR);
		return 0;
	}
	if(!send_byte_wait_ack(0x00))
	{
		printf(R"ACK failed after 0x%02x\n", 0x00);
		return 0;
	}
	I2CCTL |= 0x40;
	if(!send_byte_wait_ack((I2C_ADDR | 0x01)))
	{
		printf(R"ACK failed after 0x%02x\n", (I2C_ADDR | 0x01));
		return 0;
	}
	for(i=0; i<len; i++)
	{
		if(!wait_rdrf())
		{
			printf(R"Failed waiting for RDRF on byte %d\n", i);
			return 0;
		}
		buf[i] = I2CDATA;		
	}
	I2CCTL |= 0x20; //Send stop after reading the next byte

	return 1;
}
#define PADC 0
#define TADC 1
#define A0 2
#define B1 3
#define B2 4
#define C12 5
#define C11 6
#define C22 7
void process_data(unsigned char *buf, int len)
{
	int i, j=0;
	unsigned int coeff[8], p_adc, t_adc, pres_comp;
	long tmp1, tmp2, tmp3, c11x1, a11, c12x2, a1;
	long c22x2, a2, a1x1, y1, a2x2;
	float pres_scale; //final output
	
	//Coeff order is:
	//a0, b1, b2, c12, c11, c22

	printf(R"Coefficients are: \n");
	for(i=0; i<8; i++)
	{
		coeff[i] = (buf[j++] << 8);		
		coeff[i] += (buf[j++] & 0x00FF);
		printf(R"%u (0x%04x)\n", coeff[i], coeff[i]);
	}
	//Now we have the coefficients combined...
	//This code mimics the example code in the MPL115A spec
	//I'm a little confused by the floating point components,
	//so I'm just following along for now
	p_adc = (coeff[PADC] >> 6);
	t_adc = (coeff[TADC] >> 6);
	printf(R"P_adc = %u, T_adc = %u\n", p_adc, t_adc);
	
	tmp1 = coeff[C11];
	c11x1 = tmp1 * (long)p_adc;
	printf(R"c11x1 = %ld\n", c11x1);
	
	tmp1 = (long)(coeff[B1] << 14);
	tmp2 = c11x1 + tmp1;
	a11 = (long)(tmp2 >> 14);
	printf(R"a11 = %ld\n", a11);
	
	c12x2 = coeff[C12] * (long)t_adc;
	printf(R"c12x2 = %ld\n", c12x2);
	
	tmp1 = (a11 << 11);
	tmp3 = tmp1 + c12x2;
	a1 = (tmp3 >> 11);
	printf(R"a1 = %ld\n", a1);
	
	c22x2 = coeff[C22] * (long)t_adc;
	printf(R"c22x2 = %ld\n", c22x2);
	
	//step 6
	tmp1 = (coeff[B2] << 15);
	tmp2 = (c22x2 >> 1);
	tmp3 = tmp1 + tmp2;
	a2 = (tmp3 >> 16);
	printf(R"a2 = %ld\n", a2);
	
	//step 7 (a1x1  = a1 * p_adc)
	a1x1 = a1 * (long)p_adc;
	printf(R"a1x1 = %ld\n", a1x1);
	
	//step 8 y1 = a0 +a1x1
	tmp1 = (coeff[A0] << 10);
	tmp2 = a1x1 + tmp1;
	y1 = (tmp2 >> 10);
	printf(R"y1 = %ld\n", y1);
	
	//step 9 a2x2 = a2 * t_adc
	a2x2 = a2 * (long)t_adc;
	printf(R"a2x2 = %ld\n", a2x2);
	
	//step 10 pres_comp = y1 + a2x3
	tmp1 = (y1 << 10);
	tmp2 = tmp1 + a2x2;
	pres_comp = (tmp2 >> 13);
	printf(R"pres_comp [0,1023] = %d\n", pres_comp);
	
	pres_scale = ((65.0 / 1023.0) * (float)pres_comp) + 50.0;
	printf(R"Scaled pressure: %f kPa\n", pres_scale);
}

void  main(void)
{
	unsigned char buf[50];
	int i, len, p_adc, t_adc, a0, b1, c11, c12, b2, c22;
	len=16;
	init_clock();
	init_uart(_UART0, clock, _DEFBAUD);

    I2CBR = clock/(4 * BAUD);
    PAAF |= 0xC0;            
    I2CMODE = 0x00;                   // polling, 7bit, auto ack
                                      // no GCE, no slave addr
                                      // no diags
    I2CCTL = 0x80; // I2C Enable bit  

	printf(R"Size of int: %d, size of short: %d, size of long: %d\n",
		sizeof(int), sizeof(short), sizeof(long));

	while(1)
	{
		if(send_command(0x12))
		{
			printf(R"Successfully sent 0x12\n");
            
			delay(4); //3 ms conversion time after a command
            printf(R"Trying to read results of conversion...\n");
			if(!read_data(buf, len))
			{
				printf(R"Read failed\n");
				continue;
			}
			printf(R"Read: \n");
			for(i=0; i<len; i++)
			{
				printf(R"0x%02x ", buf[i]);
			}
			printf(R"\n");
			process_data(buf, len);
		}

		delay(1000);
	}

}
