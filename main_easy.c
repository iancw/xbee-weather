/* 
 * 
 */

#include <zneo.h>
#include <sio.h>
#include <stdio.h>
#include "led.h"
#include "shell.h"
#include "settings.h"
#include "led_commands.h"
#include "sw_macro.h"
#include "zilog_cmds.h"
#include "buttons.h"
#include "timer.h"

void interrupt read_buttons()
{
    if(button_was_released(SW1))
    {
        execute_macro(0);
    }
    if(button_was_released(SW2))
    { 
        execute_macro(1);
    }
    if(button_was_released(SW3))
    {
        execute_macro(2);
    }
}

void main(void) { 
    init_uart(_UART0, get_clock(), _DEFBAUD); 
    init_shell();
    init_led_cmds();
    init_set_cmd();
    init_zilog_cmds();
    init_switch_cmds();
    timer_start_cont(T0, .005);
    timer_enable_irq_low(T0);
    
    SET_VECTOR(TIMER0, read_buttons);
    EI();
    main_loop();
}


