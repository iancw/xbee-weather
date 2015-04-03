/* 
 * Serial Port - The hard way.
 * Dan Eisenreich 
 * February 26, 2009
 * 
 */


#include <zneo.h>

#define FREQ 5529600UL              // use internal oscillator
#define BAUD 57600UL                // our desired baud rate


// Some symbols to make our code easier to read
#define PORTA_UART_RXD				0x10
#define PORTA_UART_TXD				0x20
#define UART_TXD_EN					0x80
#define UART_RXD_EN					0x40


// Configure UART0

void init_uart(void)
{
    // Set the alternate function on port A
    // Enable UART0 TxD0/RxD0 pins (bits 4 & 5)
    PAAF |= PORTA_UART_TXD | PORTA_UART_RXD;	

    // Set the baud rate
    // BRG = freq/( baud * 16)
    U0BR = FREQ/((unsigned long)BAUD*16UL);

    // U0 control
    // Transmit enable, Receive Enable, No Parity, 1 Stop
    U0CTL0 = UART_RXD_EN | UART_TXD_EN;      
    U0CTL1 = 0;

    // Thats all folks!
}


// return true if a key is pressed (data is waiting)

int keypressed() { 
    return (U0STAT0 & 0x80);
}


// transmit one character

void putchar(char c) { 

    // wait for trans buffer to empty
    while( 0==(U0STAT0 & 0x04)) { ; } 

    // send char
    U0TXD = c;

    // wait till its sent
    while( !(U0STAT0 & 0x06)) { ; } 
}


// transmit a bunch of characters

void print(char * text) {
    int i;
    char* s = text;
 
    // crap out after 100 characters
    for(i=0;i<100 && *s; i++) { 

        // wait for trans buffer to empty
        while( 0==(U0STAT0 & 0x04)) { ; } 

        // send char
        U0TXD = *s++;
    }

    // Wait for last byte to be sent
    while( !(U0STAT0 & 0x06)) { ; } 
}


// receive a character

unsigned char getchar() {
    unsigned char c; 

    //whit untill a byte has been received
    while( !(U0STAT0 & 0x80)) { ; } 

    // get the byte and return it
    c = U0RXD;
    return c;
}



void _other_main(void) { 
    int i=0;
    int c;
    char* message="Hello World ";

    init_uart(); 

    print(message);
    putchar(0x0a); putchar(0x0d);

    while(1) { 

        if (keypressed()) {
            c = getchar();

            putchar('[');
            putchar(c);
            putchar(']');
        }
    }   
}


