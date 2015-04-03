#ifndef __WEATHER_STATS_H__
#define __WEATHER_STATS_H__

#include "weather_log.h"

void w_histo_lux(unsigned short station, int start, int duration);
void w_histo_temp(unsigned short station, int start, int duration);
void w_histo_pres(unsigned short station, int start, int duration);
void w_histo_humid(unsigned short station, int start, int duration);
void w_print_stats();

void w_get_average(weath_meas *dest, int start_sec, int duration_sec);



void dump_ram_logs();



#endif