#include <zneo.h>
#include <sio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clock.h"
#include "xbee_api.h"
#include "PDV-P8001.h"
#include "TMP36.h"
#include "MPL115A.h"
#include "led.h"
#include "weather_log.h"
#include "shell.h"
#include "settings.h"
#include "i2c.h"
#include "timer.h"
#include "buttons.h"
#include "HIH_4000.h"
#include "i2c_support.h"
#include "zilog_cmds.h"
#include "debug_flag.h"
#include "weather_stats.h"
#include "weather_help.h"
#include "ds1722.h"
#include "spi.h"
#include "HS1101.h"

#define MAX_SAMPLES 10
//Input measured on the board
#define PDV_VIN 3.293
#define HIH_4000_R2 21.78
#define HIH_4000_R1 9.82 
#define HIH_4000_UP_SCALE 1.45//vin = (r1 + R2) * Vout / R2

#define DISP_TEMP 0
#define DISP_PRES 1
#define DISP_LUX 2
#define DISP_HUM 3
#define DISP_RSS 4
#define DISP_TIMESTAMP 5
#define NUM_STATES 6

#define STATION_0 0x1234
#define STATION_1 0x1111
#define STATION_2 0x2222

#define LUX_SAMPLE 0
#define TEMP_SAMPLE 1
#define HUM_SAMPLE 2
//Holds time in second since starting up
long g_timestamp=0;
unsigned char display_state=0, needs_local_sample=1, led_needs_update=0, last_rssi;
weath_meas msmt;
char *names[3] = {R"Light", R"Temp", R"Humid"};
unsigned short cur_station = STATION_0;

int cmd_debug(char* name, int argc, char** args)
{
	if(argc == 1)
	{
		if(strcmp(R"on", args[0]) == 0)
		{
			set_debug(1);
			return 1;
		}
		if(strcmp(R"off", args[0]) == 0)
		{
			set_debug(0);
			return 1;
		}
	}
	toggle_debug();
	return 1;
}
int query_weather(char* name, int argc, char** args)
{
	int len, start;
    unsigned long add;
	_Erom unsigned short *ptr;
	weath_meas record;
	if(argc == 0)
	{
		w_print_stats();
		return 1;
	}
	if(argc == 1)
	{
		if(strcmp(R"last", args[0]) == 0)
		{
			get_most_recent(&record);
			print_measurement(&record, 0);
			return 1;
		}
		if(strcmp(R"dump", args[0]) == 0)
		{
			dump_all_logs();
			return 1;
		}
	}
	if(argc == 2)
	{
	    sscanf(args[0], R"%lx", &add);
		len = atoi(args[1]);
	    ptr = (_Erom unsigned short*)add;
		dump_from_address(ptr, len);
		return 1;
	}
	if(argc == 3)
	{
		if(strcmp(R"avg", args[0]) == 0)
		{
			start = atoi(args[1]);
			len = atoi(args[2]);
			w_get_average(&record, start, len);
			printf(R"Averages between %d seconds and %d seconds (%d samples):\n", start, start+len, record.timestamp);
			print_measurement(&record, 0);
			
		}else if(strcmp(R"min", args[0]) == 0)
		{
			
		}else if(strcmp(R"max", args[1]) == 0)
		{
			
		}
	}
	if(argc == 4)
	{

	}

	return 0;
}

int cmd_plot(char* name, int argc, char** args)
{
	int start, len;
	if(argc == 3)
	{
		start = atoi(args[1]);
		len = atoi(args[2]);
		if(strcmp(R"temp", args[0]) == 0){
			w_histo_temp(STATION_0, start, len);
			w_histo_temp(STATION_1, start, len);
			w_histo_temp(STATION_2, start, len);
		}
		if(strcmp(R"humid", args[0]) == 0){
			w_histo_humid(STATION_0, start, len);
			w_histo_humid(STATION_1, start, len);
			w_histo_humid(STATION_2, start, len);
		}
	if(strcmp(R"lux", args[0]) == 0){
			w_histo_lux(STATION_0, start, len);
			w_histo_lux(STATION_1, start, len);
			w_histo_lux(STATION_2, start, len);						
		}
		if(strcmp(R"pres", args[0]) == 0){
			w_histo_pres(STATION_0, start, len);
			w_histo_pres(STATION_1, start, len);
			w_histo_pres(STATION_2, start, len);			
		}
	}
	return 0;
}

void update_led(weath_meas *m_pt)
{
	char msg[20];
	switch(display_state)
	{
		case DISP_TEMP: 	
			sprintf(msg, R"%.01f F", m_pt->temp_f);
		break;
		case DISP_HUM:
			sprintf(msg, R"%.01f %%", m_pt->humid);
		break;
		case DISP_LUX:
			sprintf(msg, R"%.01f K", m_pt->lux);
		break;
		case DISP_PRES:
			sprintf(msg, R"%.01f in", m_pt->pressure);
		break;
		case DISP_RSS:
			sprintf(msg, R"-%u dBm", last_rssi);
		break;
		case DISP_TIMESTAMP:
			sprintf(msg, R"%d", m_pt->timestamp);
		break;
	}
	led_update(msg);
}

