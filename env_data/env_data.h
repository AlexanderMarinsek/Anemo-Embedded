#ifndef ENV_DATA_H
#define ENV_DATA_H

#include "log.h"

#include <stdint.h>
#include <stddef.h>				// size_t


#define ENV_DATA_DISCONNECTED		(-1)


/* Json buffer format.
 * 	air_pressure : _xxxxx [hPa * 10e1]
 *  air_temp : Sxxx [deg.C * 10e1]
 *	rel_humidity : _xxx [% * 10e1]
 */
#define ENV_DATA_JSON_FORMAT	""\
	"\"air_pressure\":%d,"\
	"\"air_temp\":%d,"\
	"\"rel_humidity\":%d"
//#define ENV_DATA_JSON_FORMAT	""\
//	"\"air_pressure\":%05d,"\
//	"\"air_temp\":%04d,"\
//	"\"rel_humidity\":%03d"

#define ENV_DATA_BUFFER_LEN					64

/* Data (measurements, buffer...). */
typedef struct {
	int air_pressure_sum;
	int air_temp_sum;
	int rel_humidity_sum;
	int average_counter;
} Intermediate_env_data;

/* Intermediate data (sum of measurements, avg. counter...). */
typedef struct {
	int air_pressure;
	int air_temp;
	int rel_humidity;
	char buffer[ENV_DATA_BUFFER_LEN];
} Env_data;


/* Initiate module.
 *  p1: pointer to where the module's buffer lenght will be written
 * return:
 *  0 on success, -1 on error
 */
int8_t init_env_data (size_t *buffer_len);

/* Read environmental data with(!) blocking further execution.
 * return:
 *  0: Finished
 *  -1: error
 */
int8_t read_intermediate_env_data(void);

/* Format measurements to JSON and write to internal buffer.
 * return:
 *  pointer to array's (string's) start address
 */
char *get_avg_json_env_data(void);


#endif
