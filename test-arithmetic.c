#include <stdio.h>

int main()
{
	int spage, epage, len=3, max_byte=0, page_max;
	unsigned short* ptr = 0x14000 + 0x7ff;
	
	unsigned int page;
	page = (unsigned int)ptr;
	
	//Shifting 11 right divides by 2048 ...
	printf("ptr=0x%06x (%u) / ptr>>11 = 0x%06x (%u) / ptr << 3 = 0x%06x (%u)\n", 
		page, page,
		(page>>11), (page>>11), 
		((page>>11)<<3),		((page>>11)<<3));

	printf("e_page=0x%06x (%u) / ptr>>11 = 0x%06x (%u) / ptr << 3 = 0x%06x (%u)\n", 
		(page+len), (page+len),
		((page+len)>>11), ((page+len)>>11), 
		(((page+len)>>11)<<3),(((page+len)>>11)<<3));

		
	spage = (page>>11) - 0x28;
	epage = ((page+len) >> 11) -0x28;
	max_byte = ((page+len) & 0x7FF);
	printf("ptr/2048 = 0x%06x (%u)\n", page/2048, page/2048);
	
	printf("spage = %d / 0x%06x\n", spage, spage);
	printf("epage = %d / 0x%06x\n", epage, epage);
	printf("max byte = %d / 0x%06x\n", max_byte,max_byte);
	
	page_max = ((page+len) & 0x7FF) / 20;
	printf("Added page is: %d / %06x\n", (page+len),  (page+len));
	printf("Masked page is: %d / %06x\n", ((page+len)&0x7ff),  ((page+len)&0x7ff));
	printf("Max page: %d\n", page_max);
	
return 1;
}
