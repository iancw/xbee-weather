#ifndef __24LC16B_I2C_H__
#define __24LC16B_I2C_H__

/*
 * A library for the 16K I2C Microchip 24LC16B EEPROM
 * chip.
 */

void read_24LC16B(unsigned char* data, int start, int len);
void write_24LC16B(int start, int len, unsigned char *data);
void init_24LC16B();

#endif