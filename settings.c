#include "settings.h"
#include "shell.h"
#include "led.h"
#include "clock.h"
#include "uart.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SET_HELP R"set [prompt \"prompt\"]\
 [scroll [fast|slow]] [scroll [0,1..9,99]] [scroll speed [secs]]\n\
\t- with no parameters lists all current settings\n\
\t- [scroll [fast|slow]] controls speed of LED array\n\
\t- [scroll [0,1..9,99] controls number of times the message repeats, 99 disables any repeat\n\
\t- [scroll speed [secs]] list current speed if secs is omitted or set scroll speed in seconds\n\
\t- [refresh [secs]] list current LED refresh rate, sets the rate if secs is supplied\n\
\t- [clock [internal|external]] sets the clock to be either the internal or the external clock."

#define SLOW_SPEED .09
#define FAST_SPEED .02

unsigned char speed=0; //0 means show, 1 means fast
unsigned char repeats=99;

/*
 Adds the set command to shell.h
*/
void init_set_cmd()
{
	add_command(R"set", "", SET_HELP, &setCommand);
}

int setCommand(char* name, int argc, char** args)
{
    float back_scroll, back_ref; //backups for changing the clock speed
    unsigned long back_baud;

	if(argc < 1)
	{
		printf(R"Current settings:\n\tprompt: %s\n\tscroll speed: %0.2f\n\
\tscroll repeats: %d\n", get_prompt(), led_get_scroll(), repeats);
		return 0;
	}
	if(strcmp(args[0], "prompt")==0)
	{
		if(argc == 2)
		{
			set_prompt(args[1]);
		}else{
			Command *cmd = findCommand(name);
			printf(R"%s\n", cmd->help);
			return 0;
		}
	}else if(strcmp(args[0], "scroll") == 0)
    {//set scroll [fast|slow] [0..9,99] [speed .001]
        if(argc > 1)
        {
            if(strcmp(args[1], "fast")==0)
            {
               led_set_scroll(FAST_SPEED);
            }else if(strcmp(args[1], "slow")==0)
            {
                led_set_scroll(SLOW_SPEED);
            }else if(strcmp(args[1], "speed")==0)
            {
                if(argc > 2)
                {
                    led_set_scroll(atof(args[2]));
                }else
                {
                    printf(R"Current scroll speed: %0.4f columns per second\n", led_get_scroll());
                }
            }else
            {
                //parse an int from args[1]
                //repeats = atoi(args[1]);
                printf(R"(repeat control currently unsupported, messages will always repeat)\n");
            }
        }
    }else if(strcmp(args[0], "refresh") == 0)
    {
        if(argc > 1)
        {
            led_set_refresh(atof(args[1]));
        }else
        {
            printf(R"Current refresh rate: %0.6f sec\n", led_get_refresh());
        }
    }else if(strcmp(args[0], "clock") == 0)
    {
        if(argc > 1)
        {
            if(strcmp(args[1], "internal") == 0)
            {
                back_scroll = led_get_scroll();
                back_ref = led_get_refresh();
                back_baud = uart0_get_baud();
                uart_enable(0);
                set_clock_5_5();

                uart_set_baud(back_baud);
                uart_enable(1);
                led_set_scroll(back_scroll);
                led_set_refresh(back_ref);
            }else if(strcmp(args[1], "external") == 0)
            {
                back_scroll = led_get_scroll();
                back_ref = led_get_refresh();
                back_baud = uart0_get_baud();
                uart_enable(0);
                set_clock_18_432();
                uart_set_baud(back_baud);
                uart_enable(1);
                led_set_scroll(back_scroll);
                led_set_refresh(back_ref);
            }else
            {
                printf(R"%s\n", SET_HELP);
            }
        }else
        {
            printf(R"Current clock: %lu Hz\n", get_clock());
        }

    }
	return 0;
}
