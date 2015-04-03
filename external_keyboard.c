#include "external_keyboard.h"
#include "shell.h"
#include "stdio.h"
#include "led.h"
#include "scan-codes.h"
#include <zneo.h>

#define SCAN_HELP R"scan\n\t- Displays key codes from PS/2 keyboard until a switch is pressed."
#define TYPE_HELP R"type\n\t- Displays characters from PS/2 keyboard until escape key or a switch is pressed."
#define INPUT_HELP R"input\n\t- Accepts one command from PS/2 keyboard, then reverts to serial port."
#define MAX_CODES 100

/*
 * ISR for falling edges from the PS/2 keyboard
*/
void read_pin1();

/*
 * Pulls out start, stop, and parity bits, returns
 * the 8 bits in the message
 */
unsigned char process_bits(short bits);
void increment_tail();
void increment_head();


short scan_codes[MAX_CODES];
int interrupted=0;

// static short scan_codes[10];
int i=0, tail_code=0, head_code=0;
//Called on rising or falling edge for port D

void interrupt read_pin1()
{
	// clock = port & 0x01;
   // all_data[tail_code][i++] = PDIN&0x02;
	scan_codes[tail_code] |= (((PDIN&0x02)>>1) << i);
    i++;
    if(i > 10)
    {
        i=0;
        increment_tail();

        //This means we've wrapped around.  Lets adjust
        //head_code to keep up with tail
        if(tail_code == head_code)
        {
            increment_head();
        }
        scan_codes[tail_code]=0;
    }
    
}

void increment_tail()
{
    tail_code++;
    
    if(tail_code == MAX_CODES)
    {
        tail_code = 0;
    }
}
void increment_head()
{
    head_code++;
    
    if(head_code == MAX_CODES)
    {
        head_code = 0;
    }
}


unsigned char process_bits(short bits)
{
    return (unsigned char)(bits >> 1);
}

/*
 * Bits is a stream of 11 bits from the PS/2 serial
 * line.  Bit 10 is the first bit to arrive (start bit), bit 1
 * is parity, and bit 0 is the stop bit.
 * 
 * Start should be 0, stop should be 1, and parity should be even.
 *
 */
unsigned char process_bits2(short bits)
{
    unsigned char result=0;
    int i=0, parity=0;
    if(bits & 0x0001) // stop bit (bit 10)
    {
        printf(R"Stop bit is 1\n");
    }
    if(bits & 0x0002) //parity bit is 1
    {
        //parity=1;
        printf(R"Parity bit is 1\n");
    }
    if(bits & 0x0400)
    {
        printf(R"Start bit is 1\n");
    }
    for(i=0; i<11; i++)
    {
        if(bits & (0x0001 << i))
        {
            parity++;
        }else
        {
            parity--;
        }
    }
    
    return (unsigned char)(bits >> 1);
}

/*
 * Adds the scan, type, and input commands to the shell.
 * Also activates pin 0 and 1 of Port D as inputs, and
 * configures an IRQ on pin 0.
 *
 */	
void init_external_keyboard()
{
	PDDD |= 0x03; //Set data direction to in
    PDPUE |= 0x03; //Turn on pullups
    PAIMUX = 0x01; //Select Port D (versus A) as interrupt source
    PAIEDGE &= ~0x01; //Clear bit 1 so Port D triggers only on falling edges
    IRQ1ENL |= 0x01; //set low bit to 1
    IRQ1ENH &= ~0x01; //set high bit to 0, for low priority interrupt

    SET_VECTOR(P0AD, read_pin1);
	EI();

    add_command(R"scan", "", SCAN_HELP, &scan_command);
    add_command(R"type", "", TYPE_HELP, &type_command);
    add_command(R"input", "", INPUT_HELP, &input_command);
}

void interrupt_external_keyboard()
{
    interrupted=1;
}

int scan_command(char* name, int argc, char** args)
{
    char c;
    char msg[10];
    static int char_count=0; //counts chars printed to console for wrapping
    interrupted=0;
    while(!interrupted)
    {
        //process all new scan codes...
        while(tail_code != head_code)
        {
            c = process_bits(scan_codes[head_code]);
            increment_head();
            sprintf(msg, "0x%02x ", c);
            led_message(msg);
            printf(R"0x%02x ", c);
            char_count += 5;
            if(char_count > 40)
            {
                printf(R"\n");
                char_count=0;
            }
        }
    }
	return 0;
}

/*
 * Will display characters from the PS/2 keyboard as the 
 * key are pressed/released. Will continue to do this until 
 * the escape key pressed, or until any of the buttons 
 * (SW1,2,3) are pressed (both must work).
 */
int type_command(char* name, int argc, char** args)
{
    char scan, c_val;
    NonPrinting np_val;
    char msg[10];
	int was_released=0;
	int was_special=0, is_shift=0;
    static int char_count=0; //counts chars printed to console for wrapping
    interrupted=0;
    c_val = '\0';
    while(!interrupted)
    {
        //process all new scan codes...
		//when tail_code = head_code, there are no new
		//codes.  this is a circular buffer.
        while(tail_code != head_code)
        {
            c_val='\0';
			//grab next scan code
            scan = process_bits(scan_codes[head_code]);
			increment_head();
			
			//process scan code
			if(is_extended_code(scan))
			{
				was_special=1;
			}else if(is_released_code(scan))
			{
				was_released=1;
			}else {
				if(is_non_printing(scan))
				{
					np_val = np_to_val(scan, was_special);
                    switch(np_val)
                    {
                        case L_SHFT:
                        case R_SHFT:
                            if(was_released)
                            {
                                is_shift=0;
                            }else
                            {
                                is_shift=1;
                            }
                        break;
                        case ESC: interrupted=1;
                        break;
                    }
					was_special=0;
                    c_val='\0';
				}else
                {
                    c_val = scan_to_char(scan, is_shift);
                }
				if(was_released)
				{
                    //do nothing, but clear the
					//clear the was_released flag
					was_released=0;
				}else if (c_val != '\0'){
					printf(R"%c", c_val);
					char_count ++;
					if(char_count > 80)
					{
						printf(R"\n");
						char_count=0;
					}
				}
			}          
        }
    }
	return 0;
}

int input_command(char* name, int argc, char** args)
{
    return 0;
}
