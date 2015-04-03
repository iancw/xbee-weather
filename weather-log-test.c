#include <stdio.h>
#include <string.h>

#define PAGE_SIZE 1024

int is_debug(){return 1;}

struct weath_meas
{
	float temp_f;
	float lux;
	float pressure;
	float humid;
	int timestamp;
};
typedef struct weath_meas weath_meas;
typedef void(*ptItr)(weath_meas* ms, int num);

void zero_struct(weath_meas *dest)
{
	dest->temp_f = 0.0;
	dest->humid = 0.0;
	dest->pressure=0.0;
	dest->lux=0.0;
	dest->timestamp=0;
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
			printf("temp %f is invalid\n", record->temp_f);
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
			printf("pressure %f is invalid\n", record->pressure);			
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
			printf("humidity %f is invalid\n", record->humid);
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
				printf("Lux %f is invalid\n", record->lux);
			}
			return 0;
		}
	if(record->timestamp == 0xFFFFFFFF
		||record->timestamp < 0)
		{
			if(is_debug())
			{
				printf("timestamp %d is invalid\n", record->timestamp);
			}
			return 0;
		}
		
	return 1;
}


void print_measurement(weath_meas *ms, int num)
{
	printf("(%d)-----------------------\n", num);
	printf("Temp: %f F\n", ms->temp_f);
	printf("Lux: %f K Ohms\n", ms->lux);
	printf("Pressure: %f in\n", ms->pressure);
	printf("Humid: %f %%\n", ms->humid);
	printf("Timestamp: %d secs\n", ms->timestamp);
}

void init_weather_log();
void put_measurement(int station, weath_meas *measurement);
void get_most_recent(weath_meas *dest);

void dump_ram_logs(int station);
void dump_from_address( unsigned short* ptr, int records);

void iterate_all_logs(ptItr func);
void iterate_all_erom_logs(ptItr func);
void iterate_ram_logs(ptItr func);

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
	unsigned short* tail;
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
unsigned short data_store[NUM_PAGES][PAGE_SIZE]; //about 50 KB of storage

//This persists the weather_settings struct between power cycles
unsigned short settings[PAGE_SIZE];

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
void copy_msmt(weath_meas *sink, unsigned short *src);
//Saves _s to ROM
void save_settings();
//Returns 1 if the memory address contains a valid record
int p_is_valid_record( unsigned short* ptr);
//supports histograms of any value, the offset describes the 
//offset within the struct in bytes

void iterate_erom_logs_i(ptItr func,  unsigned short* start, int records);
void iterate_erom_logs_p(ptItr func,  unsigned short* start,  unsigned short* end);

