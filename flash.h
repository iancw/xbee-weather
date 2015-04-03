/*
 * This code is borred from a Zilog example which Professor Eisenreich shared with me
*/


#ifndef __FLASH_H__
#define __FLASH_H__

#define PAGE_SIZE 1024 //2 KB page size

void init_flash(unsigned long freq);
	
void page_unlock(_Erom unsigned short int*addr);

void page_erase(_Erom unsigned short int* addr);

void lock_flash(void);

#endif