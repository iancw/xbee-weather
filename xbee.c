#include "xbee.h"

#include <zneo.h>
#include <sio.h>
#include "clock.h"
#include <stdio.h>

#define BUF_SZ 200
//circular buffer
unsigned char xbee_buf[BUF_SZ];
int buf_ptr=0;
int last_read=-1;

void interrupt xbee_capture()
{
    xbee_buf[buf_ptr++] = U1RXD;
    printf(R"In interrupt, read 0x%02x\n", xbee_buf[buf_ptr-1]);
    if(buf_ptr >= BUF_SZ)
    {
        buf_ptr = 0;
    }
}
// Configure UART0

void init_xbee(unsigned long baud)
{
    // Set the baud rate
    // BRG = freq/( baud * 16)
    U1CTL0=0; //disable uart first?
    init_uart(_UART1, get_clock(), baud);
    //this guy hoses all the data we get out
   // U1CTL1 |= 0x40; //have to enable MPEN for interrupts?
    // U0 control
    // Transmit enable, Receive Enable, No Parity, 1 Stop
    //DI();
    //SET_VECTOR(UART1_RX, xbee_capture);
}


// return true if a key is pressed (data is waiting)

int xbee_keypressed() {
    return (U1STAT0 & 0x80);
    //return last_read != buf_ptr;
}


// transmit one character

void xbee_putchar(char c) { 

    // wait for trans buffer to empty
    while( 0==(U1STAT0 & 0x04)) { ; } 

    // send char
    U1TXD = c;

    // wait till its sent
    while( !(U1STAT0 & 0x06)) { ; } 
}


// transmit a bunch of characters

void xbee_print(char * text) {
    int i;
    char* s = text;
 
    // crap out after 100 characters
    for(i=0;i<100 && *s; i++) { 

        // wait for trans buffer to empty
        while( 0==(U1STAT0 & 0x04)) { ; } 

        // send char
        U1TXD = *s++;
    }

    // Wait for last byte to be sent
    while( !(U1STAT0 & 0x06)) { ; } 
}


// receive a character

unsigned char xbee_getchar()
{
    unsigned char c; 

    //whit untill a byte has been received
    while( !(U1STAT0 & 0x80)) { ; } 

    // get the byte and return it
    c = U1RXD;
    return c;
}

unsigned char xbee_getchar2() {
    unsigned char c;
    while(!xbee_keypressed());
    c = xbee_buf[last_read++];
    if(last_read >= BUF_SZ)
    {
        last_read = 0; //wrap around
    }
    return c;
}


