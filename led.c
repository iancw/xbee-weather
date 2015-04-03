/*
 *  led.c
 *  
 *  Implementation for led.h, an API that faciliates lighting
 *  up a 5x7 led panel in the Zilog Z16 contest kit.
 *
 *  Copyright 2011 Ian Will. All rights reserved.
 *
 */
#include <zneo.h>
#include <stdio.h>
#include "led.h"
#include "timer.h"
#include "led_ascii.h"

/*
 *Private data
 */

/*
 * Shared data structures
 */
struct bank_pin {
  unsigned char latch_pin;
  unsigned char volatile near* port;
};
typedef struct bank_pin bank_pin;

//An array of banks, initialized by init_banks method,
//designed to be indexed by the LED_Bank enum (defined
//in led.h)
bank_pin banks[4];

// The (global) refresh rate for the LED panel.
// This defines the number of seconds between
// refreshing all LED banks.
float g_refresh = .0011;

// The (global) scan rate for messages longer
// than 4 characters.  This defines the rate
// at which characters will move one column.
float g_scan_rate = .01;

// This is a counter of how many interrupt events
// (controlled by g_refresh) must occur before
// its time to shift columns left (controlled by
// g_scan_rate).  This allows us to use a single
// timer to control both rates.
int g_interrupt_scan_count = 9; //g_scan_rate / g_refresh;

int g_is_scrolling = 1;

int display_going =0;

//0 = left, 1 = right
int scroll_direction=1;

/*
 * This defines the maximum number of characters (not columns)
 * that can be displayed in the LED panel at once.
 */
#define MAX_LED_CHARS 100
#define MAX_LED_COLS 606

// This is the data that will be displayed
// by the led panel.  It contains data
// in column major order.  Each character
// represents a setting for one column,
// or, it represents the data sent to
// all anodes, if one cathode is on
// and the rest are off.
char g_data[MAX_LED_COLS] = {
0x7c, 0x02, 0x11, 0x02, 0x7c, //A
0x00,
0x7f, 0x41, 0x41, 0x22, 0x1c, //D
0x00,
0x7f, 0x09, 0x19, 0x36, 0x40, //R
0x00,
0x41, 0x41, 0x7f, 0x41, 0x41, //I
0x00,
0x7f, 0x49, 0x49, 0x49, 0x49, //E
0x00,
0x7f, 0x02, 0x0c, 0x30, 0x7f, //N
0x00,
0x7f, 0x02, 0x0c, 0x30, 0x7f, //N
0x00,
0x7f, 0x49, 0x49, 0x49, 0x49, //E
0x00,
0x00, 0x00, 0x00, 0x00, 0x00,
0x00,
0x0e, 0x3f, 0x7e, 0x3f, 0x0e, 0x00, //heart
0x0e, 0x3f, 0x7e, 0x3f, 0x0e, 0x00,  // heart
0x0e, 0x3f, 0x7e, 0x3f, 0x0e, 0x00, //heart
0x00, 0x00, 0x00, 0x00, 0x00,
0x00
 };

//Length of the g_data array in chars,
//this corresponds to the number of columns
//in the current message
int g_data_len = 6*13;

int shift_offset =0;

/*
 * Private methods
 */
//Sets all leds to 0
void init_banks();
//Cycles one bank, as specified by the bank_pin
void cycle_bank(bank_pin bank);
//Shifts the current index of displayed data left
//by rotation.  Negative values of rotation shift right.
void shift_left(int rotation);
//Refreshes the next column of leds, tracks which led
//column needs to be toggled next.
void led_refresh();
//Writes the given column of data to each bank of 
//leds, used by led_refresh to light up leds
//on a single column (same column but all banks)
void write_blocks(int col, char* data);
//converts a character to the five column value
//necessary to draw that char on the 5x7 panl
void char_to_cols(char c, char* dest);
//used by led_refresh to correlate its refresh callbacks
//with the scan rate
void update_scan();
//the method that updates the message and length of the shared
//message buffer g_data
void led_message_internal(char const * msg);
//used by led_message_internal to test messages if they're
//greater than 4 chars long
int msg_over_four(char const * msg);


/*
 * Initializes the banks array
 */
void init_banks()
{
  banks[0].latch_pin = 0x80;
  banks[0].port = &PEOUT;
  banks[1].latch_pin = 0x80;
  banks[1].port = &PGOUT;
  banks[2].latch_pin=0x20;
  banks[2].port = &PEOUT;
  banks[3].latch_pin=0x40;
  banks[3].port = &PEOUT;
}