void init_weather_log()
{
	//Later, we could load these from ROM
	_s.cur_page=0;
	_s.cur_byte=0;
	_s.head_page=0;
	_s.tail = data_store[0];
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
		printf("Reached the end of page %d, moving on to %d\n", 
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
			printf("cur page wrapped around to head_page, this shouldn't happen if data decimiation is implemented\n");
		}
		_s.head_page = (_s.head_page+1) % NUM_PAGES;
		/* data decimation code
		//we've wrapped our buffer, so we need to lose a page
		_s.head_page = _s.cur_page;
		if(is_debug())
		{
			printf("Wrapped the EROM buffer, head is now %d (0x%06lx)\n", _s.head_page, 
				(unsigned int)data_store[_s.head_page]);
			printf("Summary of previous pages 0-22 is in page 0x%06lx\n", (unsigned int)data_store[_s.head_page]);
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
//	init_flash(get_clock());
	if(_s.cur_page >= NUM_PAGES)
	{
		_s.head_page = 
		_s.cur_byte=0;
		_s.cur_page=0;

	}
	if(is_debug())
	{
		printf("Dumping to flash at address 0x%06lx\n", 
		&data_store[_s.cur_page][_s.cur_byte]);
	}
	//When cur_byte is 0 that means we're writing to a new page, so we need to erase it
	if(_s.cur_byte == 0)
	{
//		page_erase(data_store[_s.cur_page]);
	}
//	page_unlock(data_store[_s.cur_page]);

	for(i=0; i<N_RAM_SAMPLES; i++)
	{
		if(!is_valid_record(&measurements[station][i]))
		{
			if(is_debug())
			{
				printf("Record %d is not valid, will not save in EROM...\n", i);
//				print_measurement(&measurements[station][i], i);
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
		_s.tail = &data_store[_s.cur_page][_s.cur_byte];
	}
//	lock_flash();
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

void dump_ram_logs(int station)
{
	printf("%d in RAM...\n", last[station]);
	iterate_ram_logs(&print_measurement);
}
	

void iterate_all_logs(ptItr function)
{
	iterate_ram_logs(function);
	iterate_all_erom_logs(function);
}

void iterate_ram_logs(ptItr function)
{
	int i, station;
	station=0;

	for(i=0; i<last[station]; i++)
	{
		if(!is_valid_record(&measurements[station][i]))
		{
			if(is_debug())
			{
				printf("Invalid record (%d)\n", i+1);
			}
			continue;
		}
		function(&measurements[station][i], i+1);
	}
}

void iterate_all_erom_logs(ptItr function)
{
	iterate_erom_logs_p(function, data_store[_s.head_page], _s.tail);
}


void iterate_erom_logs_p(ptItr function,  unsigned short* p_start,  unsigned short* p_end)
{
	weath_meas meas;
	 unsigned short* ptr;
	 unsigned short* page;
	int i,j, invalid=0, max_invalid = 100,pg_offset=0,erom_count=0, col_scale, bin, tail_page, i_end;
	
	zero_struct(&meas);
	i_end = (unsigned int)p_end;
	
	tail_page = (i_end>>11) - (EROM_START>>11);
	tail_page = (tail_page+1) % NUM_PAGES; //make tail page point to the byond
	ptr = ( unsigned short*)data_store[_s.head_page];

	for(i=_s.head_page; i!= tail_page; i=(i+1) % NUM_PAGES)
	{
		for(j=0; j<SAMPLES_PER_EROM_PAGE; j++)
		{
		 	pg_offset= j*MEAS_SIZE_SHORT;
			page = data_store[i];
			ptr = page + pg_offset;
			if(ptr > _s.tail)
			{
				break;
			}
			copy_msmt(&meas, ptr);

		}
		if(ptr > _s.tail)
		{
			break;
		}
	}
}

void iterate_erom_logs_i(ptItr function,  unsigned short* p_start, int records)
{
	int s_page=0, e_page=0, page_max=0, bytes, i, j, count=0, expanded;
	unsigned int i_ptr = (unsigned int) p_start;
	weath_meas measu;
	 unsigned short* page;
	 unsigned short* record;
	
	if(i_ptr < 0x14000)
	{
		printf("Must specify an address beyond 0x14000\n");
		return;
	}
	bytes = records * sizeof(weath_meas);

	//p_start may not begin on a page boundary, so this resolves that to
	//teh correct page / byte offset
	s_page = (i_ptr>>11) - (EROM_START>>11); //shifting right 11 is live dividing by 2048
	e_page = ((i_ptr + bytes)>>11) -(EROM_START>>11);
	page_max = ((i_ptr+bytes) & 0x7FF) / sizeof(weath_meas);
	
	iterate_erom_logs_p(function, data_store[s_page], &data_store[e_page][page_max]);
}

void dump_from_address( unsigned short* ptr, int records)
{
	iterate_erom_logs_i(&print_measurement, ptr, records);
	printf("-----------------------\n");
}

void copy_msmt(weath_meas *sink,  unsigned short *src)
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

int p_is_valid_record( unsigned short* ptr)
{
	weath_meas ms;
	copy_msmt(&ms, ptr);
	return is_valid_record(&ms);
}

