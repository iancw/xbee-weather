#include "peripheral_cmds.h"
#include "shell.h"
#include "24lc16b_eeprom.h"
#include "25lc040a_eeprom.h"
#include "spi.h"

#include <zneo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define I2C_HELP R"\
24lc16b [fill|dump [start address, [length]] \n\
\t- fill memory with incremental data (0x00, 0x01 ...) starting\n\
\t  at address (specify in hex), continue for length bytes.\n\
\t- read from memory starting at address (specify in hex), continue\n\
\t  for length bytes. display output line in common hexdump\n\
\t  (http:\/\/en.wikipedia.org/wiki/Hex_dump) format.\n"
#define SPI_HELP R"\
25lc040a [fill|rdsr|wren|clear|dump [start address, [length]] \n\
\t- fill memory with incremental data (0x00, 0x01 ...) starting\n\
\t  at address (specify in hex), continue for length bytes.\n\
\t- Read contents of status register.  Bits 2 and 3 are block protect\n\
\t  bit 0 is write in process, and bit 1 is write enabled.  Higher bits\n\
\t  are undefined.\n\
\t- Set Write Enable bits in the status register.\n\
\t- Clear errors from the ESPI status register.\n\
\t- read from memory starting at address (specify in hex), continue\n\
\t  for length bytes. display output line in common hexdump\n\
\t  (http:\/\/en.wikipedia.org/wiki/Hex_dump) format.\n"

/*
 * Gets the start address from argv[1], returns
 * a default value of 0 if argc < 2.
 *
 * Expects argv[1] to contain a hexidecimal value
 */
int parse_start(int argc, char** argv)
{
	int add;
	if(argc >= 2)
	{
		sscanf(argv[1], "%x", &add);
		return add;
	}
	return 0;
}

/*
 * Gets the len from argv[2], returns a default value
 * of 10 if argc < 3
 */
int parse_len(int argc, char** argv)
{
	if(argc >= 3)
	{
		return atoi(argv[2]);
	}
	return 10;
}

void hex_dump(unsigned char* data, int s_address, int len)
{
	int i,j,line_addr;
	line_addr = s_address;
	for(i=0; i<len; i++)
	{
		if((i % 16) == 0)
		{
			printf(R"\n%06x ", line_addr);
			line_addr += 16;
		}else if((i%8)==0)
		{
			printf(R" - ");
		}else if((i%2)==0)
		{
			printf(R" ");
		}
		
		printf(R"%02x", data[i]);
		
	}
	printf(R"\n");
}

void init_peripheral_cmds()
{
    init_25LC040A();
    init_24LC16B();
	add_command(R"24lc16b", "", I2C_HELP, &i2c_cmd);
	add_command(R"25lc040a", "", SPI_HELP, &spi_cmd);
}

/*
 i2c [fill|dump [start address, [length]] 
 - fill memory with incremental data (0x00, 0x01 ...) starting
   at address, continue for length bytes. 
 - read from memory starting at address, continue for length
   bytes. display output line in common hexdump 
   (http:// en.wikipedia.org/wiki/Hex_dump) format.
 */
int i2c_cmd(char *name, int argc, char** args)
{
	unsigned char *data=0;
	int len, addr, i;
	if(argc > 1)
	{
		if(strcmp(args[0], "fill") == 0)
		{
			len = parse_len(argc, args);
			addr = parse_start(argc, args);
			
			//data = malloc(sizeof(char) * len);
			//for(i=0; i<len; i++)
			{
			//	data[i] = i;
			}
			write_24LC16B(addr, len, data);
			//free(data);
            return 0;
		}else if(strcmp(args[0], "dump")==0)
		{
			len = parse_len(argc,args);
			addr = parse_start(argc, args);
			
			data = malloc(sizeof(char) * len);
			read_24LC16B(data, addr, len);
			hex_dump(data, addr, len);
			free(data);
		    return 0;
        }
		printf(R"Unrecognized i2c argument %s\n", args[0]);
	}
	
	printf(I2C_HELP);
    return 0;
}

/*
 spi [fill|dump [start address, [length]] 
 - fill memory with incremental data (0x00, 0x01 ...) starting
   at address, continue for length bytes. 
 - read from memory starting at address, continue
   for length bytes. display output line in common 
   hexdump (http://en.wikipedia.org/wiki/Hex_dump) format.
 */
int spi_cmd(char* name, int argc, char** args)
{
	unsigned char *data;
	int len, addr, i;
	if(argc > 0)
	{
	    if(strcmp(args[0], "fill") == 0)
		{
			len = parse_len(argc, args);
			addr = parse_start(argc, args);
			
			data = malloc(sizeof(char) * len);
			for(i=0; i<len; i++)
			{
				data[i] = i;
			}
			fill_25LC040A(addr, len, data);
			free(data);
            return 0;
		}else if(strcmp(args[0], "dump")==0)
		{
			len = parse_len(argc,args);
			addr = parse_start(argc, args);
			
			data = malloc(sizeof(char) * len);
			read_25LC040A(data, addr, len);
			hex_dump(data, addr, len);
			free(data);
		    return 0;
        }else if(strcmp(args[0], "clear") == 0)
        {
            spi_clear_errors();
            return 0;
        }else if(strcmp(args[0], "rdsr") == 0)
        {
            printf(R"RDSR: 0x%02x\n", read_status_25LC040A());
            return 0;
        }else if(strcmp(args[0], "wren") == 0)
        {
            write_en_25LC040A();
            return 0;
        }
		printf(R"Unrecognized spi argument %s\n", args[0]);
	}
	
	//printf(SPI_HELP);
    printf(R"ESPIDATA: 0x%02x\n", ESPIDATA);
    printf(R"ESPITDCR: 0x%02x\n", ESPITDCR);
    printf(R"ESPICTL: 0x%02x\n", ESPICTL);
    printf(R"ESPIMODE: 0x%02x\n", ESPIMODE);
    printf(R"ESPISTAT: 0x%02x\n", ESPISTAT);
    printf(R"ESPISTATE: 0x%02x\n", ESPISTATE);
    printf(R"ESPIBR: 0x%04x\n", ESPIBR);
    return 0;
}
