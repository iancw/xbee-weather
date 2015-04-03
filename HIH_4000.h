#ifndef __HIH_4000_H__
#define __HIH_4000_H__

float HIH_4000_read(float voltage)
{
	//This came on the package that this particular sensor
	//was calibrated with
	return (voltage - 0.850) / 0.031;
}

#endif