/*
 *  led.h
 *  
 *  An API that faciliates lighting
 *  up a 5x7 led panel in the Zilog Z16 contest kit.
 *
 *  This library usese TIMER 2.
 *
 *  Copyright 2011 Ian Will. All rights reserved.
 *
 */
#ifndef _I_LED_I_
#define _I_LED_I_

#define LED_SCROLL_LEFT 0
#define LED_SCROLL_RIGHT 1

/*
 * Enum to faciliate referring to each bank of leds.
 * Labels correspond to the labels printed on the circuit board
 * and in the schematic.
 */
enum LED_Bank { D1, D2, D3, D4};
typedef enum LED_Bank LED_Bank;

/*
 * This must be called before using any of the other
 * functions in the led library.  It initializes
 * local data structures and sets the output
 * direction of ports E and G.
 */
void led_init();

/*
 * Updates the current message, preserving the current scroll position
 */
void led_update(char const * msg);

/*
 * Set the message currently displayed in on the LED panel,
 * this will reset the scroll position, use led_update_message
 * to avoid interrupting the scroll pattern.
 */
void led_message(char const * msg);

/*
 * Displays the given floating point number on the led screen
 */
void led_message_float(float num);
/*
 * Displays the given number on the led screen in decimal
 */
void led_message_dec(int num);

/*
 * Displays the given number on the led screen in hexidecimal
 */
void led_message_hex(int num);

/*
 * Sets the speed at which characters are scrolled across the
 * led panel.  The value defines the number of seconds between
 * shift led columns in the current scroll direction.
 */
void led_set_scroll(float secs);

/*
    Return the current led scroll speed in seconds
*/
float led_get_scroll();

/*
 * Increments scroll speed by the given percent. Example:
 * 0.1 increases scroll speed by 10 percent.
 */
void led_inc_scroll(float percent);
/*
 * Decreases scroll speed by the given percent.
 */
void led_dec_scroll(float percent);

/*
 * Turns led scrolling on 
 */
void led_scroll_on();

/*
 * Turns led scrolling off
 */
void led_scroll_off();

/*
 * Sets the led scroll direction, 0 for left, 1 for right.
 * You can use LED_SCROLL_LEFT and LED_SCROLL_RIGHT for convenience.
 */
void led_scroll_directin(int dir);

/*
 * Changes the led scroll direction to be the opposite of its current
 * scroll direction
 */
void led_toggle_direction();

/*
 * Stops led character scrolling if it's on, or starts it if its off.
 */
void led_toggle_scroll();

/*
 * Resets the scroll offset for an led message.  Use this after
 * setting a message if you want it displayed from the leftmost 
 * character instead of the current scroll position.
 */
void led_reset_scroll();

/*
 * Sets the led refresh rate, this is the number of seconds that
 * an led column will be turned on, and it is also the number of
 * seconds that the led panel will be blank between lightint up
 * the next column.
 */
void led_set_refresh(float secs);

/*
 * Gets the current LED refresh rate
 */
float led_get_refresh();

/*
 * Increments the led refresh interval by the given percentage.
 * Note that increasing the refresh interval will actually cause
 * the leds to be refreshed less frequently, and will eventually
 * result in noticable flicker.
 */
void led_inc_refresh(float percent);

/*
 * Decreases the led refresh interval by the given percentage.
 * This caps the refresh rate at 0.  This will decrease flicker.
 */
void led_dec_refresh(float percent);

/*
 * Turn the specified bank of leds all on
 */
void led_bank_on(LED_Bank bank);

/*
 * Turns the specified bank of leds all off
 */
void led_bank_off(LED_Bank bank);

/*
 * Turns all leds on all banks on and off
 */
void led_all_on();
void led_all_off();

/*
 * Begins displaying the current message
 */
void led_start_display();
/*
 * Stops displaying the current message
 */
void led_stop_display();

/*
 * Initializes the auxilary leds 
 */
void led_aux_init();
/*
 * Sets the number to be displayed on the 
 * auxilary leds.  A 1 represents led on, a 0 off.
 */
void led_aux_set(int num);

/*
 * Turn all auxilary leds off
 */
void led_aux_clear();

#endif
