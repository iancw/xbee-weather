#include "scan-codes.h"
#include <zneo.h>

#define NUM_CODES 51
#define NON_PRINT_CODES 21
#define NUM_EXT_CODES 36

rom unsigned char key_code[NUM_CODES] = {
	0x1c,0x32,0x21,0x23,0x24,0x2b,0x34,0x33,0x43,0x3b,0x42,0x4b,0x3a,
	0x31,0x44,0x4d,0x15,0x2d,0x1b,0x2c,0x3c,0x2a,0x1d,0x22,0x35,0x1a,
	0x45,0x16,0x1e,0x26,0x25,0x2e,0x36,0x3d,0x3e,0x46,0x0e,0x4e,0x55,
	0x5d,0x66,0x29,0x0d,0x54,0x5b,0x4c,0x52,0x41,0x49,0x4a,0x5a
    };

rom char key_value[NUM_CODES] = {
	'a','b','c','d','e','f','g','h','i','j','k','l','m',
	'n','o','p','q','r','s','t','u','v','w','x','y','z',
	'0','1','2','3','4','5','6','7','8','9','`','-','=',
	'\\','\b',' ','\t','[',']',';','\'',',','.','/','\n'
};

rom char upper_value[NUM_CODES] = {
	'A','B','C','D','E','F','G','H','I','J','K','L','M',
	'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
	')','!','@','#','$','%','^','&','*','(','~','_','+',
	'|','\b',' ','\t','{','}',':','\"','<','>','?','\n'
};

rom unsigned char non_printing_codes[NON_PRINT_CODES] = {
	0x58,0x12,0x14,0x11,0x59,0x5a,0x76,0x05,0x06,0x04,0x0C,0x03,0x0b,0x83,
	0x0a,0x01,0x09,0x78,0x07,0x7e,0x77};

rom NonPrinting non_printing_vals[NON_PRINT_CODES]={
	CAPS,L_SHFT,L_CTRL,L_ALT,R_SHFT,ENTER,ESC,F1,F2,F3,F4,F5,F6,F7,
	F8,F9,F10,F11,F12,SCROLL,NUM
};

rom unsigned char extended_codes[NUM_EXT_CODES]={
	0x1f,0x14,0x27,0x11,0x2f,0x70,0x6c,0x7d,0x71,0x69,0x7a,0x75,0x6b,0x72,0x74,
	0x37,0x3f,0x5e,0x4d,0x15,0x3b,0x34,0x23,0x32,0x21,0x50,0x48,0x2b,0x40,
	0x10,0x3a,0x38,0x30,0x28,0x20,0x18
};
rom NonPrinting extended_vals[NUM_EXT_CODES]={
	L_GUI,R_CTRL,R_GUI,R_ALT,APPS,INS,HOME,PG_UP,DEL,END,PG_DN,U_ARR,L_ARR,D_ARR,R_ARR,
	PWR,SLP,WAKE,NEXT,PREV,STOP,PL_PAS,MUTE,VOL_UP,VOL_DWN,MEDIA,EMAIL,CALC,MY_COM,
	WWW_SRCH,WWW_HOME,WWW_BACK,WWW_FWD,WWW_STOP,WWW_REF,WWW_FAV
};

char scan_to_char(unsigned char scan_code, int shift)
{
	int i;
	for(i=0; i<NUM_CODES; i++)
	{
		if(key_code[i] == scan_code)
		{
			if(shift)
			{
				return upper_value[i];
			}else {
				return key_value[i];
			}

		}
	}
	return '\0';
}

NonPrinting ext_scan_to_val(unsigned char scan_code)
{
	int i;
	for(i=0; i<NUM_EXT_CODES; i++)
	{
		if(extended_codes[i] == scan_code)
		{
			return extended_vals[i];
		}
	}
	return '\0';
}

NonPrinting np_to_val(unsigned char scan_code, int extended)
{
    int i;
	if(extended)
	{
		return ext_scan_to_val(scan_code);
	}
	for(i=0; i<NON_PRINT_CODES; i++)
	{
		if(non_printing_codes[i] == scan_code)
		{
			return non_printing_vals[i];
		}
	}
	return '\0';
}

/*
 * Returns 1 if scan_code is the scan release code (0xfe)
 */
int is_released_code(unsigned char scan_code)
{
	return scan_code == 0xf0;
}

/*
 * Returns 1 if scan code is a special scan code (0xe0)
 */
int is_extended_code(unsigned char scan_code)
{
	return scan_code == 0xe0 || scan_code == 0xe1;
}

int is_non_printing(unsigned char scan_code)
{
	return scan_to_char(scan_code, 0) == '\0';
}
