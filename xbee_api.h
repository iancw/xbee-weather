#ifndef __XBEE_API_H__
#define __XBEE_API_H__

#include "xbee.h"


struct xbee_header{
	unsigned char start;
	unsigned short length;
	unsigned char api_id;
	unsigned char cmd_data[1];
};
typedef struct xbee_header xbee_frame;

/*
 chan_ind: Channel indicator.
   - bits 0-8 are for digital, bits 9-14 are for analog
     bit0 = D0, bit1 = D1, ... bit9=A0, ... etc
 sample_data:
   - digital data is the first short if any of bits 0-8 are set
     that's followed by two byte, right justified 10 bit ADC
     channel data.
 */
struct io_data{
	//these apply to any rx frame, really i should
	//break them out into their own struct
	unsigned short source_add;
	unsigned char rssi;
	unsigned char options;
	//these are io frame specific
	unsigned char num_samps;
	unsigned short chan_ind;
	unsigned short sample_data[1];//we will overrun this, nice not having an OS 
};
typedef struct io_data io_data;

void xbee_print_frame();
void xbee_print_raw_frame();
int xbee_check_frame();

/*
 * Returns the number of samples processed
 */
int xbee_get_samples(unsigned short *buf, int max);
unsigned short xbee_get_source();
unsigned char xbee_get_rssi();

#endif