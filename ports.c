#include "ports.h"
#include <zneo.h>
#include <stdlib.h>
#include <stdio.h>

#define BAD_PORT_ERR  R"Must specify a port between A and K\n"
#define REG_PRINT_LOOP R"P%c%s: 0x%02x\n"

#define NUM_REG 9
#define NUM_SPEC 3

rom char* _suf[NUM_REG]={R"IN", R"OUT", R"DD", R"HDE", R"AFH", R"AFL", R"OC", R"PUE", R"SMRE"};
rom char* _spec_suf[NUM_SPEC] ={R"IMUX1", R"IMUX", R"EDGE"};

void help_print_port(char c, unsigned char volatile near* pPort)
{
    unsigned char val, i=0;
    for(i=0; i<NUM_REG; i++)
    {
        val = *(pPort+i);
        printf(REG_PRINT_LOOP, c, _suf[i], val);
    }
    if(c == 'A')
    {
        printf(REG_PRINT_LOOP, c, _spec_suf[0], PAIMUX1);
        printf(REG_PRINT_LOOP, c, _spec_suf[1], PAIMUX);
        printf(REG_PRINT_LOOP, c, _spec_suf[2], PAIEDGE);
    }
}

void print_port(char c)
{
    unsigned int offset=0, shifted=0;
    unsigned char volatile near* pPort = &PAIN;
    if(c < 0)
	{
        printf(BAD_PORT_ERR);
		return;
	}
    if(c == 'I' || c == 'i' || c == 8)
    {
        printf(R"Port I does not exist\n");
        return;
    }
	if(c < 'K'-'A')
	{
		offset=c;
	}else if(c >= 'A' && c <= 'K')
	{
		offset=c-'A';
	} else if(c >= 'a' && c <= 'k')
	{
		offset = c-'a';
	}else
    {
        printf(BAD_PORT_ERR);
        return;
    }
    shifted = offset << 4;
    //The ports increment in units of 16 addresses, so
    //PAIN begins at E100, PBIN at E110, PCIN at E120, etc.
    pPort += shifted;
    help_print_port('A'+offset, pPort);
}
