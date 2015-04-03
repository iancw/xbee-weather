#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char *deep_cp_str(const char* str)
{
	int len,i;
	char *dup;
	len = strlen(str);
    dup = malloc(sizeof(char)*(len+1));
    //i've got weird behavior form strcpy here...
    strcpy(dup, str);
    dup[len]='\0';
	return dup;
}
