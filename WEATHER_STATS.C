#include "weather_log.h"
#include "weather_stats.h"
#include "weather_help.h"
#include "debug_flag.h"
#include <stdio.h>


#define HISTO_BINS 10
#define HISTO_COLS 40

int histo_cb(weath_meas* ms, int num);
int most_recent_cb(weath_meas* ms, int num);
int most_recent_station_cb(weath_meas* ms, int num);
int avg_cb(weath_meas* ms, int num);
int counter_cb(weath_meas* ms, int num);
int min_cb(weath_meas* ms, int num);
int max_cb(weath_meas* ms, int num);
int stats_cb(weath_meas* ms, int num);
void w_histo(unsigned short station, int seconds, int duration, unsigned char offset_idx, char* param, char* units);

weath_meas *_g_dest;
int avg_count, valid, invalid, g_seconds, g_duration, val_offset_bytes;
unsigned short g_station;
int bins[HISTO_BINS];
float sum[HISTO_BINS];



int most_recent_cb(weath_meas* ms, int num)
{
	if(!is_valid_record(ms))
	{
		return 1;
	}
	if((ms->station & g_station) == ms->station)
	{
		deep_copy_record(_g_dest, ms);
		return 0;//zero, to stop iterating after the first sample
	}
	return 1;
}

void dump_ram_logs()
{
	printf(R"%d in RAM...\n", get_cur_ram_tail());
	iterate_ram_logs(&print_measurement);
}

void w_get_average(weath_meas *dest, int start_sec, int duration_sec)
{
	zero_struct(dest);
	_g_dest = dest;
	avg_count=0;
	iterate_all_logs(&avg_cb);
	_g_dest->temp_f /= (float)avg_count;
	_g_dest->pressure /= (float)avg_count;
	_g_dest->lux /= (float)avg_count;
	_g_dest->humid /= (float)avg_count;
	_g_dest->timestamp = avg_count;
	_g_dest->station = avg_count;
}


int avg_cb(weath_meas* ms, int num)
{	
	if(!is_valid_record(ms))
	{
		return 1;
	}
	_g_dest->temp_f += ms->temp_f;
	_g_dest->humid += ms->humid;
	_g_dest->pressure += ms->pressure;
	_g_dest->lux += ms->lux;
	_g_dest->station = ms->station;
	avg_count++;
	return 1;
}

void w_find_min(weath_meas *dest)
{
	valid=0;
	_g_dest = dest;
	max_struct(dest);
	iterate_all_logs(&min_cb);
//	dest->timestamp=valid;
}

void w_find_max(weath_meas *dest)
{
	valid=0;
	_g_dest = dest;
	zero_struct(dest);
	iterate_all_logs(&max_cb);
//	dest->timestamp=valid;
}

int min_cb(weath_meas* ms, int num)
{
	
	if(!is_valid_record(ms))
	{
		return 1;
	}
	_g_dest->humid = _min(_g_dest->humid, ms->humid);
	_g_dest->temp_f = _min(_g_dest->temp_f, ms->temp_f);
	_g_dest->pressure = _min(_g_dest->pressure, ms->pressure);
	_g_dest->lux = _min(_g_dest->lux, ms->lux);
	_g_dest->timestamp = _min(_g_dest->timestamp, ms->timestamp);
	_g_dest->station = _min(_g_dest->station, ms->station);
	valid++;
	return 1;
}


int max_cb(weath_meas* ms, int num)
{
	
	if(!is_valid_record(ms))
	{
		return 1;
	}
	_g_dest->humid = _max(_g_dest->humid, ms->humid);
	_g_dest->temp_f = _max(_g_dest->temp_f, ms->temp_f);
	_g_dest->pressure = _max(_g_dest->pressure, ms->pressure);
	_g_dest->lux = _max(_g_dest->lux, ms->lux);
	_g_dest->timestamp = _max(_g_dest->timestamp, ms->timestamp);
	_g_dest->station = _max(_g_dest->station, ms->station);
	valid++;
	return 1;
}

int counter_cb(weath_meas* ms, int num)
{
	if(is_valid_record(ms))
	{
		valid++;
	}else
	{
		invalid++;
	}
	return 1;
}

//print number of samples in RAM / ROM,
//min and max timesteps
//min and max values
//average values
//notable events/changes?
void w_print_stats()
{
	weath_meas tmp;
	int begin, end;
	w_find_min(&tmp);
	begin = tmp.timestamp;	
	printf(R"Minimums:\n");
	print_measurement(&tmp, 0);
	
	w_find_max(&tmp);
	end = tmp.timestamp;
	printf(R"Maximums:\n");
	print_measurement(&tmp, 1);
	printf(R"-----------------------\n");

	valid=0;
	invalid=0;
	iterate_ram_logs(&counter_cb);
	printf(R"RAM stats: %d valid records, %d invalid\n", valid, invalid);
	
	valid=0;
	invalid=0;
	iterate_all_erom_logs(&counter_cb);
	printf(R"EROM stats: %d valid records, %d invalid\n", valid, invalid);
	printf(R"There should be: %d total records, hopefully all valid.\n", get_num_records());
	
	printf(R"Currently on page: 0x%06lx, byte 0x%06lx in EROM\n",
		get_cur_erom_page(),  get_cur_erom_tail());
		
//	w_histo_lux(begin, end-begin);
//	w_histo_temp(begin, end-begin);
//	w_histo_pres(begin, end-begin);
//	w_histo_humid(begin, end-begin);
}

