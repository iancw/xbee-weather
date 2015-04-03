#ifndef __LED_CMDS_H__
#define __LED_CMDS_H__

void init_led_cmds();

int display_cmd(char* name, int argc, char** args);
int hex_cmd(char* name, int argc, char** args);
int bin_cmd(char* name, int argc, char** args);

#endif