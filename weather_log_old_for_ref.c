#include "weather_log.h"
#include "25lc040a_eeprom.h"
#include <stdio.h>
#include <string.h>
#include "flash.h"
#include "clock.h"
#include "debug_flag.h"

#define NUM_STATIONS 1

//this value was chosen after consulting the map file, these are 
//some big structs
#define N_RAM_SAMPLES 10 

#define NUM_PAGES 24
#define BLOCK_SIZE  N_RAM_SAMPLES*sizeof(weath_meas)
#define STRUCT_SIZE sizeof(weath_meas);
#define SAMPS_PER_PAGE (PAGE_SIZE*sizeof(short)) / sizeof(weath_meas)
#define EROM_START 0x14000

//This stores settings persistently in ROM
//It doesn't actually need to be that space efficient 
//because it needs to eat up a full page of ROM (2048 K)
//for erasing /re-writing purposes
struct weather_settings
{
	//these should be retrieved from settings at boot time
	unsigned char cur_page; //only needs to save 0 - 24
	unsigned short cur_byte; //only needs to store 0-1024 (requires 11 bits)
	unsigned char head_page; //When pages wrap around, this tells you which is the first
	_Erom unsigned short* last;
};
typedef struct weather_settings weather_settings;

//Global member variables

//This is the location in RAM where samples are stored, the size is
//pretty limited because there's only 4K and these structs are 20 bytes 
//each.  
weath_meas measurements[NUM_STATIONS][N_RAM_SAMPLES];

//This is where data is stored for the longer term, it's about 50K
//at the top of EROM.  This could be expanded to more, but I didn't
//want to be too aggressive while I'm still adding to the program 
//size.
_Erom unsigned short data_store[NUM_PAGES][PAGE_SIZE] _At EROM_START ; //about 50 KB of storage

//This persists the weather_settings struct between power cycles
_Rom unsigned short settings[PAGE_SIZE];

//The settings object that saves state information within this file
//it's persisted to ROM when changed
weather_settings _s;

//This stores the last sample for each of the 3 stations
//It's the last value in the RAM array, so it doesn't
//need to be persisted in ROM
int last[3] = {0, 0, 0};

//these are because the compiler had problems with preprocessor defines
int SAMPLES_PER_EROM_PAGE=SAMPS_PER_PAGE;
int MEASUREMENT_SIZE=sizeof(weath_meas);
int MEAS_SIZE_SHORT = sizeof(weath_meas)/2;

void transfer_to_erom(int station);
void copy_msmt(weath_meas *sink, _Erom unsigned short *src);
//Saves _s to ROM
void save_settings();
//Returns 1 if the memory address contains a valid record
int p_is_valid_record(_Erom unsigned short* ptr);
int is_valid_record(weath_meas *record);
//supports histograms of any value, the offset describes the 
//offset within the struct in bytes
void w_histo(int seconds, int duration, unsigned char offset_idx, char* unit_str );

typedef void(*ptItr)(weath_meas* ms, int num);

void iterate_all_logs(ptItr func);
void iterate_all_erom_logs(ptItr func);
void iterate_ram_logs(ptItr func);
void iterate_erom_logs(ptItr func, _Erom unsigned short* start, int records);

int num_ram_samples()
{
	return last[0] / STRUCT_SIZE;
}

int num_rom_samples()
{
	int samples = SAMPS_PER_PAGE;
	int structsz = STRUCT_SIZE;
	return (_s.cur_page * samples * structsz) + (_s.cur_byte / structsz);
}

void init_weather_log()
{
	//Later, we could load these from ROM
	_s.cur_page=0;
	_s.cur_byte=0;
	_s.head_page=0;
	_s.last = data_store[0];
}

void save_settings()
{
	
}

int at_page_end()
{
	return _s.cur_byte >= SAMPLES_PER_EROM_PAGE;
}

//Compressed contents of head_page through cur_page-1
//into cur_page
void compress_pages()
{
	
}

