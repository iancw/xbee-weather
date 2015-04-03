#include <zneo.h>
#include <sio.h>
#include <stdio.h>
#include <string.h>
#include "flash.h"

erom unsigned short int data[PAGE_SIZE] _At 0x14000;
char test_data[100] = "Lah de dah, blah blah blah\n lorem ipsum, etc\0";

void main(void)
{
	int osc, i;
	long clock=5529600l;//internal
	char reverse[100];
	_Erom char  *rev_ptr;
    osc = OSCCTL & 0x03;
    if(osc == 0)
        clock = 5529600l;//internal
    if(osc == 1)
        clock = 18432000l;//external
    if(osc == 3)
        clock = 10000l;//watchdog

	init_flash(clock);
	init_uart(_UART0, clock, _DEFBAUD);
	
	printf("Previously: \n");
	for(i=0; i<110; i++)
	{
		if((i % 10) == 0)
		{
			printf("\n");
		}
		printf("0x%02x ", data[i]);
	}
	
	page_erase(data);
	
	printf("\nAfter erase: \n");
	for(i=0; i<110; i++)
	{
		if((i % 10) == 0)
		{
			printf("\n");
		}
		printf("0x%02x ", data[i]);
	}
	
	page_unlock(data);
	
	for(i=0; i<100; i++)
	{
		data[i] = test_data[i];
	}
	
	lock_flash();
	printf("\nWrote: \n");
	for(i=0; i<110; i++)
	{
		if((i % 10) == 0)
		{
			printf("\n");
		}
		printf("0x%02x ", data[i]);
	}
	rev_ptr = (_Erom char *)data;
	printf("\n(%s)\n", rev_ptr);
	printf("data=0x%08x, &data=0x%08x, *data=0x%08x\n", data, &data, *data);
	printf("rev_ptr=0x%08x, &rev_ptr=0x%08x, *rev_ptr=0x%08x\n", rev_ptr, &rev_ptr, *rev_ptr);
	printf("Erom ptr size: %d, reg ptr size: %d\n", sizeof(_Erom char *), sizeof(char*));
	for(i=0; i<100; i++)
	{
		reverse[i] = data[i];
	}
	printf("\n(%s)\n", reverse);
}
