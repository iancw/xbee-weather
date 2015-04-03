#include <stdio.h>
#include "shell.h"
#include "settings.h"
#include "sw_macro.h"

int main(void)
{
	printf(R"init_shell()\n");
 init_shell();
	printf(R"init_macros()\n");
 init_macros();
	printf(R"init_set_cmd()\n");
 init_set_cmd();
 printf(R"Entering main loop...\n");
	add_command(R"sw", "", "SW -- ...", &switchCommand);
 main_loop();
 return 1;
}
