#include "wind_data.h"
#include "../anemo_davis/anemo_davis.h"
#include "../pin_settings.h"

#include "periph/gpio.h"

#include <log.h>
#include <string.h>			// For 'memset'
#include <math.h>
#include <stdint.h>
#include <stddef.h>				// size_t

#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG (0)
#endif
#include "debug.h"


/* Statics */
/* Intermediate data (sum of measurements, avg. counter...). */
static Intermediate_wind_data _intermediate_wind_data;
/* Data (measurements, buffer...). */
static Wind_data _wind_data;

/* internal error variable used to set values to 0 on error */
static int8_t _error_detected;

/* Prototypes *****************************************************************/
static void _calc_avg_wind_data (void);

static void _intermediate_update_dir(int wind_direction);
static void _intermediate_update_gust(int wind_speed);
static int _calc_avg_wind_speed (void);
static int _calc_avg_wind_dir_10e1 (void);

static void _reset_intermediate_data (void);
static void _reset_avg_data (void);

/* Functions ******************************************************************/

/* Initiate module. */
int8_t init_wind_data (uint16_t north_offset_10e1, size_t *buffer_len) {

	/* Reset state variables */
	_error_detected = 0;

	int8_t anemo_init =
			anemo_davis_init (
				north_offset_10e1,
				ANEMO_DAVIS_MUX_OUT,
				ANEMO_DAVIS_MUX_C,
				ANEMO_DAVIS_MUX_B,
				ANEMO_DAVIS_MUX_A,
				ANEMO_DAVIS_COUNTER_RST,
				ANEMO_DAVIS_COUNTER_N_EN,
				ANEMO_DAVIS_ADC_LINE);

	_reset_intermediate_data();
	_reset_avg_data();

	*buffer_len = WIND_DATA_BUFFER_LEN;

	if (anemo_init != 0) {
		LOG_ERROR("Failed: anemo_init\n");
		_error_detected = 1;
		return -1;
	}

	return 0;
}

/* Read environmental data with(!) blocking further execution. */
int8_t read_intermediate_wind_data(void) {

	if (_error_detected) {
		_reset_intermediate_data();
		return -1;
	}

	int wind_speed = anemo_davis_calc_speed_ms_10e2();
	int wind_direction = anemo_get_wind_direction_10e1();

	if (wind_speed == -1 || wind_direction == -1) {
		LOG_ERROR("Failed: "
				"anemo_davis_calc_speed_ms_10e2 || "
				"anemo_get_wind_direction_10e1\n");
		_error_detected = 1;
		return -1;
	}

	DEBUG("wind_speed: %d, wind_direction: %d\n",
			wind_speed, wind_direction);

	_intermediate_wind_data.wind_speed_sum += wind_speed;
	_intermediate_update_dir(wind_direction);
	_intermediate_update_gust(wind_speed);
	_intermediate_wind_data.average_counter++;

	return 0;
}

/* Format measurements to JSON and write to internal buffer. */
char *get_avg_json_wind_data(void) {

	_calc_avg_wind_data();
	_reset_intermediate_data();

	snprintf(_wind_data.buffer, WIND_DATA_BUFFER_LEN, WIND_DATA_JSON_FORMAT,
			_wind_data.wind_speed,
			_wind_data.wind_direction,
			_wind_data.wind_gust_speed,
			_wind_data.wind_gust_peak);

	DEBUG("%s\n",_wind_data.buffer);

	return _wind_data.buffer;
}


/* Helpers ********************************************************************/

static void _calc_avg_wind_data(void) {

	if (_error_detected) {
		/* Set values to zero */
		_reset_avg_data();
		return;
	}

	/* Calculate wind speed without cutting away decimals (round them) */
	_wind_data.wind_speed = _calc_avg_wind_speed();
	_wind_data.wind_direction = _calc_avg_wind_dir_10e1();
	_wind_data.wind_gust_speed = _intermediate_wind_data.max_wind_gust_speed;
	_wind_data.wind_gust_peak = _intermediate_wind_data.wind_gust_peak;

	DEBUG(	"wind_speed: %d, wind_direction: %d, "
			"wind_gust_speed: %d, wind_gust_peak: %d\n",
			_wind_data.wind_speed, _wind_data.wind_direction,
			_wind_data.wind_gust_speed, _wind_data.wind_gust_peak);
}

/* On intermediate sampling time, add direction to array. */
static void _intermediate_update_dir(int wind_direction) {
	int dir_idx = ((wind_direction + WIND_DIR_SECTOR_OFFSET_10E1) % 3600) \
			/ WIND_DIR_SECTOR_WIDTH_10E1;
	_intermediate_wind_data.wind_direction_sum[dir_idx] += 1;

	DEBUG("dir_idx: %d, wind_direction: %d, WIND_DIR_SECTOR_OFFSET_10E1: %d,\
		WIND_DIR_SECTOR_WIDTH_10E1: %d\n",
		dir_idx, wind_direction, WIND_DIR_SECTOR_OFFSET_10E1, WIND_DIR_SECTOR_WIDTH_10E1);
}

/* On intermediate sampling time, add gust speed, if greater than preceeding */
static void _intermediate_update_gust(int wind_speed) {
	/* Update gust speed */
	if (wind_speed - _intermediate_wind_data.max_wind_gust_speed > 0) {
		/* Update gust peak */
		/*if (wind_speed_ms_10e2 - _intermediate_wind_data.max_wind_gust_speed
				> GUST_PEAK_TRESHOLD) {
			_intermediate_wind_data.wind_gust_peak = 1;
		}*/
		_intermediate_wind_data.max_wind_gust_speed = wind_speed;
	}
}

/* */
static int _calc_avg_wind_speed (void) {
	float f_wind_speed = (float)_intermediate_wind_data.wind_speed_sum
				/ _intermediate_wind_data.average_counter;
	return (int)roundf(f_wind_speed);
}

/* */
static int _calc_avg_wind_dir_10e1 (void) {
	int max_wind_dir_idx = 0;
	int max_wind_dir_count = 0;

	int i;
	for (i=0; i<WIND_DIRECTION_RESOLUTION; i++) {
		if (_intermediate_wind_data.wind_direction_sum[i] > max_wind_dir_count) {
			max_wind_dir_count = _intermediate_wind_data.wind_direction_sum[i];
			max_wind_dir_idx = i;
		}
		/* Clear after assessing */
		_intermediate_wind_data.wind_direction_sum[i] = 0;
	}

	/* Round to one decimal and return as int */
	float f_avg_wind_dir = max_wind_dir_idx * WIND_DIR_SECTOR_WIDTH;

	return (int)roundf(f_avg_wind_dir * 10);
}

/* (re)Set avg structure values to 0. */
static void _reset_intermediate_data (void) {
	memset (&_intermediate_wind_data, 0, sizeof(_intermediate_wind_data));
}

/* (re)Set avg structure values to 0. */
static void _reset_avg_data (void) {
	//memset (&_wind_data, 0, sizeof(_wind_data));
	_wind_data.wind_speed = 0;
	_wind_data.wind_direction = 0;
	_wind_data.wind_gust_speed = 0;
	_wind_data.wind_gust_peak = 0;
}

