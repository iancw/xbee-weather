#include "weather_help.h"

#include <stdio.h>
#include "debug_flag.h"
#include "weather_log.h"


void deep_copy_record(weath_meas* dest, weath_meas *src)
{
	dest->temp_f = src->temp_f;
	dest->humid = src->humid;
	dest->pressure = src->pressure;
	dest->lux = src->lux;
	dest->timestamp = src->timestamp;
	dest->station = src->station;
}

void zero_struct(weath_meas *dest)
{
	dest->temp_f = 0.0;
	dest->humid = 0.0;
	dest->pressure=0.0;
	dest->lux=0.0;
	dest->timestamp=0;
	dest->station=0;
}

void max_struct(weath_meas *dest)
{
	dest->temp_f = 1000;
	dest->humid = 100;
	dest->pressure=3000;
	dest->lux=10000;
	dest->timestamp=0x7FFFFFFF;
	dest->station=0xFFFF;
}


int print_measurement(weath_meas *ms, unsigned int num)
{
	printf(R"(0x%06x)-----------------------\n", num);
	printf(R"Temp: %f F\n", ms->temp_f);
	printf(R"Lux: %f K Ohms\n", ms->lux);
	printf(R"Pressure: %f in\n", ms->pressure);
	printf(R"Humid: %f %%\n", ms->humid);
	printf(R"Timestamp: %d secs\n", ms->timestamp);
	printf(R"Station: %04x\n", ms->station);
	return 1;
}


int is_valid_record(weath_meas *record)
{
	if(record->temp_f == 0xFFFFFFFF
	||record->temp_f == 0x0
	||record->temp_f == 0.0
	||record->temp_f == -0.0
	||record->temp_f > 400.0
	|| record->temp_f < -200.0)
	{
		if(is_debug())
		{
			printf(R"temp %f is invalid\n", record->temp_f);
		}
		return 0;
	}
	if(record->pressure == 0xFFFFFFFF
	||record->pressure == -0.0
	||record->pressure > 3000.0
	||record->pressure < -3000.0)
	{
		if(is_debug())
		{
			printf(R"pressure %f is invalid\n", record->pressure);			
		}
		return 0;
	}
	if(record->humid == 0xFFFFFFFF
		||record->humid == -0.0
		||record->humid > 100
		||record->humid < 0)
	{
		if(is_debug())
		{
			printf(R"humidity %f is invalid\n", record->humid);
		}
		return 0;
	}
	if(record->lux == 0xFFFFFFFF
		||record->lux == 0x0
		||record->lux == 0.0
		||record->lux == -0.0)
		{
			if(is_debug())
			{
				printf(R"Lux %f is invalid\n", record->lux);
			}
			return 0;
		}
	if(record->timestamp == 0xFFFFFFFF
		||record->timestamp < 0)
		{
			if(is_debug())
			{
				printf(R"timestamp %d is invalid\n", record->timestamp);
			}
			return 0;
		}
		
	return 1;
}


float _min(float a, float b)
{
	if(a < b)
	{
		return a;
	}
	return b;
}

float _max(float a, float b)
{
	if(a > b)
	{
		return a;
	}
	return b;
}