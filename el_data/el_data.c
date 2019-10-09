#include "el_data.h"
#include "../pin_settings.h"

#include "log.h"
#include "xtimer.h"
#include "ina220.h"

#include <math.h>				// roundf()
#include <stddef.h>				// size_t
#include <stdint.h>

#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG (0)
#endif
#include "debug.h"


/* Electrical data module's state variable. */
static uint16_t _el_data_state;

/* Intermediate data (sum of measurements, avg. counter...). */
static Intermediate_el_data _intermediate_el_data;
/* Data (measurements, buffer...). */
static El_data _el_data;

/* Last time relays were swithced (shared variable).
 * RL2 and RL3 switch simultaneously.
 * RL1 waits for RL2 and RL3 to finish, before switching itself.
 */
static uint64_t _relay_switch_time;

/* Shared INA device. */
static ina220_t _dev_ina;

/* internal error variable used to set values to 0 on error */
static int8_t _error_detected;


/* Prototypes *****************************************************************/

static void _calc_avg_el_data(void);

static int8_t _init_ina(void);
static int8_t _init_relays(void);

/* State manipulation */
static int8_t _idle_state(void);
static int8_t _start_vx_measurement(void);
static int8_t _measure_vx(void);
static int8_t _set_rl2_and_rl3(void);
static int8_t _start_pv_uoc_measurement(void);
static int8_t _measure_pv_uoc(void);
static int8_t _set_rl1(void);
static int8_t _start_pv_isc_measurement(void);
static int8_t _measure_pv_isc(void);
static int8_t _clear_rl1(void);
static int8_t _clear_rl2_and_rl3(void);

static void _change_state(int state_mask);
static void _reset_switch_time(void);
static int8_t _waiting_for_switch_time(void);

/* Reset data */
static void _reset_intermediate_data(void);
static void _reset_avg_data(void);


/* Functions ******************************************************************/
/* Initiate Ina module and relays for electrical measurements. */
int8_t init_el_data (size_t *buffer_len) {

	/* Reset state variables */
	_el_data_state = 0;
	_relay_switch_time = 0;
	_error_detected = 0;

	*buffer_len = EL_DATA_BUFFER_LEN;

	if (_init_ina() != 0) {
		LOG_ERROR("Failed: _init_ina\n");
		_error_detected = 1;
		return -1;
	}

	if (_init_relays() != 0) {
		LOG_ERROR("Failed: _init_relays\n");
		_error_detected = 1;
		return -1;
	}

	_change_state(EL_DATA_STATE_IDLE);

	/* Reset relay switch time */
	_reset_switch_time();

	/* Reset intermediate values */
	_reset_intermediate_data();

	return 0;
}

/* Read electrical data without blocking further execution. */
int8_t read_intermediate_el_data(void) {

	if (_error_detected) {
		_reset_intermediate_data();
		return -1;
	}

	/* Exit with busy status */
	if (_waiting_for_switch_time()) {
		return 1;
	}

	switch (_el_data_state) {

		case EL_DATA_STATE_IDLE:
			_intermediate_el_data.average_counter++;
			return _idle_state();
			break;
		case EL_DATA_STATE_START_VX_MEAS:
			return _start_vx_measurement();
			break;
		case EL_DATA_STATE_MEASURE_VX:
			return _measure_vx();
			break;
		case EL_DATA_STATE_SET_RL2_RL3:
			return _set_rl2_and_rl3();
			break;
		case EL_DATA_STATE_START_PV_UOC_MEAS:
			return _start_pv_uoc_measurement();
			break;
		case EL_DATA_STATE_MEASURE_PV_UOC:
			return _measure_pv_uoc();
			break;
		case EL_DATA_STATE_SET_RL1:
			return _set_rl1();
			break;
		case EL_DATA_STATE_START_PV_ISC_MEAS:
			return _start_pv_isc_measurement();
			break;
		case EL_DATA_STATE_MEASURE_PV_ISC:
			return _measure_pv_isc();
			break;
		case EL_DATA_STATE_CLEAR_RL1:
			return _clear_rl1();
			break;
		case EL_DATA_STATE_CLEAR_RL2_RL3:
			return _clear_rl2_and_rl3();
			break;
	}

	return 1;
}

