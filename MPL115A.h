#ifndef __MPL115A_H__
#define __MPL115A_H__

void init_MPL115A();

float MPL115A_read_pressure();

void MPL115A_read_coefficients();

#endif