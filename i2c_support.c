#include "i2c_support.h"
#include <zneo.h>
#include <string.h>
#include "shell.h"
#include "i2c.h"

void init_i2c_support()
{
	add_command(R"i2c", "", R"i2c [check|clear|stop]\n", &check_i2c);	
}

int check_i2c(char *name, int argc, char** args)
{
	if(argc > 0)
	{
		if(strcmp(R"check", args[0]) == 0)
		{
			i2c_check_device(0xa0);
			i2c_check_device(0xc0);
			return 1;
		}
		if(strcmp(R"clear", args[0]) == 0)
		{
			i2c_clear();
			return 1;
		}
		if(strcmp(R"stop", args[0]) == 0)
		{
			I2CCTL |= 0x20;
			I2CDATA = 0x00; //fill with some data to send the stop condition
		}
		
	}else
	{
		i2c_print_diag();
	}

	return 1;
}
