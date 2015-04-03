#include "TMP36.h"

/*
 * Reads the first 10 bits as an analog
 * value scaled between 0 and 3.15 V.
 *
 * From the spec, TMP 36 scales at 10 mV / degree C
 * with an offset of .5 V.
*/
float TMP_36_read_C(float vout)
{
	return (vout/.01) - 50.0;
}

float TMP_36_read_F(float vout)
{
	return (1.8 * TMP_36_read_C(vout)) + 32.0;
}