/* Format measurements to JSON and write to internal buffer. */
char *get_avg_json_el_data(void) {

	/* Calculate average values (all zero on error) */
	_calc_avg_el_data();

	snprintf(_el_data.buffer, EL_DATA_BUFFER_LEN, EL_DATA_JSON_FORMAT,
			_el_data.vx,
			_el_data.pv_uoc,
			_el_data.pv_isc);

	DEBUG("%s\n", _el_data.buffer);

	return _el_data.buffer;
}

/* Calculate average values and save to static structure.
 * All values are set to zero when module error is detected.
 */
void _calc_avg_el_data(void) {

	if (_error_detected) {
		/* Set values to zero */
		_reset_avg_data();
		return;
	}

	int average_counter = _intermediate_el_data.average_counter;

	float f_vx = (float)_intermediate_el_data.vx_sum / average_counter;
	_el_data.vx = (int)roundf(f_vx);

	float f_pv_uoc = (float)_intermediate_el_data.pv_uoc_sum / average_counter;
	_el_data.pv_uoc = (int)roundf(f_pv_uoc);

	float f_pv_isc = (float)_intermediate_el_data.pv_isc_sum / average_counter;
	_el_data.pv_isc = (int)roundf(f_pv_isc);

	DEBUG("vx: %d, pv_uoc: %d, pv_isc: %d, avg: %d\n",
			_el_data.vx, _el_data.pv_uoc, _el_data.pv_isc, average_counter);

	/* Reset intermediate values */
	_reset_intermediate_data();

	return;
}


/* Init helpers ***************************************************************/

/* Init INA device.
 * return:
 * 	0: Success
 * 	-1: Failed
 */
int8_t _init_ina(void) {
	if (ina220_init(&_dev_ina, EL_DATA_I2C_DEV, INA_I2C_ADDR) != 0) {
		LOG_ERROR("Failed: ina220_init\n");
		_error_detected = 1;
        return -1;
	}
	if (ina220_set_config(&_dev_ina, INA_CONFIG) != 0) {
		LOG_ERROR("Failed: ina220_set_config\n");
		_error_detected = 1;
		return EL_DATA_DISCONNECTED;
	}
	if (ina220_set_calibration(&_dev_ina, INA_CALIBRATION) != 0) {
		LOG_ERROR("Failed: ina220_set_calibration\n");
		_error_detected = 1;
		return -1;
	}
	return 0;
}

/* Init GPIO pins for use with relays.
 * return:
 * 	0: Success
 * 	-1: Failed
 */
int8_t _init_relays(void) {
	/* Pin setup */
    if (gpio_init (EL_DATA_RE1_PIN, GPIO_OUT) != 0) {
    	LOG_ERROR("Failed: gpio_init\n");
		_error_detected = 1;
		return -1;
    }
#if (EL_DATA_MODE & EL_DATA_MODE_VX)
    if (gpio_init(EL_DATA_RE2_PIN, GPIO_OUT) != 0) {
    	LOG_ERROR("Failed: gpio_init\n");
		_error_detected = 1;
		return -1;
    }
#endif
#if (EL_DATA_MODE & EL_DATA_MODE_CHARGE_EN)
    if (gpio_init(EL_DATA_RE3_PIN, GPIO_OUT) != 0) {
    	LOG_ERROR("Failed: gpio_init\n");
		_error_detected = 1;
		return -1;
    }
#endif

    /* Clear pins */
	gpio_clear(EL_DATA_RE1_PIN);
#if (EL_DATA_MODE & EL_DATA_MODE_VX)
	gpio_clear(EL_DATA_RE2_PIN);
#endif
#if (EL_DATA_MODE & EL_DATA_MODE_CHARGE_EN)
	gpio_clear(EL_DATA_RE3_PIN);
#endif

	return 0;
}


