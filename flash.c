#include "flash.h"

#include <zneo.h>

void init_flash(unsigned long freq)
{
	FFREQ = (unsigned short int) (freq / 1000);
}

/*
 * Must be called before a page can be written to
 */
void page_unlock(erom unsigned short int* addr)
{
	unsigned short int page;
	//get corresponding page
	page = (unsigned int) addr >> 11;
	page = page << 3; //shift out of reserved section
	
	//unlock sequence
	FPAGE = page;
	FCMD = 0x73;
	FCMD = 0x8C;
}

/*
 * Flash must be erased before it can be written
 */
void page_erase(erom unsigned short int* addr)
{
	page_unlock(addr);
	
	//Do actual erase
	FCMD = 0x95;
	while(FSTAT & 0x10);
}

/*
 * This will relock flash after unlocking for a write
 */
void lock_flash(void)
{
	FCMD = 0x00;
}