int next_erom_page()
{
	if(is_debug())
	{
		printf(R"Reached the end of page %d, moving on to %d\n", 
		_s.cur_page, (_s.cur_page+1) % NUM_PAGES);
	}
	_s.cur_byte = 0;
	_s.cur_page = (_s.cur_page+1) % NUM_PAGES;
	
	//the last EROM page stores a summary of all previous pages, then
	//those pages are considered blank and can be overwritten
	//then that page becomes the new head
	//so the first time this happens will be when page 22 wraps
	//at that time, 0-22 will be summariezed in page 23, and page
	//23 will become the new head
	if(_s.cur_page == (_s.head_page))
	{
		if(is_debug())
		{
			printf(R"cur page wrapped around to head_page, this shouldn't happen if data decimiation is implemented\n");
		}
		_s.head_page = (_s.head_page+1) % NUM_PAGES;
		/* data decimation code
		//we've wrapped our buffer, so we need to lose a page
		_s.head_page = _s.cur_page;
		if(is_debug())
		{
			printf(R"Wrapped the EROM buffer, head is now %d (0x%06lx)\n", _s.head_page, 
				(unsigned int)data_store[_s.head_page]);
			printf(R"Summary of previous pages 0-22 is in page 0x%06lx\n", (unsigned int)data_store[_s.head_page]);
		}
		*/
	}
	return 1;
}

/*
 * Writes contents of measurements buffer to the eeprom
*/
void write_buffer(int station)
{
	int i, j, max;
	unsigned short* ptr;
	station=0;
	max = PAGE_SIZE - STRUCT_SIZE;
	init_flash(get_clock());
	if(_s.cur_page >= NUM_PAGES)
	{
		_s.head_page = 
		_s.cur_byte=0;
		_s.cur_page=0;

	}
	if(is_debug())
	{
		printf(R"Dumping to flash at address 0x%06lx\n", &data_store[_s.cur_page][_s.cur_byte]);
	}
	//When cur_byte is 0 that means we're writing to a new page, so we need to erase it
	if(_s.cur_byte == 0)
	{
		page_erase(data_store[_s.cur_page]);
	}
	page_unlock(data_store[_s.cur_page]);

	for(i=0; i<N_RAM_SAMPLES; i++)
	{
		if(!is_valid_record(&measurements[station][i]))
		{
			if(is_debug())
			{
				printf(R"Record %d is not valid, will not save in EROM...\n", i);
				print_measurement(&measurements[station][i], i);
			}
			continue;
		}
		ptr = (unsigned short*)&measurements[station][i];
		for(j=0; j<MEASUREMENT_SIZE; j++)
		{
			data_store[_s.cur_page][_s.cur_byte++] = *(ptr+j);
			if(at_page_end())
			{
				next_erom_page();
			}
		}
		_s.last = &data_store[_s.cur_page][_s.cur_byte];
	}
	lock_flash();
	//increment deal with wrapping
	
	save_settings();
}

void put_measurement(int station, weath_meas *measurement)
{
	int cur;
	station=0;
	cur = last[station];
	
	measurements[station][cur++] = *measurement;
	last[station] = cur;
    if(last[station] >= N_RAM_SAMPLES)
    {
		write_buffer(station);
        last[station] = 0;
    }
}

void get_most_recent(weath_meas *dest)
{
	int station=0;
	int idx=0;
	idx = last[station] - 1;
    if(idx < 0)
    {
        idx=N_RAM_SAMPLES-1;
    }
	dest->temp_f =  measurements[station][idx].temp_f;
	dest->pressure = measurements[station][idx].pressure;
	dest->lux = measurements[station][idx].lux;
	dest->humid = measurements[station][idx].humid;
	dest->timestamp = measurements[station][idx].timestamp;
}

void print_measurement(weath_meas *ms, int num)
{

	printf(R"(%d)-----------------------\n", num);
	printf(R"Temp: %f F\n", ms->temp_f);
	printf(R"Lux: %f K Ohms\n", ms->lux);
	printf(R"Pressure: %f in\n", ms->pressure);
	printf(R"Humid: %f %%\n", ms->humid);
	printf(R"Timestamp: %d secs\n", ms->timestamp);
}

