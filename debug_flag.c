#include "debug_flag.h"

int _g_debug=0;

void toggle_debug()
{
	_g_debug = !_g_debug;
}
void set_debug(int val)
{
	_g_debug = val;
}
char is_debug()
{
	return _g_debug;
}