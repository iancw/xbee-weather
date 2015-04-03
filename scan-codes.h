#ifndef __SCAN_CODES_H__
#define __SCAN_CODES_H__

enum NonPrinting{L_CTRL, R_CTRL, L_ALT, R_ALT, L_SHFT, R_SHFT,
	CAPS, ESC, ENTER,F1,F2,F3,F4,F5,F6,F7,F8,F9,F10,F11,F12,SCROLL,
	NUM,L_GUI,R_GUI,APPS,INS,HOME,PG_UP,DEL,END,PG_DN,U_ARR,L_ARR,
	D_ARR,R_ARR,PWR,SLP,WAKE,NEXT,PREV,STOP,PL_PAS,MUTE,VOL_UP,VOL_DWN,
	MEDIA,EMAIL,CALC,MY_COM,WWW_SRCH,WWW_HOME,WWW_BACK,WWW_FWD,WWW_STOP,
	WWW_REF,WWW_FAV};

typedef enum NonPrinting NonPrinting;
/*
 * Converts a scan code to an ascii character
 */
char scan_to_char(unsigned char scan_code, int shift);

/*
 * Converts a special scan code to a NonPrinting value,
 * pass 1 for extended if this scan_code is an extended code
 * (that is, if it was preceded by a code that returned 1 for
 * is_extended_code).
 */
NonPrinting np_to_val(unsigned char special_code, int extended);

/*
 * Returns 1 if scan_code is the scan release code (0xfe)
 */
int is_released_code(unsigned char scan_code);

/*
 * Returns 1 if scan code is a special scan code (0xe0)
 */
int is_extended_code(unsigned char scan_code);

/*
 * Returns true if the scan_code is a non printing character,
 * such as CTL, ALT, SHIFT, ESC, etc.
 */
int is_non_printing(unsigned char scan_code);

#endif