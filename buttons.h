/*
 *  buttons.h
 *  
 *  A library to debounce and query the state of switches.
 *  The convention used in this api is that pressed = 1 and
 *  not-pressed = 0.  This is the reverse of how the hardware
 *  is wired (since it uses a pull-up), but matches the way 
 *  I think of buttons and is easier to keep straight conceptually.
 *
 *  Created by Ian Will on 2/6/11.
 *  Copyright 2011 Ian Will. All rights reserved.
 *
 */
#ifndef __I_BUTTONS_I__
#define __I_BUTTONS_I__

#define NUM_SWITCHES 5
enum Switch {
  SW1, SW2, SW3, SW4, SW5
};
typedef enum Switch Switch;

//Calls to methods that read buttons must be done
//at interval.  Value is in milliseconds.
#define POLL_INTERVAL 5

/*
 * This method sets the data direction on the switch 
 * GPIO ports and must be called before using any 
 * other methods from buttons.h
 */
void buttons_init();

/*
 * This debounces the signal and returns 1 if
 * the specified switch is currently pressed and
 * 0 if it is not.
 */
int buttons_read_switch(Switch sw);

/*
 * This returns 1 if the specified switch transitioned from
 * 0 to 1 since this was last called.
 */
int buttons_was_pressed(Switch sw);

/*
 * This returns 1 if the specified switch transitioned from
 * 1 to 0 since this was last called.
 */
int button_was_released(Switch sw);

#endif
