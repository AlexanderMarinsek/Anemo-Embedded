#ifndef ANEMOMETER_H
#define ANEMOMETER_H

#include <stdint.h>
#include <stddef.h>				// size_t

#define WIND_DIRECTION_RESOLUTION		16
#define WIND_DIR_SECTOR_WIDTH			(360.0 / WIND_DIRECTION_RESOLUTION)
#define WIND_DIR_SECTOR_OFFSET			(WIND_DIR_SECTOR_WIDTH / 2)
#define WIND_DIR_SECTOR_WIDTH_10E1		(3600 / WIND_DIRECTION_RESOLUTION)
#define WIND_DIR_SECTOR_OFFSET_10E1		(int)(WIND_DIR_SECTOR_WIDTH_10E1 / 2.0)

#define WIND_DATA_BUFFER_LEN		128

/* JSON format doesn't support zero padding at beginning. Either include only
 * number, or wrap in quotes (0012 -> 12, or "0012") to avoid back end errors.
 */
#define WIND_DATA_JSON_FORMAT		""\
	"\"wind_speed\":%d,"\
	"\"wind_direction\":%d,"\
	"\"wind_gust_speed\":%d,"\
	"\"wind_gust_peak\":%d"
	/*"\"wind_speed\":%04d,"\
	"\"wind_direction\":%04d,"\
	"\"wind_gust_speed\":%04d,"\
	"\"wind_gust_peak\":%01d"*/
	/*"\"w_speed\":%04d,"\
	"\"w_dir\":%04d,"\
	"\"w_gust_max\":%04d,"\
	"\"w_gust_peak\":%01d"*/


typedef struct {
	int wind_speed;
	int wind_direction;
	int wind_gust_speed;
	int wind_gust_peak;
	char buffer[WIND_DATA_BUFFER_LEN];
} Wind_data;

typedef struct {
	int wind_speed_sum;
	int wind_direction_sum [WIND_DIRECTION_RESOLUTION];
	int max_wind_gust_speed;
	int wind_gust_peak;
	int average_counter;
} Intermediate_wind_data;


/* Initiate module.
 *  p1: offset from north in degrees * 10
 *  p2: pointer to where the module's buffer lenght will be written
 * return:
 *  0 on success, -1 on error
 */
int8_t init_wind_data (uint16_t north_offset_10e1, size_t *buffer_len);

/* Read environmental data with (minor, [us]) blocking further execution.
 * return:
 *  0: Finished
 *  -1: error
 */
int8_t read_intermediate_wind_data(void);

/* Format measurements to JSON and write to internal buffer.
 * return:
 *  pointer to array's (string's) start address
 */
char *get_avg_json_wind_data(void);


#endif