void w_histo_lux(unsigned short station, int seconds, int duration)
{
	w_histo(station, seconds, duration, 1, R"Light", R"Ohms");
}

void w_histo_temp(unsigned short station, int seconds, int duration)
{
	w_histo(station, seconds, duration, 0, R"Temperature", R"F");
}

void w_histo_pres(unsigned short station, int seconds, int duration)
{
	w_histo(station, seconds, duration, 2, R"Pressure", R"in");
}
void w_histo_humid(unsigned short station, int seconds, int duration)
{
	//the lone % will be nested in a %s, later, making it work
	w_histo(station, seconds, duration, 3, R"Humidity", R"%%");
}


/*
 offset - index offset within the weath_meas structu, so for temp it's 0,
 			for lux it's 1, etc.  This is multiplied by sizeof(float) to
			get the byte offset
 units - string for units when printing the histogram
*/
void w_histo(unsigned short station, int seconds, int duration, unsigned char offset_idx, char* parameter, char* units)
{
	float min_val, max_val, step_val, loop, step_time;
	int i, j, no_samples=1;
	for(i=0;i<HISTO_BINS;i++)
	{
		bins[i]=0;
		sum[i]=0.0;
	}	
	val_offset_bytes = offset_idx * sizeof(float);
	//set these for the callback to use..
	g_seconds = seconds;
	g_duration = duration;
	
	g_station = station;
	iterate_all_logs(&histo_cb);
	
	//find max bin...
	max_val=0.0;
	for(i=0; i<HISTO_BINS; i++)
	{
		//avoid divide by zero
		if(bins[i] == 0)
		{
			sum[i] = 0;
		}else
		{
			sum[i] /= (float)bins[i]; //compute average
		}
		if(sum[i] > max_val)
		{
			max_val = sum[i];
		}
	}
	//go through again and find the mins
	min_val = max_val;
	for(i=0; i<HISTO_BINS; i++)
	{
		if(bins[i] == 0)
		{
			continue;//ignore zero bins, we'll ignore while printing too
		}
		no_samples=0;
		if(sum[i] < max_val)
		{
			min_val = sum[i];
		}
	}
	if(no_samples)
	{
		return;
	}
	step_val = (max_val-min_val) / (float)HISTO_COLS;
	step_time = (float)duration / (float)HISTO_BINS;
	printf(R"Timeline values for %s at station 0x%04x:\n", parameter, station);
	printf(R"-----------------------\n");

	for(i=0; i<HISTO_BINS; i++)
	{
		if(bins[i] == 0)
		{
			continue; //ignore zero bins
		}
		printf(R"% 3d sec (%.04f %s avg of %d):", seconds + ((float)i*step_time), sum[i], units, bins[i]);
		loop = min_val;
		for(j=0; j<HISTO_COLS; j++)
		{
			if(loop > sum[i])
			{
				break;
			}
			printf(R"x");
			loop += step_val;
		}
		printf(R"\n");
	}
	printf(R"-----------------------\n");
}

int histo_cb(weath_meas* ms, int num)
{
	float *p_val, step_time;
	int bin;
	unsigned char* byte_ptr = (unsigned char*) ms;
	if(!is_valid_record(ms))
	{
		if(is_debug())
        printf("Record %d is invalid\n", ms->timestamp);
		return 1;
	}
	if(ms->station != g_station)
	{
		if(is_debug())
        printf("Station 0x%04x doesn't match 0x%04x\n", ms->station, g_station);
		return 1;
	}
	p_val = (float*)(byte_ptr + val_offset_bytes);
	step_time = (float)g_duration / (float)HISTO_BINS;
	if(step_time <= 0.0)
	{
		if(is_debug())
        printf("Step time is %f, returning (g_duration=%d)\n", step_time, g_duration);
		return 1;
	}
	if(ms->timestamp > g_seconds && ms->timestamp <= g_seconds + g_duration)
	{
		bin =((float)(ms->timestamp-g_seconds) / step_time);
		bins[bin]++;
		sum[bin] += *p_val;
	}else
    {
		if(is_debug())
        printf("Timestamp %d out of bounds of [%d, %d]\n", ms->timestamp, g_seconds, g_seconds+g_duration);
    }
	return 1;
}