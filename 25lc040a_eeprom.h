#ifndef __25LC040A_SPI_H__
#define __25LC040A_SPI_H__

#define MAX_ADDRESS 512

void init_25LC040A();
void read_25LC040A(unsigned char* data, int start_add, int len);
void fill_25LC040A(int start_add, int len, unsigned char* data);
unsigned char read_status_25LC040A();
void write_en_25LC040A();

#endif