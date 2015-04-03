#ifndef __PDV_8001_H__
#define __PDV_8001_H__

//Dark resistance (after 10 sec at 10 lux at 2856 degs K): 0.2 M Ohms
//Illuminated resistance (10 lux @ 2856 degs K):  3 - 11 K Ohms

/*
 * This library requires that the PDV-P8001 be connected
 * in a voltage divider circuit with a 21.1 K Ohm resistor to 3.15 V,
 * and an ADC with VREF as 3.15 V.
 * 
 *            ^ 3.293 V (VCC)
 *            \
 *            /
 *            \
 *  Sample -----
 *             \
 *             / (PDV P8001)
 *             \
 *             V
 *
 */

/*
 * Voltage is the voltage as read from an ADC
 * Divider is the other resistor in the voltage divider circuit
 */
float PDV_8001_compute_lux(float vout);

/*
 * Read from analog pin 0 PB0
 */
float PDV_8001_read_ana0();

#endif