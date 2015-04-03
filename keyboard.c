#include "keyboard.h"
#include <stdio.h>
#include <sio.h>

char key_get()
{
   return getch();
	
}

void key_put(char c)
{
  putch(c);
}

void key_print(char* str)
{
  printf(R"%s", str);
}

int key_typed()
{
	return kbhit();
}
