#include <stdio.h>
#include <stdlib.h>

#define NUM_PAGES 24
#define SHORTS_PER_SAMPLE 10
#define SAMPLES_PER_EROM_PAGE 102
#define PAGE_SIZE 1024


#define EROM_START 0x14000

void convert_ptrs(unsigned int start, unsigned int end)
{
	unsigned int start_page, last_page, sp_record, ep_record, num_records;
	unsigned int sp_add, lp_add;
	start_page = (start >> 11) - (EROM_START>>11);
	last_page = (end>>11) - (EROM_START>>11);
	sp_add = (start_page << 11) + EROM_START;
	lp_add = (last_page << 11) + EROM_START;
	
	sp_record = (start-sp_add)/20;
	ep_record = (end-lp_add)/20;
	
	if(start_page == last_page)
	{
		num_records = ep_record - sp_record;
	}else if(start_page == last_page-1)
	{
		num_records = (SAMPLES_PER_EROM_PAGE-sp_record) + ep_record;
	}else
	{
		num_records = ((last_page - start_page) -2) * SAMPLES_PER_EROM_PAGE + 
			(SAMPLES_PER_EROM_PAGE-sp_record) + ep_record;
	}
	printf("From 0x%06x to 0x%06x, start page is %d, last page is %d\n",
		start, end, start_page, last_page);
	printf("Start record index is [%d][%d], last record index is [%d][%d], num records is %d\n", start_page, sp_record, last_page, ep_record, num_records);
}

int main(int argc, char** argv)
{
	int s, e;

	if(argc == 3)
	{
		sscanf(argv[1], "%x", &s);
		sscanf(argv[2], "%x", &e);
		convert_ptrs(s, e);
	}else
	{
		printf("a.out <start record> <start page> <num records>\n");
	}
  return 1;
}
