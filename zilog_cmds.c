#include "zilog_cmds.h"
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <zneo.h>
#include <string.h>
#include "timer.h"
#include "ports.h"
#include "uart.h"

#define UART0_HELP R"uart0 [speed [baud]] [parity [even|odd|none]] [bits [7|8]]\n\
\t- Sets the uart 0 settings immediately."

#define UART1_HELP R"uart0 [speed [baud]] [parity [even|odd|none]] [bits [7|8]]\n\
\t- Sets the uart 0 settings immediately."

#define TIMER_HELP R"timer [0-2]\n\
\t- Displays how timer n is configured and if enabled."

#define INFO_HELP R"info\n\t- Displays part number, oscillator setting,\
 clock speed, \n\t  serial port baud rate, CLI compile date & time."

 #define PORT_HELP R"port [A-K]\n\t- Displays how the IO port [A-K] is \
 configured (all settings, each bit)."

 #define MEM_HELP R"mem [address, length]\n\t- Dump memory in hex starting\
 at address for length bytes."

int uart_helper(unsigned char volatile near* uart, int argc, char** args);

//Adds commands to shell.h
void init_zilog_cmds()
{
	add_command(R"timer", "", TIMER_HELP, &timer_cmd);
    add_command(R"uart0", "", UART0_HELP, &uart0_cmd);
    add_command(R"uart1", "", UART1_HELP, &uart1_cmd);
    add_command(R"info", "", INFO_HELP, &info_cmd);
    add_command(R"port", "", PORT_HELP, &port_cmd);
    add_command(R"mem", "", MEM_HELP, &mem_cmd);
}

//Displays how the timer n is configured and if enabled
//timer [0-3]
int timer_cmd(char *name, int argc, char** args)
{
    int timer, enab, presc, mode, int_conf, casc=0, pwm=0, capt=0, pol=0;
    unsigned char ctl0, ctl1;
    unsigned short reload, value;
    if(argc != 1)
    {
        DI();   //All the DI/EIs around prinf statements are weird.
        //The program was getting really slow and crashing around
        //printf statements in timer.h for some reason
        //it didn't seem to do that with any other methods.
        //it was even crashing/hanging while printing relatively simple 
        //things like printing error help messages
        //Disabling interrupts around printf seems to have fixed
        //the problem
        printf(R"%s\n", TIMER_HELP);
        EI();
        return 0;
    }
    timer = atoi(args[0]);
    if(timer < 0 || timer > 2)
    {
        DI();
        printf(R"TIMER must be between [0,2]\n");
      printf(R"%s\n", TIMER_HELP);
      EI();
      return 0;
    }
    
    ctl0 = timer_get_CTL0(timer);
    ctl1 = timer_get_CTL1(timer);
    enab = timer_is_enabled(timer);
    presc = timer_get_prescale(timer);
    mode = timer_get_mode(timer);
    int_conf = timer_get_interrupt_config(timer);
    casc = timer_is_cascade(timer);
    pwm = timer_get_PWM_delay(timer);
    capt = timer_was_capture(timer);
    pol = timer_get_polarity(timer);
    reload = timer_get_reload(timer);
    value = timer_get_value(timer);
    printf(R"TIMER%d: \n", timer);
    printf(R"\tenabled=%d\n", enab);
    printf(R"\tprescale=%d\n", presc);
    printf(R"\tmode=%d", mode);
    if(mode == 0)
    {
        printf(R" (one-shot)\n");
    }else if(mode == 1)
    {
        printf(R" (continuous)\n");
    }else
    {
        printf(R"\n");
    }
    printf(R"\tinterrupt config=%d", int_conf);
    if(int_conf < 2)
    {
        printf(R" (interrupt on reload)\n");
    }else if(int_conf == 2)
    {
        printf(R" (interrupts disabled)\n");
    }else if(int_conf == 3)
    {
        printf(R" (interrupt on reload)\n");
    }
    printf(R"\tcascade=%d\n", casc);
    printf(R"\tPWM delay=%d\n", pwm);
    printf(R"\tpolarity bit=%d\n", pol);
    printf(R"\twas capture=%d\n", capt);
    printf(R"\treload=%d\n", reload);
    printf(R"\tvalue=%d\n", value);
	return 0;
}

