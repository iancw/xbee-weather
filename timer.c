/*
 *  timer.c
 *  
 *  Implementation for timer.h, an API that faciliates configuring
 *  timers for the ZNEO.  The code here is based on the code suplied
 *  by Dan Eisenreich in his timer example files.
 *
 *  Copyright 2011 Ian Will. All rights reserved.
 *
 */

#include <zneo.h>
#include "timer.h"
#include "timer_defines.h"
#include <stdio.h>
#include "clock.h"

#define CTL1_PRES_MASK 0x38

float get_prescale_step(int prescale);
float get_prescale_hz(int prescale);
int get_prescale_divider(int prescale);
unsigned int find_ideal_prescale(float seconds);
void finish_timer_start(Timer t, float timeout);

struct timer
{
    unsigned char volatile near* ctl0;
    unsigned char volatile near* ctl1;
    unsigned short volatile near* reload;
    unsigned short volatile near* value;
    unsigned char irq_vector;
    unsigned char volatile near* port_dd;
    unsigned char pin_in;
    unsigned char pin_out;
    unsigned short volatile near* port_af;
};
typedef struct timer timer;

timer timers[3];


unsigned char timer_get_CTL0(Timer t)
{
    return *timers[t].ctl0;
}
unsigned char timer_get_CTL1(Timer t)
{
    return *timers[t].ctl1;
}
unsigned short timer_get_reload(Timer t)
{
    return *timers[t].reload;
}
unsigned short timer_get_value(Timer t)
{
    return *timers[t].value;
}

void init_timers()
{
    init_clock();
    timers[0].ctl0 = &T0CTL0;
    timers[0].ctl1 = &T0CTL1;
    timers[0].reload = &T0R;
    timers[0].value = &T0HL;
    timers[0].irq_vector = 0x20;
    timers[0].port_dd = &PADD;
    timers[0].pin_in = 0x01;//PA0
    timers[0].pin_out = 0x02;//PA1
    timers[0].port_af = &PAAF;

    timers[1].ctl0 = &T1CTL0;
    timers[1].ctl1 = &T1CTL1;
    timers[1].reload = &T1R;
    timers[1].value = &T1HL;
    timers[1].irq_vector = 0x40;
    timers[1].port_dd = &PCDD;
    timers[1].pin_in = 0x01; //PC0
    timers[1].pin_out = 0x02; //PC1
    timers[1].port_af = &PCAF;

    timers[2].ctl0 = &T2CTL0;
    timers[2].ctl1 = &T2CTL1;
    timers[2].reload = &T2R;
    timers[2].value = &T2HL;
    timers[2].irq_vector = 0x80;
    timers[2].port_dd = &PCDD;
    timers[2].pin_in = 0x40; //PC6
    timers[2].pin_out = 0x80; //PC7
    timers[2].port_af = &PCAF;
}

void timer_init()
{
    init_timers();
}

float get_prescale_step(int prescale)
{
	return (float)get_prescale_divider(prescale) / (float)get_clock();
}

float get_prescale_hz(int prescale)
{
	return  (float)get_clock() / (float)get_prescale_divider(prescale);
}

/*
 * Takes prescale bits as input, shifts them right
 * and preforms another shift left and returns the 
 * actual divider (e.g. 128)
 */
int get_prescale_divider(int prescale)
{
	unsigned int value;
	//The prescale section is in bits 3-5 of TxCTL1
	//It represents a bitshift left value (7 = shift left 7)
	//It must be shifed right 3 digits to extract it
	value = (prescale >> 3) & 0x07;
	return 0x01 << value;
}

/*
* Finds the ideal prescale for the given deal based
* on clock speed.  Returns a shifted prescale ready
* for use with TxCTL1.
*/
unsigned int find_ideal_prescale(float seconds)
{
	unsigned int i, scale;
	float max;
	for(i=0; i<7; i++)
	{
		scale = 0x01 << i;
		max = (float)0xffff * (float) scale / (float)get_clock();
		if(seconds <= max)
		{
			break;
		}
	}
	return (i << 3) & CTL1_PRES_MASK;
}

