#include "sw_macro.h"
#include "helper.h"
#include "shell.h"
#include <stdlib.h>
#include <stdio.h>

#define MAX_MACROS 3
#define SW_HELP R"switch [n] [\"text\"]\n\
\t- Pressing a button causes \"text\" to be interpred as\n\
\t  though it was sent to the UART from the terminal (like \n\
\t  a macro function)."

struct Switch_Macro{
	int sw;
	int argc;
	char* name;
	char** args;
    Command *cmd;
};
typedef struct Switch_Macro Macro;


void print_macro(Macro);

Macro macros[MAX_MACROS];

void init_macros()
{
	int i;
	for(i=0; i<MAX_MACROS; i++)
	{
		macros[i].name=NULL; //this keeps the first round from being freed
		macros[i].args=NULL;
        macros[i].cmd = NULL;
	}
}

void init_switch_cmds()
{
    init_macros();
    add_command(R"switch", "sw", SW_HELP, &switch_macro);

}

int switch_macro(char* name, int argc, char** args)
{
	int sw_num,i;
    Command *cmd;
	if(argc < 2) //one for the switch num, one for the command
	{
		//print switches?
        printf(R"Not enough arguments\n");
        printf(R"%s\n", SW_HELP);
		return 0;
	}
    cmd = findCommand(args[1]); //from shell.h
    if(cmd == NULL)
    {
        printf(R"No command %s, see help for available commands\n", args[1]);
        return 0;
    }
	sw_num = atoi(args[0]);
	if(sw_num <0 || sw_num >= MAX_MACROS)
	{
		//print error
        printf(R"Switch number must be in the range [0,2]\n");
        printf(R"%s\n", SW_HELP);
		return 0;
	}
	
	//clean up from the last macro
	if(macros[sw_num].name != NULL)
	{
		free(macros[sw_num].name);
        macros[sw_num].name=NULL;
	}
	if(macros[sw_num].args != NULL)
	{
		for(i=0; i<macros[sw_num].argc; i++)
		{
            if(macros[sw_num].args[i] != NULL)
            {
			    free(macros[sw_num].args[i]);
                macros[sw_num].args[i] = NULL;
            }
		}
		free(macros[sw_num].args);
        macros[sw_num].args=NULL;
	}
	macros[sw_num].sw = sw_num;
	macros[sw_num].argc = argc-2; //subtract the switch num & name
	macros[sw_num].name = deep_cp_str(args[1]);
	macros[sw_num].args = malloc(sizeof(char*)*(argc-2));
	for(i=2; i<argc; i++)//start on the third arg (skip sw no. and name)
	{
		macros[sw_num].args[i-2] = deep_cp_str(args[i]);
	}
    macros[sw_num].cmd = cmd;
	
	//print_macro(macros[sw_num]);
	return 0;
}

void execute_macro(int sw)
{
  if(sw >= 0 && sw < MAX_MACROS)
  {
    Macro mac = macros[sw];
    if(mac.cmd != NULL)
    {
        mac.cmd->function(mac.name, mac.argc, mac.args);
        printf(R"%s", get_prompt());
    }
  }
}

void print_macro(Macro m)
{
	int i;
	printf(R"Macro for sw %d:  %s\n", m.sw, m.name);
	for(i=0; i<m.argc; i++)
	{
		printf(R"%s", m.args[i]);
		if(i < m.argc-1)
		{
			printf(R" ");
		}else {
			printf(R"\n");
		}

	}
}