/*
 * Cycles the latch on the given pin by 
 * setting it low, then high (rising edge)
 */
void cycle_bank(bank_pin bank)
{
  *bank.port &= ~bank.latch_pin;
  *bank.port |= bank.latch_pin;
}



/*
 * Writes the appropriate contents of data to the
 * specified column on each of the four banks of
 * LEDs.  The col variable corresponds to the
 * shared cathode for a column, and should be
 * between 0 and 4.  Data should contain at least
 * 20 characters.
 *
 * This uses the pin and block_latch arrays to 
 * cycle each block of LEDs in order.
 *
 */
void write_blocks(int col, char* data)
{
    int block, pos;
    for(block=0; block<4; block++)
    {
        //First clear all our bits
        PGOUT &= 0x80;
        PEOUT &= 0xe0;
        pos = (shift_offset + (block*5 + col));
        if(g_data_len > 20)
        {
            pos = pos % g_data_len;
        }
        PGOUT |= (~0x80 & data[pos]);
        PEOUT |= (~0xe0 & ~(0x10 >> col));
        cycle_bank(banks[block]);
    }
}

/*
 * This is the ISR for TIMER0. Its job is
 * to refresh the LED panels.  Every time
 * its called, it turns one column on for
 * each LED bank and leaves it on until the
 * next time its called.
 */ 
void interrupt led_refresh()
{
  static int col=0;
  if(col>=5)
  {
    col=0;
  }
  led_all_off();
  write_blocks(col, g_data);
  col++;

  if(g_is_scrolling)
  {
	  update_scan();
  }
}

/*
 * This method handles scanning a message and is called
 * by led_refresh.  It counts the number of times its
 * been called, and examines g_interrupt_scan_count to
 * determine when it's time to scan one colum to the left.
 */
void update_scan()
{
	static int scan_count=0;
	//The g_interrupt_scan_count variable controls the scan speed, 
	// It's a refresh rate multiplier, based on the current settings
	// of g_refresh and g_scan_rate
	if(scan_count > g_interrupt_scan_count && g_data_len > 20)
	{
		scan_count=0;
		shift_left(scroll_direction);
	}else
	{
		scan_count++;
	}
}

/*
 * Shifts the contents of data to the left
 * one index, wraps the zero index around
 * to the end.
 * Size must specify the size of the data 
 * array
 */
void shift_left(int rotation)
{
	shift_offset = (shift_offset +rotation) % g_data_len;
    //The modulo operator here doesn't wrap around zero
    if(shift_offset < 0)
    {
        shift_offset = g_data_len-5;
    }
}

/*
 * Converts a character (c) to the five columns
 * necesary to display that character on the 
 * four LED panels.  The dest argument must point
 * to a character array at least five characters
 * long.
 */
void char_to_cols(char c, char* dest)
{
    int i, c_idx;
    unsigned char* cols;
	
    c_idx = (int)c;
    c_idx -= 32;
    
    cols = led_ascii[c_idx];
    for(i=0; i<5; i++)
    {
        dest[i] = cols[i];
    }
}

/*
 * Returns 1 if msg has over 4 characters, 0
 * if it four or less.
 */
 int msg_over_four(char const * msg)
{
	int i;
	for(i=0; msg[i] != '\0' && i<5; i++);
	return i == 5;
}

/**
 * Updates the message displayed in the LED data buffer (g_data)
 */
void led_message_internal(char const * msg)
{
	int i, j, cols_per_char, over_four;
	char cols[5];
	
	//First see if the message is greater than 4 characters,
    //if so, we'll need to scroll it, and for that to look
    //correct (because scrolling is by column and not by bank)
    //we must insert a blank column between each character.
    //This only applies for messages greater than 4 characters
	over_four = msg_over_four(msg);
	if(over_four)
	{
		cols_per_char=6;
	}else{
        shift_offset=0;
		cols_per_char = 5;
	}
	DI();   
	
	//Clear the first four characters, only if the message
	//is less than four characters
	for(i=0; i<20 && !over_four; i++)
	{
		g_data[i] = 0x00;
	}

	for(i=0; msg[i] != '\0' && i < MAX_LED_CHARS; i++)
	{
		char_to_cols(msg[i], cols);
		//copy each column into g_data
		for(j=0; j<5; j++)
		{
			g_data[i*cols_per_char + j] = cols[j];
		}
		//Insert an extra space if the message needs to be
		//scanned
		if(over_four)
		{
			//Put a space between characters.
			g_data[i*cols_per_char+j] = 0x00;
		}
	}
	//If we're scanning, add an extra space at the end of the message
	if(over_four && i < MAX_LED_CHARS)
	{
		char_to_cols(' ', cols);
		for(j=0; j<5; j++)
		{
			//we can reuse i here, because when the loop exits i will
			//be one greater than it was in the last loop
			g_data[i*cols_per_char + j] = cols[j];
		}
		g_data_len = i*cols_per_char + j;
	}else
	{
        g_data_len = i*cols_per_char;
	}
	
	if(!display_going)
	{
		led_start_display();
	}else {
		//Update the shift offset so it isn't
		//greater than the current message length
		shift_left(0);
	}
	
	EI();
}


