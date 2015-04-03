#include "keyboard.h"
#include <stdio.h>

char key_get_auto();

char *input="echo text\r\ndisplay text\r\nhex 10\r\n\
switch 1 echo hi\r\nport a\r\ntimer 0\r\ninfo\r\n?\r\n\
uart speed 9600 parity even bits 7\r\n\
set prompt C:>\r\nset scroll fast\r\nset scroll\r\n\
set scroll 0\r\nset scroll 11\r\nset\r\nset prompt\r\n\
help\r\nsw 1 echo hi\r\nsw 3\r\nsw\r\nsw 3 info\r\nsw 2 \
name argtwo argthree argfour argfive.\r\nexit\r\n";
int char_idx=0;
int done=0;

char key_get()
{
	return key_get_auto();
	
}

char key_get_auto()
{
	char next=input[char_idx++];
	if(input[char_idx] == '\0')
	{
		done=1;
		char_idx=0;
	}
	return next;
}

void key_put(char c)
{
	putchar(c);
}

void key_print(char* str)
{
	printf(R"%s", str);
}

int key_typed()
{
	return !done;
}