/* State machine **************************************************************/

/* Reduntant state - for easier understanding of state machine.
 */
int8_t _idle_state(void) {

#if (EL_DATA_MODE & EL_DATA_MODE_VX)
	/* Next read interval will start by measuring Vx */
	_change_state(EL_DATA_STATE_START_VX_MEAS);
#elif (EL_DATA_MODE & EL_DATA_MODE_CHARGE_EN)
	/* Next read interval will start by switching RL3 */
	_change_state(EL_DATA_STATE_SET_RL2_RL3);
#else
	/* Next read interval will start by measuring Uoc */
	_change_state(EL_DATA_STATE_START_PV_UOC_MEAS);
#endif

	/* Return 'not yet finished' */
	return 1;
}


/* Trigger new measurement and change state.
 * return.
 *  1: Triggered
 *  -1: Failiure
 */
int8_t _start_vx_measurement(void) {
	if (ina220_set_config(&_dev_ina, INA_CONFIG) != 0) {
		LOG_ERROR("Failed: ina220_set_config\n");
		_error_detected = 1;
		return EL_DATA_DISCONNECTED;
	}
	_change_state(EL_DATA_STATE_MEASURE_VX);
	return 1;
}


/* Measure Vx.
 */
int8_t _measure_vx(void) {

	/* Check CNVR bit and exit if nothing new */
	int16_t val;
    ina220_read_bus(&_dev_ina, &val);
    if (!(val & INA_CNVR_READY_MASK)) {
    	return 1;
    }

    val = (val >> INA220_BUS_VOLTAGE_SHIFT) * 4;

	_intermediate_el_data.vx_sum += (int)val;
	_change_state(EL_DATA_STATE_SET_RL2_RL3);

	DEBUG("bus: %6d mV, vx_sum: %d\n",
			val, _intermediate_el_data.vx_sum);

	/* Return 'not yet finished' */
	return 1;
}


/* Connect relays RL2 and/or RL3.
 */
int8_t _set_rl2_and_rl3(void) {
	/* Set corresponding relay pins to high */
#if (EL_DATA_MODE & EL_DATA_MODE_VX)
	gpio_set(EL_DATA_RE2_PIN);
#endif
#if (EL_DATA_MODE & EL_DATA_MODE_CHARGE_EN)
	gpio_set(EL_DATA_RE3_PIN);
#endif
	/* Reset timer and change state */
	_reset_switch_time();
	_change_state(EL_DATA_STATE_START_PV_UOC_MEAS);
	//_set_switching_mask();

	/* Return 'not yet finished' */
	return 1;
}


/* Trigger new measurement and change state.
 * return.
 *  1: Triggered
 *  -1: Failiure
 */
int8_t _start_pv_uoc_measurement(void) {
	if (ina220_set_config(&_dev_ina, INA_CONFIG) != 0) {
		LOG_ERROR("Failed: ina220_set_config\n");
		_error_detected = 1;
		return EL_DATA_DISCONNECTED;
	}
	_change_state(EL_DATA_STATE_MEASURE_PV_UOC);
	return 1;
}


/* Measure Uoc.
 */
int8_t _measure_pv_uoc(void) {

	/* Check CNVR bit and exit if nothing new */
	int16_t val;
    ina220_read_bus(&_dev_ina, &val);
    if (!(val & INA_CNVR_READY_MASK)) {
    	return 1;
    }

    val = (val >> INA220_BUS_VOLTAGE_SHIFT) * 4;

	_intermediate_el_data.pv_uoc_sum += (int)val;
	_change_state(EL_DATA_STATE_SET_RL1);

	DEBUG("bus: %6d mV, pv_uoc_sum: %d\n",
			val, _intermediate_el_data.pv_uoc_sum);

	/* Return 'not yet finished' */
	return 1;
}


/* Connect relay RL1.
 */
