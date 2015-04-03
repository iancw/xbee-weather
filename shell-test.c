
#include "shell.h"
#include "settings.h"

int main(void)
{
 init_shell();
 init_set_cmd();
 main_loop();
 return 1;
}
