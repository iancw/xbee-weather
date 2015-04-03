#ifndef __PREIPH_CMDS_H__
#define __PREIPH_CMDS_H__

/*
 * i2c [fill|dump [start address, [length]] 
	- fill memory with incremental data (0x00, 0x01 ...) starting
	at address, continue for length bytes. 
	- read from memory starting at address, continue for length
	bytes. display output line in common hexdump (http:// en.wikipedia.org/wiki/Hex_dump) format.
 
 spi [fill|dump [start address, [length]] 
	- fill memory with incremental data (0x00, 0x01 ...) starting
	at address, continue for length bytes. 
	- read from memory starting at address, continue
	for length bytes. display output line in common hexdump (http://en.wikipedia.org/wiki/Hex_dump) format.
 *
 */

void init_peripheral_cmds();

int i2c_cmd(char* name, int argc, char** argv);

int spi_cmd(char *name, int argc, char** argv);

#endif