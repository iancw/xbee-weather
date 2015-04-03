#ifndef __WEATHER_LOG_H__
#define __WEATHER_LOG_H__

#define CENTRAL_STATION 0
#define REMOTE_STATION_1 1
#define REMOTE_STATION_2 2

struct weath_meas
{
	float temp_f;
	float lux;
	float pressure;
	float humid;
	unsigned timestamp;
	unsigned short station;
};
typedef struct weath_meas weath_meas;

/**
 * Return 1 to continue, 0 to stop iteration
 */
typedef int(*ptItr)(weath_meas* ms, unsigned int address);

void init_weather_log();
void put_measurement(weath_meas *measurement);
void dump_from_address(_Erom unsigned short* ptr, int records);
void dump_all_logs();

void iterate_all_logs(ptItr func);
void iterate_all_erom_logs(ptItr func);
void iterate_ram_logs(ptItr func);
int get_num_records();

unsigned int get_cur_erom_page();
unsigned int get_cur_erom_tail();
void get_most_recent(weath_meas *dest);
void get_most_recent_station(weath_meas *dest, unsigned short station);
int get_cur_ram_tail();

#endif