int timer_get_prescale(Timer t)
{
    //prescal is in CTL1 in bits 3,4,5
    unsigned char ctl1 = timer_get_CTL1(t);
    return get_prescale_divider(ctl1);

}
int timer_is_enabled(Timer t)
{
    //enabled is bit 7 of CTL1
    return ((timer_get_CTL1(t) & TIMER_ENABLE) >> 7);
}
int timer_get_polarity(Timer t)
{
    //polarity is bit 6 of CTL1
    return ((timer_get_CTL1(t) & 0x40) >> 6);

}
int timer_get_mode(Timer t)
{
    unsigned char ctl0, ctl1, ret;
    //mode is bits 0-2 of CTL1 and bit 7 of CTL0
    ctl1 = timer_get_CTL1(t);
    ctl0 = timer_get_CTL0(t);
    ret = ctl1 & 0x03; //mask out everything but the mode part of CTL1
    ret |= ((ctl0 & 0x80) >> 4); //mask out the mode bit of ctl0, shift it to bit 3 and add it to ret
    return ret;
}
int timer_get_interrupt_config(Timer t)
{
    //interrupt confit (TICONFIG) is bits 5 and 6 of CTL0
    return ((timer_get_CTL0(t) & 0x60) >> 5);
}
int timer_is_cascade(Timer t)
{
    //cascade is bit 4 of CTL0
    return ((timer_get_CTL0(t) & 0x10) >> 4);
}
int timer_get_PWM_delay(Timer t)
{
    //PWM delay is bits 1-3 of CTL0
    return ((timer_get_CTL0(t) & 0x0E) >> 1);
}
int timer_was_capture(Timer t)
{
    //INCAP is bit 0, its 0 if the previous timer interrupt is not a sresult of a timer input capture
    //and 1 if it is.
    return (timer_get_CTL0(t) & 0x01);
}

//private method 
void finish_timer_start(Timer t, float timeout)
{
    int scale_bits;
    scale_bits = find_ideal_prescale(timeout);
    *timers[t].ctl1 |= scale_bits | TIMER_TPOL_0;
    *timers[t].ctl0 = TIMER_CASCADE_NOT + TIMER_PWM_DELAY_0 + TIMER_INPUT_CAPTURE_OFF;
    *timers[t].value = 0x00;
   // prescale = select_prescale(timeout);
    *timers[t].reload = (float)get_clock() / (float)get_prescale_divider(scale_bits) * timeout;
    *timers[t].ctl1 |= TIMER_ENABLE;
}

void timer_start_cont(Timer ti, float timeout)
{
    *timers[ti].ctl1 &= TIMER_DISABLE;
    *timers[ti].ctl1 = TIMER_MODE_CONTINUOUS;
    finish_timer_start(ti, timeout);
}

void timer_start_oneshot(Timer ti, float timeout)
{
    *timers[ti].ctl1 &= TIMER_DISABLE;
    *timers[ti].ctl1 = TIMER_MODE_ONESHOT;
    finish_timer_start(ti, timeout);
}

void timer_stop(Timer t)
{
    *timers[t].ctl1 &= TIMER_DISABLE;
}

void timer_continue(Timer t)
{
    *timers[t].ctl1 |= TIMER_ENABLE;
}

void timer_disable_irq(Timer ti)
{
    IRQ0ENL &= ~timers[ti].irq_vector;
    IRQ0ENH &= ~timers[ti].irq_vector;
}

void timer_enable_irq_low(Timer ti)
{
    IRQ0ENL |= timers[ti].irq_vector;//enable at low priority
    IRQ0ENH &= !timers[ti].irq_vector; // clear high big
}

void timer_enable_irq_nom(Timer ti)
{
    IRQ0ENL &= ~timers[ti].irq_vector; //clear the bit on the low for nominal priority
    IRQ0ENH |= timers[ti].irq_vector; //enable at high priority   
}

void timer_enable_irq_high(Timer ti)
{
    IRQ0ENL |= timers[ti].irq_vector;
    IRQ0ENH |= timers[ti].irq_vector;
}

void timer_pin_out_enable(Timer t)
{
    *timers[t].port_dd &= ~timers[t].pin_out;// set to 0 for output
    *timers[t].port_af |= timers[t].pin_out;
}

void timer_pin_out_disable(Timer t)
{
    *timers[t].port_dd |= timers[t].pin_out; //set to 1 to disable output
}

void timer_pin_in_enable(Timer t)
{
    *timers[t].port_dd |= timers[t].pin_in; //set to 1 for input
    *timers[t].port_af |= timers[t].pin_in;
}

void timer_pin_in_disable(Timer t)
{
    *timers[t].port_dd &= ~timers[t].pin_in; //0 to disable input
}