//uart0 [speed [baud]] [parity [even|odd|none]] [bits [7|8]]
//sets the uart0 settings immediately.
int uart0_cmd(char* name, int argc, char** args)
{
    return uart_helper((unsigned char volatile near*)0xE200, argc, args);
}

int uart1_cmd(char* name, int argc, char** args)
{
    return uart_helper((unsigned char volatile near*)0xE210, argc, args);
}

int uart_helper(unsigned char volatile near* uart, int argc, char** args)
{
    int i, cmd_type=-1, i_val;
    unsigned long l_val;
    unsigned char volatile near* ctl0, *mdstat, *stat0, *ctl1;

    ctl0 = uart+2;
    ctl1 = uart + 3;
    mdstat = uart+4;
    stat0 = uart+1;
    if(argc == 0)
    {
        //print uart register status
		if(*ctl0 & 0x80)
        {
            printf(R"\tTransmit is enabled\n");
        }else
        {
            printf(R"\tTransmit is disabled\n");
        }
        if(*ctl0 & 0x40)
            printf(R"\tReceive is enabled\n");
        if(*ctl0 & 0x20)
            printf(R"\tCTS is enabled\n");
        if(*ctl0 & 0x10)
        {
            if(*ctl0 & 0x08)
                printf(R"\tUsing odd parity\n");
            else
                printf(R"\tUsing even parity\n");
        }else
        {
            printf(R"\tParity is disabled\n");
        }
        if(*ctl0 & 0x04)
            printf(R"\tSend break is on, transmit is paused\n");
        if(*ctl0 & 0x02)
            printf(R"\tSending two stop bits\n");
        else
            printf(R"\tSending one stop bit\n");
        if(*ctl0 & 0x01)
            printf(R"\tLoop back enabled\n");

        printf(R"\tUxCTL1: 0x%02x\n", *ctl1);
        printf(R"\tBaud rate is %lu\n", uart0_get_baud());
        printf(R"\tUxADDR (address) : 0x%02x\n", *ctl0);
        printf(R"\tUxMDSTAT: 0x%02x\n", *mdstat);

		if((*stat0 & 0x80) == 0x80)
			printf(R"\tReceive data available\n");
		if((*stat0 & 0x40) == 0x40)
			printf(R"\tParity error\n");
		if(*stat0 & 0x20)
			printf(R"\tOverrun error\n");
		if(*stat0 & 0x10)
			printf(R"\tFraming error\n");
		if(*stat0 & 0x08)
			printf(R"\tBreak has occurred\n");
		if(*stat0 & 0x04)
			printf(R"\tTransmitter data register is empty\n");
		if(*stat0 & 0x02)
			printf(R"\tTransmitter empty\n");
		if(*stat0 & 0x01)
        {
            printf(R"\tBusy\n");
         }else
         {
			printf(R"\tClear to send\n");
         }
    }
    for(i=0; i<argc; i++)
    {
        switch(cmd_type)
        {
            case 0: //speed [baud]
                l_val = atol(args[i]); //baud rate
                if(l_val > 0)
                {
                    uart_set_baud(l_val);
                    
                }else
                {
                    printf(R"Baud value must be a non-zero integer representing rate in Hz, %s is invalid.\n", args[i]);
                    return 0;
                }
                cmd_type=-1;
            break;
            case 1: //parity [even|odd|none]
                if(strcmp(R"even", args[i]) == 0)
                {
                    uart_set_parity(EVEN);
                }else if(strcmp(R"odd", args[i]) == 0)
                {
                    uart_set_parity(ODD);
                }else if(strcmp(R"none", args[i]) == 0)
                {
                    uart_set_parity(NONE);
                }else
                {
                    printf(R"Unrecognized argument to parity %s\n", args[i]);
                    return 0;
                }
                cmd_type=-1;
            break;
            case 2: //bits [7|8]
                i_val = atoi(args[i]);
                if(i_val == 7 || i_val == 8)
                {
                    uart_set_bits(i_val);
                }else
                {
                    printf(R"Only 7 or 8 bits are valid, %s is an invalid number of bits\n", args[i]);
                    return 0;
                }
                cmd_type=-1;
            break;
            default:
                if(strcmp(args[i], "speed") == 0)
                {
                    cmd_type=0;
                }else if(strcmp(args[i], "parity") == 0)
                {
                    cmd_type=1;
                }else if(strcmp(args[i], "bits") == 0)
                {
                    cmd_type=2;
                }else
                {
                    cmd_type=-1;
                    printf(R"Ignoring unrecognized argument %s\n", args[i]);
                    return 0;
                }
        }
    }
    if(cmd_type >= 0)
    {
        printf(R"Need additional arguments to %s\n", args[i-1]);
    }

	return 0;
}

