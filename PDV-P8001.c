#include "PDV-P8001.h"
#include <stdio.h>
#include <zneo.h>

#define PDV_VIN 3.29
#define VIN_2 3.23
#define PDV_R1 9.76
#define PDV_OHMS_LUX 0.6
#define PDV_MAX_RES 11000
#define PDV_MIN_RES .002
/*
 * This solves the voltage divider equation
 * for the unknown resistance R2
*/
float PDV_8001_compute_lux(float vout)
{
	float r1;
	r1 = (vout * PDV_R1) / (PDV_VIN - vout);
	//convert r1 to ohms from k-ohms
	//printf(R"Illumination resistance: %f K Ohms\n", r1);
//	printf(R"Illumination voltage: %f V\n", vout);
///	return (PDV_MAX_RES - (r1 * 1000)) * PDV_OHMS_LUX;
	return r1;
}

float PDV_8001_read_ana0()
{
	int value, i;
    float ana0, avg=0.0;
	PBAFL = 0x01; //enable alt func for PB0, and PB1 which are ANA0 and ANA1
	ADC0CTL |= 0x10; //enable ADC0 and external reference
	
    ADCSST = 0x0F; //and a really long hold time
    ADCST = 0x0F; //do a long sample time
	
	ADC0CTL &= 0xF0; //select ANA0 by setting LS Nibble to 0
	while(ADC0CTL & 0x80); //wait for existing conversion before starting...
    for(i=0; i<10; i++)
    {
    	ADC0CTL |= 0x80; //Start conversion
    	while(ADC0CTL & 0x80);
		
    	value = (ADC0D >> 6);
        //from zilog spec P 244, 2.0 V is from the internal
        //reference generator, this converts it the the 
        //Voltage on the input pin
    	ana0 = ((float)value * VIN_2) / 1024.0;
        //from LM34 spec...
        //VOUT = + 10.0 mV/F
        //I think that means F = VOUT / 10 mV
        avg += ana0;
    }
	avg = avg/10.0;

	//Reverse the computation for ohms of resistance
	//9.6 was hand-measured
	return (avg * 9.6) / (VIN_2 - avg);
}