/*
 * Public methods
 */

/*
 * This must be called before using any of the other
 * functions in the led library.  It initializes
 * local data structures and sets the output
 * direction of ports E and G.
 */
void led_init()
{
  timer_init();
  init_banks();
    //Set data direction as output for port E and G
  PEDD = 0x00;
  PGDD = 0x00;
}

void led_aux_init()
{
    PFDD  &= ~0x0F;    // Set the data direction to OUT
    PFOUT &= ~0x0F;    // Initialize them to all off
}

void led_aux_set(int num)
{
  PFOUT = (PFOUT & 0xF0) | (num & 0x0F);
}  

void led_aux_clear()
{
  PFOUT &= ~0x0F;
}

void led_start_display()
{
  timer_start_cont(T2, g_refresh);
  
  timer_enable_irq_low(T2);
  //TIMER_init0_cont(g_refresh);
  display_going=1;
  SET_VECTOR(TIMER2, led_refresh);
  EI();
}

void led_reset_scroll()
{
  shift_offset=0;
}


void led_update(char const * msg)
{
    led_message_internal(msg);
}

void led_message(char const * msg)
{
  led_reset_scroll();
  led_message_internal(msg);
}

void led_message_float(float f)
{
  char float_str[20];
  sprintf(float_str, "%f", f);
  led_message(float_str);
}

void led_message_dec(int num)
{
    char str[20];
    sprintf(str, "%u", num);
    led_message(str);
}

void led_message_hex(int num)
{
    char str[20];
    sprintf(str, "0x%X", num);
    led_message(str);
}


void led_scroll_off()
{
  g_is_scrolling = 0;
}


void led_scroll_on()
{
  g_is_scrolling = 1;
}

void led_scroll_direction(int dir)
{
    if(dir)
    {
        scroll_direction = -1;
    }else
    {
        scroll_direction = 1;
    }
}

void led_toggle_direction()
{
    scroll_direction *= -1;
}

void led_toggle_scroll()
{
  g_is_scrolling = !g_is_scrolling;
}

void led_set_scroll(float secs)
{
  if(secs > 0)
  {
      g_scan_rate = secs;
      g_interrupt_scan_count = g_scan_rate / g_refresh;
   }
}

float led_get_scroll()
{
    return g_scan_rate;
}

void led_inc_scroll(float percent)
{
  led_set_scroll(g_scan_rate + percent*g_scan_rate);
}

void led_dec_scroll(float percent)
{
  led_set_scroll(g_scan_rate - percent*g_scan_rate);
}


void led_set_refresh(float secs)
{ 
  if(secs > 0)
  {
    g_refresh = secs;
    timer_start_cont(T2, g_refresh);
    g_interrupt_scan_count = g_scan_rate / g_refresh;
  }
}

float led_get_refresh()
{
    return g_refresh;
}

void led_inc_refresh(float percent)
{
    led_set_refresh(g_refresh + g_refresh*percent);
}

void led_dec_refresh(float percent)
{
    led_set_refresh(g_refresh - g_refresh*percent);
}

void led_bank_on(LED_Bank e_bank)
{
  PEOUT &= ~0x1f; //set lower 5 bits low on cathodes
  PGOUT |= 0x7f; //set lower 7 bis high to set all anodes
  cycle_bank(banks[e_bank]);
}

void led_bank_off(LED_Bank e_bank)
{
  PEOUT &= ~0x1f; //set lower 5 bits low on cathodes
  PGOUT &= ~0x7f; //set lower 7 bis low to clear all anodes
  cycle_bank(banks[e_bank]);
}

void led_all_on()
{
  int i;
  for(i=0; i<4; i++)
  {
    led_bank_on(i);
  }
}

void led_all_off()
{
  int i;
  for(i=0; i<4; i++)
  {
    led_bank_off(i);
  }
}

