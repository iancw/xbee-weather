#ifndef __ZILOG_CMDS_H__
#define __ZILOG_CMDS_H__

//Adds all commands to shell.h
void init_zilog_cmds();

//Displays how the timer n is configured and if enabled
//timer [0-3]
int timer_cmd(char *name, int argc, char** args);

//uart0 [speed [baud]] [parity [even|odd|none]] [bits [7|8]]
//sets the uart0 settings immediately.
int uart0_cmd(char* name, int argc, char** args);

int uart1_cmd(char* name, int argc, char** args);

//displays part number, oscillator setting, clock spd,
//serial port baud rate, CLI compile date & time
int info_cmd(char* name, int argc, char** args);

//port [A-K]
//displays how the IO port [A-K] is configured (all settings,
//each bit).
int port_cmd(char* name, int argc, char** args);

//mem [address, length]
//dump memory in hex start at address for length bytes
int mem_cmd(char* name, int argc, char** args);
		   
#endif 