#include "weather_log.h"
#include "25lc040a_eeprom.h"
#include <stdio.h>
#include <string.h>
#include "flash.h"
#include "clock.h"
#include "debug_flag.h"
#include "weather_help.h"

//this value was chosen after consulting the map file, these are 
//some big structs
#define N_RAM_SAMPLES 10 

#define NUM_PAGES 24
#define STRUCT_SIZE 22//sizeof(weath_meas)
#define SAMPLES_PER_EROM_PAGE 93//(PAGE_SIZE*sizeof(short)) / sizeof(weath_meas)
#define EROM_START 0x14000
#define SHORTS_PER_SAMPLE 11 //sizeof(weath_meas)/2
#define USABLE_SHORTS_PER_PAGE 1023 // SAMPLES_PER_EROM_PAGE * SHORTS_PER_SAMPLE
//This stores settings persistently in ROM
//It doesn't actually need to be that space efficient 
//because it needs to eat up a full page of ROM (2048 K)
//for erasing /re-writing purposes
struct weather_settings
{
	//these should be retrieved from settings at boot time
	unsigned char cur_page; //only needs to save 0 - 24
	unsigned short cur_word; //only needs to store 0-1024 (requires 11 bits)
	unsigned char head_page; //When pages wrap around, this tells you which is the first
};
typedef struct weather_settings weather_settings;

//Global member variables

//This is the location in RAM where samples are stored, the size is
//pretty limited because there's only 4K and these structs are 20 bytes 
//each.  
weath_meas measurements[N_RAM_SAMPLES];

//Used when compressing pages
weath_meas running_sum;

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
int ram_tail = 0;

void transfer_to_erom( );
void print_settings();
void copy_msmt(weath_meas *sink, _Erom unsigned short *src);
//Saves _s to ROM
void save_settings();
//Returns 1 if the memory address contains a valid record
int p_is_valid_record(_Erom unsigned short* ptr);
//supports histograms of any value, the offset describes the 
//offset within the struct in bytes

void iterate_erom_logs_i(ptItr function, int start_page, int start_record, int num_records);
void iterate_erom_logs_p(ptItr func, _Erom unsigned short* start, _Erom unsigned short* end);
int compression_cb(weath_meas *ms, unsigned int num);

void init_weather_log()
{
	//Later, we could load these from ROM
	_s.cur_page=0;
	_s.cur_word=0;
	_s.head_page=0;
}

void print_settings()
{
	printf(R"Current page: %d\nCurrent word: %d\nCurrent head: %d\n", _s.cur_page, _s.cur_word, _s.head_page);
}

unsigned int get_cur_erom_page()
{
	return (unsigned int)data_store[_s.cur_page];
}
unsigned int get_cur_erom_tail()
{
	return (unsigned int)&data_store[_s.cur_page][_s.cur_word];
}

int get_cur_ram_tail()
{
	return ram_tail;
}

void save_settings()
{
	
}

int at_page_end()
{
	return _s.cur_word >= USABLE_SHORTS_PER_PAGE;
}

int compression_cb(weath_meas *ms, unsigned int num)
{
	if(!is_valid_record(ms))
	{
		if(is_debug())
		{
			printf(R"Warning! Invalid record found in EROM.  Skipping during compression");
		}
		return 1;
	}
	running_sum.temp_f += ms->temp_f;
	running_sum.humid += ms->humid;
	running_sum.pressure += ms->pressure;
	running_sum.lux += ms->lux;
	//num - 1 because the callback sends nums from 1, rather than 0
	//num pages -1 because we're sumamrizing every 23rd record, since
	//there's no data in the 24th page
	if((num-1) % (NUM_PAGES-1) == 0)
	{
		running_sum.temp_f /= 23.0;
		running_sum.humid /= 23.0;
		running_sum.pressure  /= 23.0;
		running_sum.lux  /= 23.0;
	}
	return 1;
}

//Compressed contents of head_page through cur_page-1
//into cur_page
//the last EROM page stores asummary of all previous pages, then
//those pages are considered blank and can be overwritten
//then that page becomes the new head
//so the first time this happens will be when page 22 wraps
//at that time, 0-22 will be summariezed in page 23, and page
//23 will become the new head
void compress_pages()
{
	zero_struct(&running_sum);
	iterate_all_erom_logs(&compression_cb);
}


