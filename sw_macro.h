#ifndef __SW_MACRO_H__
#define __SW_MACRO_H__

void init_switch_cmds();

int switch_macro(char* name, int argc, char** args);

/*
 Executes the given macro, corresponding to a programmed
 switch.  sw should be 0-2, for compatibility with the 
 Switch enum defined in buttons.h.

 If no switch macro has been defined for the given switch,
 the function will do nothing.
*/
void execute_macro(int sw);

#endif