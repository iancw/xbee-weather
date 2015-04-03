#include "shell.h"
#include "keyboard.h"
#include "helper.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>p

#define MAX_CMD_LEN 100 //maximum size of a command string (including arguments)
#define NUM_CMDS 20 //number of commands

void main_loop();
int handle_cmd();

ptCmd findFunction(char *);
int defaultCommand(char*, int, char **);
int exitCommand(char*, int, char **);
int helpCommand(char*, int, char**);

int num_def_cmds=0;
Command g_cmds[NUM_CMDS];
char *prompt_text;

int defaultCommand(char * name, int argc, char **args)
{
	printf(R"Command %s not found\n", name);
	return 0;
}

int exitCommand(char* name, int argc, char** args)
{
	return 1;
}

int helpCommand(char* name, int argc, char** args)
{
	int i;
	for(i=0; i<num_def_cmds; i++)
	{
		printf(R"%s\n", g_cmds[i].help);
	}
	return 0;
}

int echoCommand(char* name, int argc, char** args)
{
	int i;
	for(i=0;i<argc;i++)
	{
		printf(R"%s", args[i]);
		if(i < argc-1)
		{
			printf(R" ");
		}
	}
	printf(R"\n");
	return 0;
}

void init_shell()
{
	int i=0;
	//No need to keep the default command in the array
	//g_cmds[i].name = "default";
	//g_cmds[i].function = &defaultCommand;
	//i++;
    //also no need for an exit function, you don't really want
    //an embedded system to exit its main loop
	//g_cmds[i].name="exit";
	//g_cmds[i].alt="quit";
	//g_cmds[i].help=R"exit\n\t- Exits from the shell";
	//g_cmds[i].function = &exitCommand;
	//i++;
	g_cmds[i].name="echo";
	g_cmds[i].alt="";
	g_cmds[i].help=R"echo [text]\n\t- echos the text to the serial port";
	g_cmds[i].function = &echoCommand;
	i++;

	g_cmds[i].name="help";
	g_cmds[i].alt="?";
	g_cmds[i].help=R"help\n\t- prints help info for all commands";
	g_cmds[i].function = &helpCommand;
	i++;
	num_def_cmds = i;
	
	prompt_text=deep_cp_str(R">");
}

void add_command(char* name, char* alt, rom char* help, ptCmd function)
{
	if(num_def_cmds < NUM_CMDS)
	{
	g_cmds[num_def_cmds].name=deep_cp_str(name);
	g_cmds[num_def_cmds].alt=deep_cp_str(alt);
	g_cmds[num_def_cmds].help = help;
	g_cmds[num_def_cmds].function = function;
	num_def_cmds++;
	}else {
		printf(R"Can't add another command, already at maximum (%d)\n", num_def_cmds);
	}
}
			   
 Command *findCommand(char *command)
{
	int i;
	for(i=0; i<num_def_cmds; i++)
	{
		if(strcmp(g_cmds[i].name, command) == 0 || strcmp(g_cmds[i].alt, command) == 0)
		{
			return &g_cmds[i];
		}
	}
	return NULL;
}

ptCmd findFunction(char *command)
{
	 Command *cmd;
	cmd = findCommand(command);
	if(cmd == NULL)
	{
		return &defaultCommand;
	}
	return cmd->function;
}

void set_prompt(char* new_prompt)
{
	free(prompt_text);
	prompt_text=deep_cp_str(new_prompt);
}

char const* get_prompt()
{
    return prompt_text;
}

/*
 Prints the shell prompt
 */
void prompt()
{
	printf(R"%s", prompt_text);
}

/*
 Returns 1 if the shell should exit, otherwise 0.
 
 Parses the command string into tokens, looks up
 the corresponding function for the given command,
 executes that command, and returns 0 unless the 
 command terminates the shell.
 */
int handle_cmd(const char *cmd)
{
	char* dup, *token;
	char* delims=" ";
    int len;
    static char* name=NULL;
	static char *args[10];
	static int argc=0, i;
	ptCmd cmdFunc;
    //clean up from last time
    if(name != NULL)
    {
        free(name);
		name=NULL;
    }
    for(i=0; i<argc; i++)
    {
		free(args[i]);
    }
	argc=0;
	if(cmd == NULL || strlen(cmd) < 1)
	{
		return 0;
	}
	dup = deep_cp_str(cmd);
	token = strtok(dup, delims);
    len = strlen(token);
    name = malloc(sizeof(char)*(len+1));
    strcpy(name, token);
    name[len]='\0';
	//lookup function corresponding to name
	token = strtok(NULL, delims);
	while(token != NULL)
	{
        len = strlen(token);
		args[argc]=malloc(sizeof(char)*(len+1));
        strcpy(args[argc], token);
        args[argc][len]='\0';
        
        argc++;
		token = strtok(NULL, delims);
	}
	cmdFunc = findFunction(name);
    free(dup);
	return cmdFunc(name, argc, args);
}

/*
 Called from inside the main loop, this checks if key_typed
 and if so, uses key_get and processes the result.  It buffers
 key types and tries to execute commands when \r or \n is encountered.

 Call this from your own main loop, or indirectly using main_loop
*/
char check_key()
{
	static char buff[MAX_CMD_LEN];
    static int tail;
    char c;
    if(key_typed())
	{
        if(tail >= MAX_CMD_LEN-1)
        {
            printf(R"\nCommand length is larger than limit (%d), resetting.\n", MAX_CMD_LEN);
            tail=0;
            prompt();
        }else
        {
			c = key_get();
			if(c == '\r' || c == '\n')
			{
				key_print(R"\n");
				buff[tail] = '\0';
				handle_cmd(buff);
				prompt();
				tail=0;
			}else if(c == '\t')
			{
				//do tab completion
			}else if(c == '\b')
            {
                //ignore backspaces
                tail--;
                key_put(c);//need to echo backspace back to terminal
            }else if(tail < MAX_CMD_LEN-1){
				key_put(c);
				buff[tail++] = c;
			}
            return c;
        }
	}
    return '\0';
}

/*
 Monitors input from keyboard.h one character at a time
 and builds up commands in a buffer.  When enter is pressed,
 defers to the command handling routine.
 */
void main_loop()
{
	prompt();
	while(!exit)
	{
		check_key();
	}
}
