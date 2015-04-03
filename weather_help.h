#ifndef __WEATHER_HELP_H__
#define __WEATHER_HELP_H__

#include "weather_log.h"

void zero_struct(weath_meas *dest);
void max_struct(weath_meas *dest);
int print_measurement(weath_meas *ms, unsigned int num);
int is_valid_record(weath_meas *record);
float _min(float a, float b);
float _max(float a, float b);
void deep_copy_record(weath_meas* dest, weath_meas *src);

#endif