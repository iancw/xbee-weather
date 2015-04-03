#ifndef __PORTS_H__
#define __PORTS_H__

#include <zneo.h>


//If c is between 0 and 11, interprets it as a direct index
//If c is between 'A'-'K', or 'a'-'k', interprets it as port letter
//If c is not in any of those ranges, returns null

//Prints output from the given port
void print_port(char c);
#endif