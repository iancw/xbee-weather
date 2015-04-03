/*
 *  buttons.c
 *  
 *
 *  Created by Ian Will on 2/6/11.
 *  Copyright 2011 Ian Will. All rights reserved.
 *
 */
#include <zneo.h>
#include "buttons.h"

// Delay (in ms) to wait for a switch to 
//settle down after its been released
#define RELEASE_DELAY 20
//Delay (in ms) to wait for a switch to settle
// after it's been pressed
#define PRESS_DELAY 10
//Interval to poll the switches, used to convert
//the two above delays into a counter variable
//This is defined in buttons.h so external code
//can access it and set their timer intervals
//appropriately
//#define POLL_INTERVAL 5

//Poll the current state of the specified switch
int raw_poll(Switch sw);

//Computed the debounce countdown required
//for the next state transition, based on 
//the current state and the polling interval
int reset_countdown(int pressed);

//Debounce a raw value for the given switch
int debounce(int val, Switch sw);

/*
 * Reads the current (undebounced) value of the specified
 * switch and returns 1 if it is pressed or 0 if it
 * is not.  This flips the defaults, but it's easier
 * to interpret.
 */
int raw_poll(Switch sw)
{
	switch(sw)
	{
		case SW1:
			if(!(PDIN & 0x08))
			{
				return 1;
			}
			break;
		case SW2:
			if(!(PFIN & 0x40))
			{
				return 1;
			}
			break;
		case SW3:
			if(!(PFIN & 0x80))
			{
				return 1;
			}
			break;
		case SW4:
			if(!(PFIN & 0x20))
			{
				return 1;
			}
			break;
		case SW5:
			if(!(PGIN & 0x01))
			{
				return 1;
			}
			break;
		default:
			return 0;
	}
	return 0;
}

/*
 * Method used by debounce to compute the countdown
 * required based on whether the button was just pressed
 * or just released.
 * Switches bounce for different lengths in either case.
 */
int reset_countdown(int pressed)
{
	return pressed ? RELEASE_DELAY / POLL_INTERVAL : PRESS_DELAY / POLL_INTERVAL;
}

/*
 * This debounce algorithm came from an article on 
 * software debouncers by Jack Ganssle in EE Times, June 16, 2004
 * http://www.eetimes.com/discussion/break-points/4024981/My-favorite-software-debouncers
 * 
 * It was modified to support debouncing three switches simultaneously,
 * and for readability.  It works by maintaining a count for each switch
 * and reseting that count (using reset_countdown) each time the state 
 * changes.  It returns the previous value until the countdown is reached,
 * at which time the previous value is updated.  This call returns immediately
 * (instead of busy waiting for some timeout), so callers will have to handle
 * the delay appropriately.
 * 
 */
int debounce(int raw_val, Switch sw)
{
	static int debounced_val[NUM_SWITCHES];
    static int deb_count[NUM_SWITCHES] = {RELEASE_DELAY / POLL_INTERVAL, RELEASE_DELAY / POLL_INTERVAL, RELEASE_DELAY / POLL_INTERVAL};
	
	if(raw_val == debounced_val[sw])
	{
		deb_count[sw] = reset_countdown(raw_val);
	}else {
		if(--deb_count[sw] == 0){
			deb_count[sw] = reset_countdown(raw_val);
			debounced_val[sw] = raw_val;
		}
	}
	return debounced_val[sw];
}

/*
 * Initializes the data direction to input 
 * for PD3, PF6, and PF7.
 */
void buttons_init()
{
	PDDD |= 0x08;
	PFDD |= 0xE0;
	PGDD |= 0x01;
}

/*
 * Reads a debounced value from the
 * specified switch.
 */
int buttons_read_switch(Switch sw)
{
	return debounce(raw_poll(sw), sw);
}

/*
 * This saves the state of each switch
 * and returns 1 only when the switch
 * transitions from not-pressed to pressed.
 */
int buttons_was_pressed(Switch sw)
{
    static int state[] = {0, 0, 0, 0, 0};
    int last_state;
    last_state = state[sw];
    state[sw] = buttons_read_switch(sw);
    return state[sw] && !last_state;
}

/*
 * This saves the state of each switch
 * and returns 1 when the state transitions
 * from 1 to 0 since the last call.
 */
int button_was_released(Switch sw)
{
    static int state[] = {0, 0, 0, 0, 0};
    int last_state;
    last_state = state[sw];
    state[sw] = buttons_read_switch(sw);
    return !state[sw] && last_state;
}