//displays part number, oscillator setting, clock spd,
//serial port baud rate, CLI compile date & time
int info_cmd(char* name, int argc, char** args)
{
    unsigned char volatile near* pInfo = (unsigned char volatile near*)0x0040;
    int i, ctl=1;
    unsigned char part_no[20];
    unsigned long serial;
    FCTL = 0x80; //enable reading the flash info section
    for(i=0; i<20; i++)
    {
        part_no[i] = *(pInfo+i);   
    }
    FCTL = 0x00;

    printf(R"Part no. %s\n", part_no);

    ctl = ((OSCCTL & 0x80)>>7);
    printf(R"Internal oscillator enabled: %d\n", ctl);
     ctl = ((OSCCTL & 0x40)>>6);
    printf(R"External crystal oscillator enabled: %d\n", ctl);
    ctl = ((OSCCTL & 0x20)>>5);
    printf(R"Watchdog oscillator enabled: %d\n", ctl);
    ctl = (OSCCTL & 0x03);
    if(ctl == 0)
        printf(R"Primary oscillator: internal 5.6 MHz\n");
    if(ctl== 1)
        printf(R"Primary oscillator: external 18.432 MHz\n");
    if(ctl == 3)
        printf(R"Primary oscillator: watchdog\n");

    printf(R"Clock speed: %lu Hz\n", get_clock());

    serial = uart0_get_baud();
    printf(R"Serial port baud rate: %lu\n", serial);
    serial = uart1_get_baud();
    printf(R"XBee baud rate: %lu\n", serial);
    printf(R"Compile date & time: %s %s\n", __DATE__, __TIME__);
	return 0;
}

//port [A-K]
//displays how the IO port [A-K] is configured (all settings,
//each bit).
int port_cmd(char* name, int argc, char** args)
{
	if(argc <= 0)
    {
        printf(R"Must specify a port\n");
        printf(R"%s\n", PORT_HELP);
        return 0;
    }
    
    print_port(args[0][0]);//use the first char as the port
    return 0;

}


//mem [address, length]
//dump memory in hex start at address for length bytes
int mem_cmd(char* name, int argc, char** args)
{
    int len, line=16, i;
    unsigned long add;
    _Erom const char *ptr, *end;
//	_Erom const char* e_ptr;
    if(argc != 2)
    {
        printf(R"%s\n", MEM_HELP);
        return 0;
    }
    sscanf(args[0], R"%08lx", &add);
    //add = atoi(args[0]);
    len = atoi(args[1]);
    ptr = (_Erom const char*)add;
    end = ptr+len;
    while(ptr < end)
    {
        printf(R"%8lX\t: ", ptr);
        for(i=0; i<line; i++)
        {
             printf(R"%02X ", *ptr++);
        }
        printf(R"\n");
    }
	return 0;
}

