#include <stdio.h>
#include <stdlib.h>

#define NUM_PAGES 24
#define SHORTS_PER_SAMPLE 10
#define SAMPLES_PER_EROM_PAGE 102
#define PAGE_SIZE 1024

int call_count=0;

int is_debug()
{
	return 1;
}

void iterate(int start_record, int start_page, int num_records)
{
	int num_records_first_page, num_records_last_page, num_pages;
	int last_page, beyond_last_page, pg_offset, i, j, page_start, page_max,print_count=1;
	
	printf("Called with start record %d, start page %d, and %d records\n", start_record, start_page, num_records);


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
		printf("Warning, asking to iterate beyond data");
	}
	printf("Num records first page: %d\n", num_records_first_page);
	printf("Num pages: %d\n", num_pages);
	printf("Num records last page: %d\n", num_records_last_page);
	
	last_page = (start_page + num_pages) % NUM_PAGES -1;
	printf("Last page is %d\n", last_page);
	beyond_last_page = (last_page+1) % NUM_PAGES; //make tail page point to the byond
	printf("Beyond last page is %d\n", beyond_last_page);

	for(i=start_page; i!= beyond_last_page; i=(i+1) % NUM_PAGES)
	{
		if(is_debug())
		{
			printf("\t...on page %d\n", i);
		}
		printf("Loading records from...\n");
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
		printf("Computed page_star=%d, page_max= %d (%d records)\n", page_start, page_max, (page_max-page_start)/10);

		for(j=page_start; j <= (page_max-SHORTS_PER_SAMPLE); j+=SHORTS_PER_SAMPLE)
		{
			call_count++;
			printf("[%d][% 4d], ", i, j);
			if(print_count== 5)
			{
				printf("\n");
				print_count=0;
			}
			print_count++;
		}
		print_count=1;
		printf("\n");
	}
}
#define EROM_START 0x14000

void convert_ptrs(unsigned int start, unsigned int end)
{
	unsigned int start_page, last_page;
	start_page = (start >> 11) - (EROM_START>>11);
	last_page = (end>>11) - (EROM_START>>11);
	
	printf("From 0x%06x to 0x%06x")
}

int main(int argc, char** argv)
{
	if(argc == 4)
	{
		call_count=0;
		iterate(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
		printf("Total of %d records were iterated over\n", call_count);
	}else
	{
		printf("a.out <start record> <start page> <num records>\n");
	}
  return 1;
}