void interrupt button_isr()
{
	static unsigned char call_count=0;
    if(button_was_released(SW1))
    {
		led_aux_set(0x01);
		cur_station = STATION_0;
		led_needs_update=1;
    }

    if(button_was_released(SW2))
    { 
		led_aux_set(0x02);
		cur_station = STATION_1;
		led_needs_update=1;
    }

    if(button_was_released(SW3))
    {
		led_aux_set(0x04);
		cur_station= STATION_2;
		led_needs_update=1;
    }

	if(button_was_released(SW4))
	{
		display_state += 1;
		display_state = display_state % NUM_STATES;
		led_needs_update=1;
	}
	if(button_was_released(SW5))
	{
		
	}

	call_count++;
	//called every 5 ms (.005 sec), so every 200
	//calls it's 1 second
	if(call_count == 200)
	{
		g_timestamp++;
		call_count=0;
		needs_local_sample=1;
	}
}

void xbee_get_record(weath_meas *p_ms)
{
	unsigned short src_station, samples[MAX_SAMPLES];
	int num_samps=0, i;
	float lux=0, voltage=0, temp=0, pressure=0, hum=0;
	xbee_print_raw_frame();
	xbee_print_frame();
	src_station = xbee_get_source();
	num_samps = xbee_get_samples(samples, MAX_SAMPLES);
	zero_struct(p_ms);
	last_rssi = xbee_get_rssi();
	for(i=0; i<num_samps; i++)
	{
		voltage = PDV_VIN * ((float)samples[i] / (float)1024);
		if(is_debug())
		{
			printf(R"Sample %d (%s): [%d ADC] {%f V} from station 0x%02x\n", i, names[i], samples[i], voltage, src_station);
			if(samples[i] < 10)
			{
				printf(R"%s sample is undersaturated [%d ADC] {%f V}\n", names[i], samples[i], voltage);
			}
			if(samples[i] > 1015)
			{
				printf(R"%s sample is oversaturated [%d ADC] {%f V}\n", names[i], samples[i], voltage);
			}
		}
		
		//First sample comes from pin 20, which is
		//a PDV 8001
		if(i == LUX_SAMPLE)
		{
			lux = PDV_8001_compute_lux(voltage);
			continue;
		}
		//Second sample comes from at TMP 35 sensor,
		//which is connected to PIN 19
		if(i == TEMP_SAMPLE)
		{
			temp = TMP_36_read_F(voltage);
		}
		
		if(i == HUM_SAMPLE)
		{
			if(is_debug())
			{
				printf(R"Scaled up HIH_4000 voltage is %f\n", voltage * HIH_4000_UP_SCALE);
			}
			hum = HIH_4000_read(voltage * HIH_4000_UP_SCALE);
		}
	}
	p_ms->temp_f = temp;
	p_ms->lux = lux;
	p_ms->humid = hum;
	p_ms->timestamp = g_timestamp;
	p_ms->station = src_station;
}

void main(void) { 
	set_clock_5_5();
    init_xbee(9600);
    init_uart(_UART0, get_clock(), _DEFBAUD);

	led_init();
	led_aux_init();
	led_set_scroll(.03);
 	init_shell();
	init_set_cmd();
	init_MPL115A();
	init_i2c_support();
	init_zilog_cmds();
	init_weather_log();
	
	spi_init();
	ds1722_init();
	
	timer_start_cont(T0, .005);
    timer_enable_irq_low(T0);    

    SET_VECTOR(TIMER0, button_isr);

	add_command(R"weather", "", R"weather [dump] [last] [avg start duration] [address length]\n\
\t- (no parameters) Print current summary statistics\n\
\t- dump:  Dumps all weather measurements contained in the logs\n\
\t- last:  Print the most recent record in the logs\n\
\t- avg <start> <duration>:  Compute the average for all logged measurements between\n\
\t\t the start timestamp (in seconds) for duration (in seconds)\n\
\t- <address> <len>: Print all logs starting from the specified memory address (must be greater than 0x14000)\n", &query_weather);
	add_command(R"debug", "", R"debug [on|off]\n\t- Toggles debug output\n", &cmd_debug);
	add_command(R"plot", "", R"plot [lux|pres|humid|temp] [min time] [max time]\n\
\t- lux:  Compute light values over the given time period and print as a graph over time\n\
\t- pres:  Compute pressure values over the given time period and print as a graph over time\n\
\t- temp:  Compute temperature values over the given time period and print as a graph over time\n\
\t- humid:  Compute humidity values over the given time period and print as a graph over time",
 &cmd_plot);

    EI();
	led_aux_set(1);
	
	printf(R"Hi!");
    printf(R"\n");
	//MPL115A_read_coefficients();
    while(1)
    {
		check_key();
        if(xbee_check_frame())
		{
			xbee_get_record(&msmt);
			msmt.pressure = MPL115A_read_pressure();
			put_measurement(&msmt);
			if(msmt.station == cur_station)
			{
				led_needs_update=1;
			}
        }
		if(needs_local_sample)
		{
			//this is toggled in the button callback
			needs_local_sample=0;
			msmt.station = STATION_0;
			msmt.temp_f = 1.8 * ds1722_get_ftemp() + 32.0;
			msmt.humid = HS1101_read_hum();
			msmt.lux = PDV_8001_read_ana0();
			msmt.timestamp = g_timestamp;
			msmt.pressure = MPL115A_read_pressure();
			put_measurement(&msmt);
			if(cur_station == STATION_0)
			{
				led_needs_update=1;
			}
		}
		if(led_needs_update)
		{
			get_most_recent_station(&msmt, cur_station);
			update_led(&msmt);
			led_needs_update=0;
		}

    }
}
