#include "led_commands.h"
#include "shell.h"
#include "led.h"
#include "helper.h"
#include <string.h>
#include <stdlib.h>

//The R prefix puts them in ROM
#define DISPLAY_HELP R"display [\"text\"]\n\
\t- Write \"text\" to the LED panel."

#define HEX_HELP  R"hex [num]\n\
\t- Write num to the LED panel as hex."

#define BIN_HELP R"bin [num]\n\
\t- Write num to the LED panel in binary."

void init_led_cmds()
{
	led_init();
	add_command(R"display", "", DISPLAY_HELP, &display_cmd);
	
	add_command(R"hex", "", HEX_HELP, &hex_cmd);
	
    //bin not implemented
	//add_command(R"bin", "", BIN_HELP, &hex_cmd);
    led_message(get_prompt());
}

int display_cmd(char* name, int argc, char** args)
{
    int i, len=0;
    char *cat, *cat_tail;
	if(argc > 0)
	{
        for(i=0; i<argc; i++)
        {
            len+=strlen(args[i]);
        }
        //len of all chars, plus len for spaces, plus \0
        cat = malloc(sizeof(char)*(len+argc+1));
        cat[0]='\0';
        for(i=0; i<argc; i++)
        {
            strcat(cat, args[i]);
            if(i< argc-1)
            {
                strcat(cat, " ");
            }
        }
        cat[len+argc]='\0';
		led_message(cat);
        free(cat);
	}else {
		led_message(R"");
	}
    return 0;
}

int hex_cmd(char* name, int argc, char** args)
{
	if(argc > 0)
	{
		led_message_hex(atoi(args[0]));
	}
    return 0;
}

int bin_cmd(char* name, int argc, char** args)
{
	return 0;
}
