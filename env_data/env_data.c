#include "env_data.h"

#include "bmx280_params.h"
#include "bmx280.h"

#include "log.h"

#include <math.h>				// roundf()
#include <stddef.h>				// size_t
#include <stdint.h>


#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG (0)
#endif
#include "debug.h"


/* Intermediate data (sum of measurements, avg. counter...). */
static Intermediate_env_data _intermediate_env_data;
/* Data (measurements, buffer...). */
static Env_data _env_data;

/* Shared BME280 device. */
static bmx280_t _dev_bme;

/* internal error variable used to set values to 0 on error */
static int8_t _error_detected;


/* Prototypes *****************************************************************/

static void _calc_avg_env_data(void);

static int _get_rel_humidity_rh_10e1 (void);
static int _get_air_temp_c_10e1 (void);
static int _get_air_pressure_hpa_10e1 (void);

static int16_t _is_temperature_valid (int temperature);

static void _reset_intermediate_data (void);
static void _reset_avg_data (void);


/* Functions ******************************************************************/

/* Initiate Device. */
int8_t init_env_data (size_t *buffer_len) {

	/* Reset state variables */
	_error_detected = 0;

	*buffer_len = ENV_DATA_BUFFER_LEN;

	if (bmx280_init(&_dev_bme, &bmx280_params[0]) != 0) {
		LOG_ERROR("Failed: bmx280_init\n");
		_error_detected = 1;
		return -1;
	}

	/* Reset intermediate values */
	_reset_intermediate_data();

	return 0;
}


/* Read environmental data with(!) blocking further execution. */
int8_t read_intermediate_env_data(void) {

	if (_error_detected) {
		_reset_intermediate_data();
		return -1;
	}

	/* Read temperature first!
	 * BMX280 module triggers measurement only when reading temperature
	 */

	/* Get air temperature in degrees Celsius * 10e1. */
	int air_temp = _get_air_temp_c_10e1();

	/* Catch device disconnected error (embedded in temperature measurement) */
	if (_is_temperature_valid(air_temp) != 0) {
		_error_detected = 1;
		return ENV_DATA_DISCONNECTED;
	}

	_intermediate_env_data.air_temp_sum += air_temp;
	/* Get air pressure in hPa * 10e1. */
	int air_pressure = _get_air_pressure_hpa_10e1();
	_intermediate_env_data.air_pressure_sum += air_pressure;
	/* Get relative humidity in % * 10e1. */
	int rel_humidity = _get_rel_humidity_rh_10e1();
	_intermediate_env_data.rel_humidity_sum += (int)rel_humidity;

	_intermediate_env_data.average_counter++;

	DEBUG(	"Pressure [hPa]: %d.%d "
			"Temperature [°C]: %d.%d "
			"Humidity [%%]: %d.%d\n",
			air_pressure / 10, air_pressure % 10,
			air_temp / 10, air_temp % 10,
			rel_humidity / 10, rel_humidity % 10);

	return 0;
}


/* Format measurements to JSON and write to internal buffer. */
char *get_avg_json_env_data(void) {

	/* Calculate average values (all zero on error) */
	_calc_avg_env_data();

	snprintf(_env_data.buffer, ENV_DATA_BUFFER_LEN, ENV_DATA_JSON_FORMAT,
			_env_data.air_pressure,
			_env_data.air_temp,
			_env_data.rel_humidity);

	DEBUG("%s\n", _env_data.buffer);

	return _env_data.buffer;
}


/* Helpers ********************************************************************/

/* Calculate average values and save to static structure.
 * All values are set to zero when module error is detected.
 */
static void _calc_avg_env_data(void) {

	if (_error_detected) {
		/* Set values to zero */
		_reset_avg_data();
		return;
	}

	int average_counter = _intermediate_env_data.average_counter;

	float f_air_pressure =
			(float)_intermediate_env_data.air_pressure_sum / average_counter;
	_env_data.air_pressure = (int)roundf(f_air_pressure);

	float f_air_temp =
			(float)_intermediate_env_data.air_temp_sum / average_counter;
	_env_data.air_temp = (int)roundf(f_air_temp);

	float f_rel_humidity =
			(float)_intermediate_env_data.rel_humidity_sum / average_counter;
	_env_data.rel_humidity = (int)roundf(f_rel_humidity);

	DEBUG(	"[hPa]: %d.%d, "
			"[°C]: %d.%d, "
			"[%%]: %d.%d, "
			"Avg. c.: %d\n",
			_env_data.air_pressure / 10, _env_data.air_pressure % 10,
			_env_data.air_temp / 10, _env_data.air_temp % 10,
			_env_data.rel_humidity / 10, _env_data.rel_humidity % 10,
			average_counter);

	/* Reset intermediate values */
	_reset_intermediate_data();
}


/* Get air pressure in hPa * 10e1.
 * return: air pressure
 */
static int _get_air_pressure_hpa_10e1 (void) {
	/* Get pressure in Pa */
	volatile uint32_t air_pressure = bmx280_read_pressure(&_dev_bme);
	/* Transform pressure to hPa 10e1 */
	float f_air_pressure = air_pressure / 10.0;
	return (int)roundf(f_air_pressure);
}


/* Get air temperature in degrees Celsius * 10e1.
 * return: air temperature
 */
static int _get_air_temp_c_10e1 (void) {
	/* Get temperature in centi degrees Celsius */
	int16_t temperature = bmx280_read_temperature(&_dev_bme);

	/* Catch device disconnected error and return */
	if (_is_temperature_valid(temperature) != 0) {
		return _is_temperature_valid(temperature);
	}

	/* Transform temperature to dgrees Celsius 10e1 */
	float f_temperature = temperature / 10.0;
	return (int)roundf(f_temperature);
}


/* Get relative humidity in % * 10e1.
 * return: relative humidity
 */
static int _get_rel_humidity_rh_10e1 (void) {
	/* Get pressure in %rH */
	 uint16_t humidity = bme280_read_humidity(&_dev_bme);
	/* Transform humidity to % 10e1 */
	float f_humidity = humidity / 10.0;
	return (int)roundf(f_humidity);
}


/* Catch device disconnected error.
 */
static int16_t _is_temperature_valid (int temperature) {
	if (temperature == INT16_MIN) {
		return INT16_MIN;
	}
	return 0;
}


/* (re)Set intermediate structure values to 0.
 */
static void _reset_intermediate_data (void) {
	_intermediate_env_data.air_pressure_sum = 0;
	_intermediate_env_data.air_temp_sum = 0;
	_intermediate_env_data.rel_humidity_sum = 0;
	_intermediate_env_data.average_counter = 0;
}

/* (re)Set average structure values to 0.
 */
static void _reset_avg_data (void) {
	_env_data.air_pressure = 0;
	_env_data.air_temp = 0;
	_env_data.rel_humidity = 0;
}
