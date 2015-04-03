#ifndef __SHELL_H__
#define __SHELL_H__

/*
 The type for a function pointer that all shell
 commands must implement.
 */
typedef int(*ptCmd)(char*, int, char **);

struct Command{
	char *name;
	char *alt;
	_Rom char  *help;
	ptCmd function;
};
typedef struct Command Command;

//Must be called before using any other shell
//functions
void init_shell();

// This initiates an infinte loop which process
// keystrokes and shell commands
void main_loop();

//This is the functional equivalent of running
//main_loop, but it allows shell to be used from
//inside something else's loop
char check_key();

//Adds a command to the shell by supplying a function
//pointer
void add_command(char* name, char* alt, char* help, ptCmd function);

//sets the shell prompt
void set_prompt(char* prompt);

//Returns NULL if command is not a registered command or alias
Command *findCommand(char *command);

//returns the current shell prompt
char const *get_prompt();

//Prints a prompt to the serial port, indicating input is expected
void prompt();
#endif