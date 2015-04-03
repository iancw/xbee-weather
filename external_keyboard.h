#ifndef __EXTERNAL_KEYBOARD_H__
#define __EXTERNAL_KEYBOARD_H__

/*
 * Adds the scan, type, and input commands to the shell
 */
void init_external_keyboard();

/*
 * Interrupts the current activity (based on SW inputs) 
 * and reverts i/o to normal serial sources.
 */
void interrupt_external_keyboard();

/*
 * Will display scancodes from the PS/2 keyboard as the 
 * key are pressed/released. Will continue to do this 
 * until any of the buttons (SW1,2,3) are pressed.
 */
int scan_command(char* name, int argc, char** args);

/*
 * Will display characters from the PS/2 keyboard as the 
 * key are pressed/released. Will continue to do this until 
 * the escape key pressed, or until any of the buttons 
 * (SW1,2,3) are pressed (both must work).
 */
int type_command(char* name, int argc, char** args);

/*
 * Will get and display characters from the PS/2 keyboard
 * AND interpret them as if they came from the console serial 
 * port. Will input one command (up to the newline) and then 
 * return to normal CLI input from the serial port.
 */
int input_command(char* name, int argc, char** args);

#endif