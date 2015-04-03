#ifndef __CLOCK_H__
#define __CLOCK_H__

//Sets the clock speed to be 18.432 MHz using
//the external crystal
void set_clock_18_432();

//Sets the clock speed to be 5.5 MHz using
//the internal oscillator
void set_clock_5_5();

//Gets the current clock speed in Hz
unsigned long get_clock();

void init_clock();

void delay(int ms);

#endif