void dump_ram_logs(int station)
{	
	

void iterate_all_logs(ptItr function)
{
	
}

void iterate_ram_logs(ptItr function)
{
	int num_records, i;
	station=0;
	printf(R"%d in RAM...\n", last[station]);
	for(i=0; i<last[station]; i++)
	{
		if(!is_valid_record(&measurements[station][i]))
		{
			printf(R"Invalid record (%d)\n", i+1);
			continue;
		}
		print_measurement(&measurements[station][i], i+1);
	}
	num_records = _s.cur_page * SAMPLES_PER_EROM_PAGE + _s.cur_byte/MEASUREMENT_SIZE;
	printf(R"%d in EROM...\n", num_records);
	dump_from_address(data_store[0], num_records);
}
}

void iterate_erom_logs(ptItr function, _Erom unsigned short* ptr, int records)
{
	
}

void iterate_all_erom_logs(ptItr function)
{
	
}

void dump_from_address(_Erom unsigned short* ptr, int records)
{
	int s_page=0, e_page=0, page_max=0, bytes, i, j, count=0, expanded;
	unsigned int i_ptr = (unsigned int) ptr;
	weath_meas measu;
	_Erom unsigned short* page;
	_Erom unsigned short* record;
	
	if(i_ptr < 0x14000)
	{
		printf(R"Must specify an address beyond 0x14000\n");
		return;
	}
	bytes = records * sizeof(weath_meas);

	s_page = (i_ptr>>11) - (EROM_START>>11); //shifting right 11 is live dividing by 2048
	e_page = ((i_ptr + bytes)>>11) -(EROM_START>>11);
	
	
	for(i=s_page; i<=e_page; i++)
	{
		if(i == e_page)
		{
			page_max = ((i_ptr+bytes) & 0x7FF) / sizeof(weath_meas);
		}else
		{
			page_max = SAMPLES_PER_EROM_PAGE;
		}

		for(j=0; j<page_max; j++)
		{
			expanded = j*MEAS_SIZE_SHORT;
			page = data_store[i];
			record = page + expanded;
			copy_msmt(&measu, record);
			if(!is_valid_record(&measu))
			{
				printf(R"Invalid record (%d)\n", count++);
				continue;
			}
			print_measurement(&measu, count++);
		}
	}
	printf(R"-----------------------\n");
}

void copy_msmt(weath_meas *sink, _Erom unsigned short *src)
{
	int i, struct_sz;
	unsigned short * s_ptr;
	s_ptr = (unsigned short*)sink;
	struct_sz = STRUCT_SIZE;
	for(i=0; i<struct_sz/2; i++) // div 2 because it's a pointer to shorts
	{
		*(s_ptr+i) = *(src+i);
	}
}

int p_is_valid_record(_Erom unsigned short* ptr)
{
	weath_meas ms;
	copy_msmt(&ms, ptr);
	return is_valid_record(&ms);
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
//		printf(R"temp %f is invalid\n", record->temp_f);
		return 0;
	}
	if(record->pressure == 0xFFFFFFFF
	||record->pressure == -0.0
	||record->pressure > 3000.0
	||record->pressure < -3000.0)
	{
//		printf(R"pressure %f is invalid\n", record->pressure);
		return 0;
	}
	if(record->humid == 0xFFFFFFFF
		||record->humid == -0.0
		||record->humid > 100
		||record->humid < 0)
	{
//		printf(R"humidity %f is invalid\n", record->humid);
		return 0;
	}
	if(record->lux == 0xFFFFFFFF
		||record->lux == 0x0
		||record->lux == 0.0
		||record->lux == -0.0)
		{
//			printf(R"Lux %f is invalid\n", record->lux);
			return 0;
		}
	if(record->timestamp == 0xFFFFFFFF
		||record->timestamp < 0)
		{
//			printf(R"timestamp %d is invalid\n", record->timestamp);
			return 0;
		}
		
	return 1;
}

void zero_struct(weath_meas *dest)
{
	dest->temp_f = 0.0;
	dest->humid = 0.0;
	dest->pressure=0.0;
	dest->lux=0.0;
	dest->timestamp=0;
}

void max_struct(weath_meas *dest)
{
	dest->temp_f = 1000;
	dest->humid = 100;
	dest->pressure=3000;
	dest->lux=10000;
	dest->timestamp=0x7FFFFFFF;
}


