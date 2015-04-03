/*
 *  timer.h
 *  
 *  Defines an API that faciliates configuring
 *  timers for the ZNEO.  The code here is based on the code suplied
 *  by Dan Eisenreich in his timer example files.  His comments are below.
 *
 * Initialize TIMER0 
 * Method TWO using symbolic defines to improve readability
 * 
 * All these do is define TEXT labels for each of the many bit 
 * fields that we need for configuring the timers. All of the 
 * values are straight from the product specification. 
 *
 */

#ifndef __TIMER_H__
#define __TIMER_H__

enum Timer {T0=0, T1=1, T2=2};
typedef enum Timer Timer;

void set_clock_18_432();
void set_clock_5_5();
unsigned long get_clock(); //gets clock speed in MHz

void timer_init();

void timer_start_cont(Timer t, float secs);
void timer_start_oneshot(Timer t, float secs);
void timer_stop(Timer t);
void timer_continue(Timer t);

void timer_irq_disable(Timer t);
void timer_enable_irq_low(Timer t);
void timer_enable_irq_nom(Timer t);
void timer_enable_irq_high(Timer t);

void timer_pin_out_enable(Timer t);
void timer_pin_out_disable(Timer t);
void timer_pin_in_enable(Timer t);
void timer_pin_in_disable(Timer t);

//Timer querying functions
unsigned char timer_get_CTL0(Timer t);
unsigned char timer_get_CTL1(Timer t);
unsigned short timer_get_reload(Timer t);
unsigned short timer_get_value(Timer t);

int timer_get_prescale(Timer t);
int timer_is_enabled(Timer t);
int timer_get_polarity(Timer t);
int timer_get_mode(Timer t);
int timer_get_interrupt_config(Timer t);
int timer_is_cascade(Timer t);
int timer_get_PWM_delay(Timer t);
int timer_was_capture(Timer t);

#endif
