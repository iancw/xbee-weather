#ifndef ds1722_H
#define ds1722_H

unsigned char ds1722_read_byte(unsigned char addr);

void ds1722_init(void);

unsigned char ds1722_get_config(void);

int ds1722_get_temp(void);

float ds1722_get_ftemp(void);

#endif