int8_t _set_rl1(void) {
	/* Set corresponding relay pin to high */
	gpio_set(EL_DATA_RE1_PIN);
	/* Reset timer and change state */
	_reset_switch_time();
	_change_state(EL_DATA_STATE_START_PV_ISC_MEAS);

	/* Return 'not yet finished' */
	return 1;
}


/* Trigger new measurement and change state.
 * return.
 *  1: Triggered
 *  -1: Failure
 */
int8_t _start_pv_isc_measurement(void) {
	if (ina220_set_config(&_dev_ina, INA_CONFIG) != 0) {
		LOG_ERROR("Failed: ina220_set_config\n");
		_error_detected = 1;
		return EL_DATA_DISCONNECTED;
	}
	_change_state(EL_DATA_STATE_MEASURE_PV_ISC);
	return 1;
}


/* Measure Isc.
 */
int8_t _measure_pv_isc(void) {

	/* Check CNVR bit and exit if nothing new */
	int16_t val;
    ina220_read_bus(&_dev_ina, &val);
    if (!(val & INA_CNVR_READY_MASK)) {
    	return 1;
    }

    ina220_read_current(&_dev_ina, &val);

	_intermediate_el_data.pv_isc_sum += (int)val;
	_change_state(EL_DATA_STATE_CLEAR_RL1);

	DEBUG("current: %6d mA, pv_isc_sum: %d\n",
			val, _intermediate_el_data.pv_isc_sum);

	/* Return 'not yet finished' */
	return 1;
}


/* Disconnect relay RL1.
 */
int8_t _clear_rl1(void) {
	/* Set corresponding relay pin to high */
	gpio_clear(EL_DATA_RE1_PIN);

	/* Reset timer and change state */
	_reset_switch_time();

#if (EL_DATA_MODE & EL_DATA_MODE_VX || EL_DATA_MODE & EL_DATA_MODE_CHARGE_EN)
	/* Clear RL2, RL3 before proceding */
	_change_state(EL_DATA_STATE_CLEAR_RL2_RL3);
	/* Return 'not yet finished' */
	return 1;
#else
	_change_state(EL_DATA_STATE_IDLE);
	/* Return 'finished' */
	return 0;
#endif
}


/* Disconnect relays RL2 and/or RL3.
 */
int8_t _clear_rl2_and_rl3(void) {
	/* Set corresponding relay pins to high */
#if (EL_DATA_MODE & EL_DATA_MODE_VX)
	gpio_clear(EL_DATA_RE2_PIN);
#endif
#if (EL_DATA_MODE & EL_DATA_MODE_CHARGE_EN)
	gpio_clear(EL_DATA_RE3_PIN);
#endif

	/* Reset timer and change state */
	_reset_switch_time();

	_change_state(EL_DATA_STATE_IDLE);
	/* Return 'finished' */
	return 0;
//#endif
}


/* Helpers ********************************************************************/

/* Change el. data state.
 *  param1: new state's mask
 */
void _change_state(int state_mask) {
	_el_data_state &= EL_DATA_STATE_MASK_RST_STATE;
	_el_data_state |= state_mask;
}


/* Reset relay switch timer.
 */
void _reset_switch_time(void) {
	_relay_switch_time = xtimer_now_usec64();
}


/* Check if relay timer has ended.
 */
int8_t _waiting_for_switch_time(void) {
	return
		(!(xtimer_now_usec64() - _relay_switch_time > EL_DATA_RELAY_DELAY_US));
}


/* (re)Set intermediate structure values to 0.
 */
void _reset_intermediate_data (void) {
	_intermediate_el_data.vx_sum = 0;
	_intermediate_el_data.pv_uoc_sum = 0;
	_intermediate_el_data.pv_isc_sum = 0;
	_intermediate_el_data.average_counter = 0;
}

/* (re)Set average structure values to 0.
 */
static void _reset_avg_data (void) {
	_el_data.vx = 0;
	_el_data.pv_uoc = 0;
	_el_data.pv_isc = 0;
}
