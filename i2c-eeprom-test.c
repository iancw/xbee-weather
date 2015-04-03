#include <zneo.h>
#include <sio.h>
#include <stdio.h>
#include "shell.h"
#include "clock.h"
#include "peripheral_cmds.h"
#include "settings.h"
#include "i2c.h"
#include "MPL115A.h"

int check_i2c(char *name, int argc, char** args)
{
	i2c_check_device(0xa0);
	i2c_check_device(0xc0);
	return 1;
}

int clear_i2c(char *name, int argc, char** args)
{
	i2c_clear();
	return 1;
}

int diag_i2c(char* name, int argc, char** args)
{
	i2c_print_diag();
	return 0;
}

int poll_pres(char* name, int argc, char** args)
{
	float pres;
	pres = MPL115A_read_pressure();
	printf(R"Read %f kPa\n", pres);
	return 1;
}

int stop_i2c(char* name, int argc, char** args)
{
	I2CCTL |= 0x20;
	I2CDATA = 0x00; //fill with some data to send the stop condition
	return 0;
}
void main(void)
{

	set_clock_5_5();
	
    init_uart(_UART0, get_clock(), _DEFBAUD);
	PAAF |= 0xc0;
	i2c_init(400000);	//set baud to 200 kHz
	i2c_print_diag();
	init_shell();
	init_set_cmd();
	init_peripheral_cmds();
	init_MPL115A();
	
	add_command(R"checki2c", "", "Checks both i2c devices", &check_i2c);
	add_command(R"cleari2c", "", "Clears i2c", &clear_i2c);
	add_command(R"i2creg", "", "Prints status of i2c registers", &diag_i2c);
	add_command(R"i2cstop", "", "Sends stop bit", &stop_i2c);
	add_command(R"mpl115a", "", "Polls pressure from MPL115A", &poll_pres);
	
	i2c_print_diag();
		
	i2c_check_device(0xa0);
	i2c_check_device(0xc0);
 
	while(1)
	{
		check_key();
	}
	
}
