#ifndef __UART_CMD_H__
#define __UART_CMD_H__

enum Parity {EVEN=0, ODD, NONE};
typedef enum Parity Parity;

/*
 Starts UART0 with default clock and baud rates
*/
void init_uart0();

void init_uart1();

/*
 Disables the uart, sets the baud rate, and reneables.
*/
void uart_set_baud(unsigned long bd);

/*
 Sets the parity
*/
void uart_set_parity(Parity p);

/*
 Sets the number of bits to use as stop bits, takes
 7 or 8 as the arguments, 7 for 1 stop bit, 8 for 2.
*/
void uart_set_bits(int num);

/*
 Returns the baud rate that has been set in the past, 
 this won't be change if clock has changed underneath you.
*/
unsigned long uart0_get_baud();
unsigned long uart1_get_baud();

/*
 Pass a 0 for disable and a 1 for enable
*/
void uart_enable(int);
#endif