int next_erom_page()
{
	if(is_debug())
	{
		printf(R"Reached the end of page %d, moving on to %d\n", 
		_s.cur_page, (_s.cur_page+1) % NUM_PAGES);
	}
	_s.cur_word = 0;
	_s.cur_page = (_s.cur_page+1) % NUM_PAGES;
	
	//Look ahead one step in the circle buffer to see if this
	//is the last page before page head.  If it is, we will
	//compress all previous pages into this page, then move 
	//forward another page and start overwriting old data
	if(_s.head_page == ((_s.cur_page+1) % NUM_PAGES))
	{
		//This will do page_erase on cur_page,
		//fill it with summary data, and move cur_page forward
		//to head_page.  It will move cur_page back one...effectively
		//swapping cur_page and head page.  leapfrogging the cur-pointer
		//then remaining logic can continue, erasing and overriting the
		//new curpage
		//compress_pages();
	}
	page_erase(data_store[_s.cur_page]);
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
void write_buffer( )
{
	int i, j, max;
	unsigned short* ptr;

	max = PAGE_SIZE - STRUCT_SIZE;
	init_flash(get_clock());
	if(is_debug())
	{
		printf(R"Dumping to flash at address 0x%06lx\n", &data_store[_s.cur_page][_s.cur_word]);
	}
	//When cur_byte is 0 that means we're writing to a new page, so we need to erase it
	if(_s.cur_word == 0)
	{

		if(is_debug())
		{
			printf(R"Erasing flash page 0x%06lx\n", data_store[_s.cur_page]);
		}
		page_erase(data_store[_s.cur_page]);
	}
	page_unlock(data_store[_s.cur_page]);

	for(i=0; i<N_RAM_SAMPLES; i++)
	{
		if(!is_valid_record(&measurements[i]))
		{
			if(is_debug())
			{
				printf(R"Record %d is not valid, will not save in EROM...\n", i);
				print_measurement(&measurements[i], i);
			}
			continue;
		}
		ptr = (unsigned short*)&measurements[i];
		//copy a single record to a single spot...
		for(j=0; j<SHORTS_PER_SAMPLE; j++)
		{
			data_store[_s.cur_page][_s.cur_word++] = *(ptr+j);
			if(at_page_end())
			{
				lock_flash();
				next_erom_page();
				page_unlock(data_store[_s.cur_page]);
			}
		}
	}
	lock_flash();
	//increment deal with wrapping
	
	save_settings();
}

void put_measurement(weath_meas *measurement)
{
	if(measurement->station == 0x0000)
	{
		printf(R"Station is zero, something's not right\n");
	}
	measurements[ram_tail] = *measurement;
	ram_tail = (ram_tail+1);
	if(ram_tail == N_RAM_SAMPLES)
    {
		write_buffer();
        ram_tail = 0;
    }
}

int get_num_records()
{
	int num_full_pgs=0;
	if(_s.cur_page > _s.head_page)
	{
		num_full_pgs = _s.cur_page - _s.head_page;
	}
	//it was wrapped around
	if(_s.cur_page < _s.head_page) 
	{
		num_full_pgs = _s.cur_page + (NUM_PAGES - _s.head_page);
	}
	return num_full_pgs * SAMPLES_PER_EROM_PAGE + _s.cur_word/SHORTS_PER_SAMPLE;
}

void get_most_recent(weath_meas *dest)
{
	if(ram_tail > 0)
	{
		deep_copy_record(dest, &measurements[ram_tail]);
	}else
	{
		copy_msmt(dest, &data_store[_s.cur_page][_s.cur_word]);
	}
}

void get_most_recent_station(weath_meas *dest, unsigned short station)
{
	int i, j, j_start;
	for(i = ram_tail; i>= 0; i--)
	{
		if(measurements[i].station == station)
		{
			deep_copy_record(dest, &measurements[i]);
			return;
		}
	}
	for(i=_s.cur_page; i != _s.head_page; i--)
	{
		if(i < 0)
		{
			i = NUM_PAGES-1;
		}
		if(i == _s.cur_page)
		{
			j_start = _s.cur_word-SHORTS_PER_SAMPLE;
		}else
		{
			j_start = SHORTS_PER_SAMPLE * (SAMPLES_PER_EROM_PAGE-1);
		}
		for(j=j_start; j>=0; j-= SHORTS_PER_SAMPLE)
		{
			copy_msmt(dest, &data_store[i][j]);
			if(dest->station == station)
			{
				return;
			}
		}
	}
}

void iterate_all_logs(ptItr function)
{
	iterate_ram_logs(function);
	iterate_all_erom_logs(function);
}

void iterate_ram_logs(ptItr function)
{
	int i;
	if(is_debug())
	{
		printf(R"Iterating over RAM samples from 0 to %d\n", ram_tail);
	}
	for(i=0; i<ram_tail; i++)
	{
		if(is_debug())
		{
			printf(R"\ton sample %d\n", i);
		}
		function(&measurements[i], (unsigned int)&measurements[i]);
	}
}



int get_num_erom_words_help( _Erom unsigned short* p_start, _Erom unsigned short* p_end)
{
	unsigned int i_end, i_start, start_page_idx, last_page_idx;
	unsigned int sp_add, lp_add, sp_word, ep_word, num_shorts;

	i_end = (unsigned int)&data_store[_s.cur_page][_s.cur_word];
	i_start = (unsigned int)data_store[_s.head_page];
	
	start_page_idx = (i_start >> 11) - (EROM_START>>11);
	last_page_idx = (i_end>>11) - (EROM_START>>11);
	sp_add = (start_page_idx << 11) + EROM_START;
	lp_add = (last_page_idx << 11) + EROM_START;
	
	//divide by 2, not struct size, becuase the array stores
	//shorts, not records
	sp_word = (i_start-sp_add)/2;
	ep_word = (i_end-lp_add)/2;
	
	if(start_page_idx == last_page_idx)
	{
		num_shorts = (ep_word - sp_word);
	}else if(start_page_idx == last_page_idx-1)
	{
		num_shorts = (PAGE_SIZE-sp_word) + ep_word;
	}else if(start_page_idx < last_page_idx)
	{
		num_shorts = (last_page_idx - start_page_idx) -1;
		num_shorts *= PAGE_SIZE;
		num_shorts += (PAGE_SIZE-sp_word);
		num_shorts += ep_word;
	}else //it's wrapped around 
	{
		num_shorts = NUM_PAGES * PAGE_SIZE - (PAGE_SIZE-sp_word);
	}
		
	if(is_debug())
	{
		printf(R"\tstart_page_idx=%d, last_page_idx=%d, \nsp_word=%d, ep_word=%d, \nnum_shorts=%d\n", 
		start_page_idx, last_page_idx, sp_word, ep_word, num_shorts);
	}
	return num_shorts;
}

int get_num_erom_words()
{
	return get_num_erom_words_help(data_store[_s.head_page], &data_store[_s.cur_page][_s.cur_word]);
}

void iterate_all_erom_logs(ptItr function)
{
	if(is_debug())
	{
		printf(R"Iterating over all EROM, up to [%d][%d]\n", _s.cur_page, _s.cur_word);
		printf(R"theoretically this will go from 0x%06lx to 0x%06lx\n", 
		(unsigned int)data_store[_s.head_page],
		(unsigned int)&data_store[_s.cur_page][_s.cur_word]);
	}
	iterate_erom_logs_i(function, _s.head_page, 0, get_num_erom_words()/SHORTS_PER_SAMPLE);
}


void iterate_erom_logs_p(ptItr function, _Erom unsigned short* p_start, _Erom unsigned short* p_end)
{
	unsigned int i_start, start_page_idx, sp_add, sp_word;
	i_start = (unsigned int)p_start;
	if(i_start < 0x14000)
	{
		printf(R"Must specify an address beyond 0x14000\n");
		return;
	}
	if((unsigned int)p_end >0x1FFFF )
	{
		printf(R"End address can't be beyond 0x1FFFF\n");
		return;
	}
	

	
	start_page_idx = (i_start >> 11) - (EROM_START>>11);
	sp_add = (start_page_idx << 11) + EROM_START;
	
	//divide by 2, not struct size, becuase the array stores
	//shorts, not records
	sp_word = (i_start-sp_add)/2;

	iterate_erom_logs_i(function, start_page_idx, sp_word/SHORTS_PER_SAMPLE, get_num_erom_words_help(p_start, p_end)/SHORTS_PER_SAMPLE);
}

void iterate_erom_logs_i(ptItr function, int start_page, int start_record, int num_records)
{
	int num_records_first_page, num_records_last_page, num_pages, count=0, stop=0;
	int last_page, beyond_last_page, pg_offset, i, j, page_start, page_max,print_count=1;
	int page_count=0;
	weath_meas meas;
	
	if(is_debug())
	{
		printf(R"Called with start record %d, start page %d, \n\tand %d records\n", start_record, start_page, num_records);
	}
	if(num_records < 0)
	{
		printf(R"Error, can't iterate over %d records\n", num_records);
	}

	if(start_record + num_records < SAMPLES_PER_EROM_PAGE)
	{
		num_records_first_page = num_records;
		num_pages = 1;
		num_records_last_page = num_records;//this is unenecssary
	}else
	{
		num_records_first_page = SAMPLES_PER_EROM_PAGE - start_record;
		num_pages = (start_record + num_records) / SAMPLES_PER_EROM_PAGE + 1; //this gives us the number of pages we'll need to iterate through	
		num_records_last_page = (num_records-num_records_first_page) % SAMPLES_PER_EROM_PAGE;		
	}
	if(num_pages > NUM_PAGES)
	{
		printf(R"Warning, asking to iterate beyond data");
	}
	
	last_page = (start_page + num_pages-1) % NUM_PAGES;
	beyond_last_page = (last_page+1) % NUM_PAGES; //make tail page point to the byond
	
	if(is_debug())
	{
		printf(R"Num records first page: %d\n", num_records_first_page);
		printf(R"Num pages: %d\n", num_pages);
		printf(R"Num records last page: %d\n", num_records_last_page);
		printf(R"Beyond last page is %d\n", beyond_last_page);
		printf(R"Last page is %d\n", last_page);
		print_settings();
	}
	page_count=0;
	for(i=start_page; page_count<num_pages && !stop; i=(i+1) % NUM_PAGES)
	{
		page_count++;
		if(is_debug())
		{
			printf(R"\t...on page %d\n", i);
			printf(R"Loading records from...\n");
		}

		if(i == start_page)
		{
			page_start = (start_record*SHORTS_PER_SAMPLE);
			page_max = page_start + (num_records_first_page*SHORTS_PER_SAMPLE);
		}else if(i == last_page)
		{
			//this can't overlap if start_page == last_page becuase
			//of the if/else structure
			page_start = 0;
			page_max = (num_records_last_page*SHORTS_PER_SAMPLE);
		}else
		{
			page_start = 0;
			page_max = PAGE_SIZE;
		}
		if(is_debug())
		{
			printf(R"For page %d...\tcomputed page_start=%d, page_max= %d (%d records)\n", i, page_start, page_max, (page_max-page_start)/SHORTS_PER_SAMPLE);
		}

		for(j=page_start; j <= (page_max-SHORTS_PER_SAMPLE) &&!stop; j+=SHORTS_PER_SAMPLE)
		{
			if((j % SHORTS_PER_SAMPLE) != 0)
			{
				printf(R"Warning, j=%d which does not evenly divide a record\n", j);
			}
			//payload
			copy_msmt(&meas, &data_store[i][j]);
			stop = !function(&meas, (unsigned int)&data_store[i][j]);
			
			if(is_debug())
			{
				printf(R"[%d][% 4d], ", i, j);
				if(print_count== 5)
				{
					printf(R"\n");
					print_count=0;
				}
				print_count++;
			}
		}
		if(is_debug())
		{
			print_count=1;
			printf(R"\n");
		}
	}
}

void dump_from_address(_Erom unsigned short* ptr, int records)
{
	unsigned int start_page_idx, i_start, sp_add, sp_record;
	i_start = (unsigned int)ptr;
	
	start_page_idx = (i_start >> 11) - (EROM_START>>11);
	sp_add = (start_page_idx << 11) + EROM_START;
	sp_record = (i_start-sp_add)/STRUCT_SIZE;
	
	iterate_erom_logs_i(&print_measurement, start_page_idx, sp_record, records);
	printf(R"-----------------------\n");
}

void dump_all_logs()
{
	iterate_ram_logs(&print_measurement);
	iterate_all_erom_logs(&print_measurement);
}

void copy_msmt(weath_meas *sink, _Erom unsigned short *src)
{
	int i;
	unsigned short * s_ptr;
	s_ptr = (unsigned short*)sink;
	for(i=0; i<SHORTS_PER_SAMPLE; i++) // div 2 because it's a pointer to shorts
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

