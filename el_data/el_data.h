#ifndef EL_DATA_H
#define EL_DATA_H


#include "ina220.h"

#include <stdint.h>
#include <stddef.h>				// size_t


/* Select what you want to measure and use */
#define EL_DATA_MEASURE_PV 					1
#define EL_DATA_MEASURE_VX 					1
#define EL_DATA_USE_CHARGER					1

/* Determine selected mode */
#define EL_DATA_MODE 						(\
	EL_DATA_USE_CHARGER << 2 | EL_DATA_MEASURE_VX << 1 | EL_DATA_MEASURE_PV << 0 )

#define EL_DATA_MODE_PV						(1U << 0)
#define EL_DATA_MODE_VX						(1U << 1)
#define EL_DATA_MODE_CHARGE_EN				(1U << 2)

#define EL_DATA_DISCONNECTED				(-1)

/* State identifiers */
#define EL_DATA_STATE_IDLE					(0x0000U)
#define EL_DATA_STATE_START_VX_MEAS			(0x0001U)
#define EL_DATA_STATE_MEASURE_VX			(0x0002U)
#define EL_DATA_STATE_SET_RL2_RL3			(0x0003U)
#define EL_DATA_STATE_START_PV_UOC_MEAS		(0x0004U)
#define EL_DATA_STATE_MEASURE_PV_UOC		(0x0005U)
#define EL_DATA_STATE_SET_RL1				(0x0006U)
#define EL_DATA_STATE_START_PV_ISC_MEAS		(0x0007U)
#define EL_DATA_STATE_MEASURE_PV_ISC		(0x0008U)
#define EL_DATA_STATE_CLEAR_RL1				(0x0009U)
#define EL_DATA_STATE_CLEAR_RL2_RL3			(0x000AU)

#define EL_DATA_STATE_MASK_RST_STATE		(~(0x000FU))

/* Relay propagation / switch delay (equal for both on and off) */
/* HF3FD relay needs 10ms (datasheet) */
#define EL_DATA_RELAY_DELAY_US				(20 * 1000)

/* Json buffer format (%05d => sign + %04d).
 * 	vx : Sxxxx [mV]
 *  pv_uoc : Sxxxx [mV]
 *	pv_isc : Sxxxx [mA]
 */
#define EL_DATA_JSON_FORMAT		""\
	"\"vx\":%d,"\
	"\"pv_uoc\":%d,"\
	"\"pv_isc\":%d"
//#define EL_DATA_JSON_FORMAT		""\
//	"\"vx\":%05d,"\
//	"\"pv_uoc\":%05d,"\
//	"\"pv_isc\":%05d"

/* Length of json data buffer */
#define EL_DATA_BUFFER_LEN		64


/* Ina configuration and calibration ******************************************/
#define INA_I2C_ADDR						(0x45)
/* Calibration returns mA from mV when using R = 0.01 ohm */
#define INA_CALIBRATION 					(4096)
#define INA_CNVR_READY_MASK					(1U << 1)
#define INA_CONFIG   	(INA220_MODE_TRIGGER_SHUNT_BUS | \
						 INA220_RANGE_320MV | \
						 INA220_BRNG_32V_FSR | \
						 INA220_SADC_12BIT | \
						 INA220_BADC_12BIT)


 /* Intermediate data (sum of measurements, avg. counter...). */
typedef struct {
	int vx_sum;
	int pv_isc_sum;
	int pv_uoc_sum;
	int average_counter;
} Intermediate_el_data;

/* Data (measurements, buffer...). */
typedef struct {
int vx;
int pv_isc;
int pv_uoc;
char buffer[EL_DATA_BUFFER_LEN];
} El_data;


/* Initiate Ina module and relays for electrical measurements.
 *  p1: pointer to where the module's buffer lenght will be written
 * return:
 *  0 on success, -1 on error
 */
int8_t init_el_data (size_t *buffer_len);

/* Read electrical data without blocking further execution.
 * return:
 *  0: measurement cycle finished
 *  1: measurement cycle running
 *  -1: error
 */
int8_t read_intermediate_el_data(void);

/* Format measurements to JSON and write to internal buffer.
 * return:
 *  pointer to array's (string's) start address
 */
char *get_avg_json_el_data(void);


#endif