void w_get_average(weath_meas *dest, int start_sec, int duration_sec)
{
	weath_meas meas;
	_Erom unsigned short* ptr;
	_Erom unsigned short* page;
	int i,j, invalid=0, avg_count=0, max_invalid = 100,offset=0,erom_count=0;
	ptr = (_Erom unsigned short*)EROM_START;
	zero_struct(dest);
	for(i=0; i<last[0]; i++)
	{
		if(measurements[0][i].timestamp > start_sec
		 && measurements[0][i].timestamp <= start_sec + duration_sec)
		{
			dest->temp_f += measurements[0][i].temp_f;
			dest->humid += measurements[0][i].humid;
			dest->pressure += measurements[0][i].pressure;
			dest->lux += measurements[0][i].lux;
			avg_count++;
		}
	}
	for(i=0; i<NUM_PAGES; i++)
	{
		for(j=0; j<SAMPLES_PER_EROM_PAGE; j++)
		{
		 	offset= j*MEAS_SIZE_SHORT;
			page = data_store[i];
			ptr = page + offset;
			if(ptr > _s.last)
			{
				break;
			}
			copy_msmt(&meas, ptr);
			if(!is_valid_record(&meas))
			{
				invalid++;
				if(invalid > max_invalid)
				{
					if(is_debug())
					{
						printf(R"Breaking due to excess invalid records (%d)\n", invalid);
					}
					break;
				}
				continue;
			}

			invalid=0;
			if(meas.timestamp > start_sec && meas.timestamp <= start_sec + duration_sec)
			{
				dest->temp_f += meas.temp_f;
				dest->humid += meas.humid;
				dest->pressure += meas.pressure;
				dest->lux += meas.lux;
				avg_count++;
			}
		}
		if(ptr > _s.last)
		{
			break;
		}
	}
	dest->temp_f /= (float)avg_count;
	dest->pressure /= (float)avg_count;
	dest->lux /= (float)avg_count;
	dest->humid /= (float)avg_count;
	dest->timestamp = avg_count;
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

void w_find_min(weath_meas *dest)
{
	weath_meas tmp;
	int invalid=0, consec_inv=0,max_invalid=100, valid=0, i, offset=0,j;
	_Erom unsigned short* ptr;
	_Erom unsigned short* page;
	ptr = (_Erom unsigned short*)EROM_START;

	max_struct(dest);
	
	for(i=0; i<last[0]; i++)
	{
		if(!is_valid_record(&measurements[0][i]))
		{
			invalid++;
			continue;
		}
		dest->humid = _min(dest->humid, measurements[0][i].humid);
		dest->temp_f = _min(dest->temp_f, measurements[0][i].temp_f);
		dest->pressure = _min(dest->pressure, measurements[0][i].pressure);
		dest->lux = _min(dest->lux, measurements[0][i].lux);
		dest->timestamp = _min(dest->timestamp, measurements[0][i].timestamp);
		valid++;
	}
	
	for(i=0; i<NUM_PAGES; i++)
	{
		for(j=0; j<SAMPLES_PER_EROM_PAGE; j++)
		{
		 	offset= j*MEAS_SIZE_SHORT;
			page = data_store[i];
			ptr = page + offset;
			if(ptr > _s.last)
			{
				break;
			}
			copy_msmt(&tmp, ptr);
			if(!is_valid_record(&tmp))
			{
				invalid++;
				consec_inv++;
				if(consec_inv > max_invalid)
				{
					break;
				}
				continue;
			}
			consec_inv=0;
			valid++;
			dest->humid = _min(dest->humid, tmp.humid);
			dest->temp_f = _min(dest->temp_f, tmp.temp_f);
			dest->pressure = _min(dest->pressure, tmp.pressure);
			dest->lux = _min(dest->lux, tmp.lux);
			dest->timestamp = _min(dest->timestamp, tmp.timestamp);
		}
		if(ptr > _s.last)
		{
			break;
		}
	}
}

void w_find_max(weath_meas *dest)
{
	weath_meas tmp;
	int invalid=0, consec_inv=0,max_invalid=100, valid=0, i, offset=0,j;
	_Erom unsigned short* ptr;
	_Erom unsigned short* page;
	ptr = (_Erom unsigned short*)EROM_START;
	zero_struct(dest);

	
	for(i=0; i<last[0]; i++)
	{
		if(!is_valid_record(&measurements[0][i]))
		{
			invalid++;
			continue;
		}
		
		dest->humid = _max(dest->humid, measurements[0][i].humid);
		dest->temp_f = _max(dest->temp_f, measurements[0][i].temp_f);
		dest->pressure = _max(dest->pressure, measurements[0][i].pressure);
		dest->lux = _max(dest->lux, measurements[0][i].lux);
		dest->timestamp = _max(dest->timestamp, measurements[0][i].timestamp);
		valid++;
	}
	
	for(i=0; i<NUM_PAGES; i++)
	{
		for(j=0; j<SAMPLES_PER_EROM_PAGE; j++)
		{
		 	offset= j*MEAS_SIZE_SHORT;
			page = data_store[i];
			ptr = page + offset;
			if(ptr > _s.last)
			{
				break;
			}
			copy_msmt(&tmp, ptr);
			if(!is_valid_record(&tmp))
			{
				invalid++;
				consec_inv++;
				if(consec_inv > max_invalid)
				{
					break;
				}
				continue;
			}
			consec_inv=0;
			valid++;
			
			dest->humid = _max(dest->humid, tmp.humid);
			dest->temp_f = _max(dest->temp_f, tmp.temp_f);
			dest->pressure = _max(dest->pressure, tmp.pressure);
			dest->lux = _max(dest->lux, tmp.lux);
			dest->timestamp = _max(dest->timestamp, tmp.timestamp);
		}
		if(ptr > _s.last)
		{
			break;
		}
	}
}

//print number of samples in RAM / ROM,
//min and max timesteps
//min and max values
//average values
//notable events/changes?
void w_print_stats()
{
	weath_meas min,max,avg,tmp;
	int invalid=0, consec_inv=0,max_invalid=100, valid=0, i, offset=0,j;
	_Erom unsigned short* ptr;
	_Erom unsigned short* page;
	ptr = (_Erom unsigned short*)EROM_START;
	zero_struct(&max);
	max_struct(&min);
	zero_struct(&avg);
	
	for(i=0; i<last[0]; i++)
	{
		if(!is_valid_record(&measurements[0][i]))
		{
			invalid++;
			continue;
		}
		min.humid = _min(min.humid, measurements[0][i].humid);
		min.temp_f = _min(min.temp_f, measurements[0][i].temp_f);
		min.pressure = _min(min.pressure, measurements[0][i].pressure);
		min.lux = _min(min.lux, measurements[0][i].lux);
		min.timestamp = _min(min.timestamp, measurements[0][i].timestamp);
	
		max.humid = _max(max.humid, measurements[0][i].humid);
		max.temp_f = _max(max.temp_f, measurements[0][i].temp_f);
		max.pressure = _max(max.pressure, measurements[0][i].pressure);
		max.lux = _max(max.lux, measurements[0][i].lux);
		max.timestamp = _max(max.timestamp, measurements[0][i].timestamp);
		valid++;
	}
	
	for(i=0; i<NUM_PAGES; i++)
	{
		for(j=0; j<SAMPLES_PER_EROM_PAGE; j++)
		{
		 	offset= j*MEAS_SIZE_SHORT;
			page = data_store[i];
			ptr = page + offset;
			if(ptr > _s.last)
			{
				break;
			}
			copy_msmt(&tmp, ptr);
			if(!is_valid_record(&tmp))
			{
				invalid++;
				consec_inv++;
				if(consec_inv > max_invalid)
				{
					break;
				}
				continue;
			}
			consec_inv=0;
			valid++;
			min.humid = _min(min.humid, tmp.humid);
			min.temp_f = _min(min.temp_f, tmp.temp_f);
			min.pressure = _min(min.pressure, tmp.pressure);
			min.lux = _min(min.lux, tmp.lux);
			min.timestamp = _min(min.timestamp, tmp.timestamp);
		
			max.humid = _max(max.humid, tmp.humid);
			max.temp_f = _max(max.temp_f, tmp.temp_f);
			max.pressure = _max(max.pressure, tmp.pressure);
			max.lux = _max(max.lux, tmp.lux);
			max.timestamp = _max(max.timestamp, tmp.timestamp);
		}
		if(ptr > _s.last)
		{
			break;
		}
	}
	printf(R"Found %d samples (%d invalid) from %d seconds to %d seconds\n", valid, invalid, min.timestamp, max.timestamp);
	printf(R"Minimums:\n");
	print_measurement(&min, 0);
	printf(R"Maximums:\n");
	print_measurement(&max, 1);
	printf(R"Currently on page: %d, byte %d (0x%08lx) of EROM, with %d records in RAM\n", 
	_s.cur_page, _s.cur_byte, (unsigned int)_s.last, last[0]);
}

void w_histo_lux(int seconds, int duration)
{
	w_histo(seconds, duration, 1, R"Ohms");
}

void w_histo_temp(int seconds, int duration)
{
	w_histo(seconds, duration, 0, R"F");
}

void w_histo_pres(int seconds, int duration)
{
	w_histo(seconds, duration, 2, R"in");
}
void w_histo_humid(int seconds, int duration)
{
	w_histo(seconds, duration, 3, R"%%");
}

#define BINS 10
#define HISTO_COLS 40
/*
 offset - index offset within the weath_meas structu, so for temp it's 0,
 			for lux it's 1, etc.  This is multiplied by sizeof(float) to
			get the byte offset
 units - string for units when printing the histogram
*/
void w_histo(int seconds, int duration, unsigned char offset_idx, char* units)
{
	weath_meas meas;
	_Erom unsigned short* ptr;
	_Erom unsigned short* page;
	unsigned char* p_meas;
	float *p_val;
	int i,j, invalid=0, max_invalid = 100,pg_offset=0,erom_count=0, col_scale, bin;
	int min_time, max_time, step_time;
	unsigned int offset_bytes;
	int bins[BINS];
	float sum[BINS];
	float min_val, max_val, step_val;
	for(i=0;i<BINS;i++)
	{
		bins[i]=0;
		sum[i]=0.0;
	}
	offset_bytes = offset_idx * sizeof(float);
	
	min_time = seconds;
	max_time = seconds+duration;
	step_time = duration / BINS;

	ptr = (_Erom unsigned short*)EROM_START;

	for(i=0; i<last[0]; i++)
	{

		if(measurements[0][i].timestamp > seconds
		 && measurements[0][i].timestamp <= seconds + duration)
		{
			bin =((measurements[0][i].timestamp-min_time) / step_time);
			bins[bin]++;
			p_meas = (unsigned char*)&measurements[0][i];
			sum[bin] += (float)*(p_meas + offset_bytes);
		}
	}
	//can do this once for the entire loop, because they all share
	//the same structure
	p_meas = (unsigned char*)&meas;
	p_val = (float*)(p_meas + offset_bytes);
	for(i=0; i<NUM_PAGES; i++)
	{
		for(j=0; j<SAMPLES_PER_EROM_PAGE; j++)
		{
		 	pg_offset= j*MEAS_SIZE_SHORT;
			page = data_store[i];
			ptr = page + pg_offset;
			if(ptr > _s.last)
			{
				break;
			}
			copy_msmt(&meas, ptr);
			if(!is_valid_record(&meas))
			{
				invalid++;
				if(invalid > max_invalid)
				{
					printf(R"Breaking due to excess invalid records (%d)\n", invalid);
					break;
				}
				continue;
			}

			invalid=0;
			if(meas.timestamp > seconds && meas.timestamp <= seconds + duration)
			{
				bin =((meas.timestamp-min_time) / step_time);
				bins[bin]++;

				sum[bin] += *p_val;
			}
		}
		if(ptr > _s.last)
		{
			break;
		}
	}
	//find max bin...
	max_val=0.0;
	for(i=0; i<BINS; i++)
	{
		sum[i] /= (float)bins[i]; //compute average
		if(sum[i] > max_val)
		{
			max_val = sum[i];
		}
	}
	step_val = max_val / (float)HISTO_COLS;
	printf(R"Histogram:\n");
	printf(R"-----------------------\n");
	for(i=0; i<BINS; i++)
	{
		printf(R"% 3d sec (%04f %s avg of %d):", min_time + (i*step_time), sum[i], units, bins[i]);
		min_val=0.0;
		for(j=0; j<HISTO_COLS && min_val < sum[i]; j++)
		{
			printf(R"x");
			min_val += step_val;
		}
		printf(R"\n");
	}
	printf(R"-----------------------\n");
}
