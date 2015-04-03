#ifndef SPI_H
#define SPI_H

void spi_init();

unsigned char spi_read(unsigned char addr);

void spi_write(unsigned char addr, unsigned char data);

unsigned char spi_cycle(unsigned char out);

void spi_wait_TDRE();
void spi_wait_RDRF();
void spi_wait_TFST();
void spi_clear_errors();

#endif

