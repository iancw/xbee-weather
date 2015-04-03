#include "uart.h"
#include <sio.h>
#include <zneo.h>
#include "timer.h"
#include "clock.h"

#define PARITY_EVEN ~0x08 //even parity requires a zero, and this with u0ctl0
#define PARITY_ODD 0x08 //odd parity requires a 1, or this with u0ctl0
#define PARITY_ENABLE 0x10 //PEN is bit 4 of u0ctl0
#define TWO_STOP 0x02 //or this with u0ctl0 to enable two stop bits
#define UART_ENABLE_TX 0x80
#define UART_ENABLE_RX 0x40

unsigned long baud = _DEFBAUD;

void init_uart0()
{
	init_uart(_UART0, get_clock(), baud);
}

void init_uart1(unsigned long _baud)
{
    init_uart(_UART1, get_clock(), _baud);
}

void uart_set_baud(unsigned long bd)
{
    long divisor;
    baud = bd;
    divisor = get_clock() / (16 * bd);
    if(divisor > 0xFFFF)
    {
        //Maximum baud exceeded
    }
    uart_enable(0);
    //baud (b/s) = hz / (16 * baud divisor)
    //we need to solve for the baud rate divisor, so
    //divisor = hz /(16 * baud)
	U0BRH = ((divisor & 0x0000ff00)>>8);
    U0BRL = (divisor & 0x000000ff);
    uart_enable(1);
}

void uart_enable(int able)
{
    if(able)
    {
        U0CTL0 |= UART_ENABLE_TX | UART_ENABLE_RX;
    }else
    {
        U0CTL0 &= ~UART_ENABLE_TX & ~UART_ENABLE_RX;
    }
}

unsigned long uart0_get_baud()
{
    return baud;
}

unsigned long uart1_get_baud()
{
    //baud = hz/16*divisor
    long baud;
    long divisor = 0l;
    divisor = (U1BRH << 8);
    divisor |= U1BRL;
    return get_clock() / (16 * divisor);
}

unsigned long compute_current_baud()
{
    //baud = hz/16*divisor
    long baud;
    long divisor = 0l;
    divisor = (U0BRH << 8);
    divisor |= U0BRL;
    return get_clock() / (16 * divisor);
}

void uart_set_parity(Parity p)
{
    switch(p)
    {
        case EVEN:
            U0CTL0 &= PARITY_EVEN;
            U0CTL0 |= PARITY_ENABLE;
            break;
        case ODD:
            U0CTL0 |= PARITY_ODD;
            U0CTL0 |= PARITY_ENABLE;
            break;
        case NONE:
            U0CTL0 &= ~PARITY_ENABLE;      
    }
}

void uart_set_bits(int num)
{
	switch(num)
    {
        case 7: 
            U0CTL0 &= ~TWO_STOP;
            break;
        case 8:
            U0CTL0 |= TWO_STOP;
            break;
    }
}
