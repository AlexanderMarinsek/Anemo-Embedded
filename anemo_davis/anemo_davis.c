#include "anemo_davis.h"

#include "xtimer.h"
#include "periph/gpio.h"
#include "periph/adc.h"

#include <log.h>
#include <math.h>
#include <stdint.h>

#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG (0)
#endif
#include "debug.h"

/* Anemometer physical interface and timer structure */
static Anemo_davis anemo_davis;

/* Direction offset from north in degrees * 10e1 */
uint16_t _north_offset_10e1;

/* Prototypes */
static int _read_rotations (void);
static int _read_single_counter_output (int mux_c_val, int mux_b_val, int mux_a_val);
static int8_t _disable_and_reset_counter (void);
static int8_t _is_counter_reset (void);
static void _set_address_bits (int mux_c_val, int mux_b_val, int mux_a_val);
static void _wait_for_mux_output_propagation (void);
static void _enable_counter_input (void);
static void _disable_counter_input (void);
static void _clear_counter (void);

static int _calc_wind_speed_ms_10e2 (int rotations, uint64_t elapsed_time_us);


/* Init anemometer counter, reseet and enable. */
int8_t anemo_davis_init (
		uint16_t north_offset_10e1,
		gpio_t mux_out, gpio_t mux_c, gpio_t mux_b, gpio_t mux_a,
		gpio_t counter_rst, gpio_t counter_n_en, adc_t adc_line)
{
	_north_offset_10e1 = north_offset_10e1;

    anemo_davis.mux_out = mux_out;
    anemo_davis.mux_c = mux_c;
    anemo_davis.mux_b = mux_b;
    anemo_davis.mux_a = mux_a;
    anemo_davis.counter_rst = counter_rst;
    anemo_davis.counter_n_en = counter_n_en;
    anemo_davis.adc_line = adc_line;
    anemo_davis.last_read_time_us = 0;

    int8_t gpio_error = 0;
    gpio_error += gpio_init (anemo_davis.mux_out, GPIO_IN);
    gpio_error += gpio_init (anemo_davis.mux_c, GPIO_OUT);
    gpio_error += gpio_init (anemo_davis.mux_b, GPIO_OUT);
    gpio_error += gpio_init (anemo_davis.mux_a, GPIO_OUT);
    gpio_error += gpio_init (anemo_davis.counter_rst, GPIO_OUT);
    gpio_error += gpio_init (anemo_davis.counter_n_en, GPIO_OUT);

    if (gpio_error != 0) {
    	LOG_ERROR("Failed: gpio_init\n");
        return -1;
    }

    /* Reset counter */
    if (_disable_and_reset_counter() != 0) {
        LOG_ERROR("Failed: _disable_and_reset_counter\n");
        return -1;
    }

    /* Start counting and timing */
    _enable_counter_input();
    anemo_davis.last_read_time_us = xtimer_now_usec64();
    _wait_for_mux_output_propagation();

    /* Enable ADC */
    if (adc_init(anemo_davis.adc_line) != 0) {
        LOG_ERROR("Failed: adc_init\n");
        return -1;
    }

    return 0;
}



/**
 * Read and reset anemometer counter, while converting to kmh speed
 */
int anemo_davis_calc_speed_ms_10e2 (void)
{
    /* Disable counter and save time */
    _disable_counter_input();
    uint64_t read_time_us = xtimer_now_usec64();
    _wait_for_mux_output_propagation();

    /* Get elapsed time and number of rotations */
    uint64_t elapsed_time_us = read_time_us - anemo_davis.last_read_time_us;
    int rotations = _read_rotations();

    /* Reset counter */
    if (_disable_and_reset_counter() != 0) {
        LOG_ERROR("Failed: _disable_and_reset_counter\n");
        return -1;
    }

    /* Enable counter and reset timer */
    _enable_counter_input();
    anemo_davis.last_read_time_us = xtimer_now_usec64();
    _wait_for_mux_output_propagation();

    /* Calculate speed in m/s * 100 */
    int speed_ms_10e2 =
    		_calc_wind_speed_ms_10e2(rotations, elapsed_time_us);

    DEBUG("rotations: %d, elapsed_time_us: %lu, speed_ms_10e2: %d\n",
        rotations,
		(uint32_t)elapsed_time_us,
		speed_ms_10e2);

    return speed_ms_10e2;
}


int _calc_wind_speed_ms_10e2 (int rotations, uint64_t elapsed_time_us)
{
    /* Calc speed in m/s (official formula) */
    float f_speed_ms =
    		((float)rotations * DAVIS_SPEED_MULTIPLIER_MS * U_SEC_IN_SEC)
    		/ elapsed_time_us;

    /* Round to one decimal and multiply by 10 (vice versa) */
    return (int)(roundf(f_speed_ms * 100));
}


int anemo_get_wind_direction_10e1 (void)
{
	// Read ADC
    int adc_value = adc_sample (anemo_davis.adc_line, DAVIS_ADC_RESOLUTION);

    /* Catch error */
    if (adc_value == -1) {
		LOG_ERROR("Failed: adc_sample\n");
    	return -1;
    }

    float f_wind_direction = (float)adc_value / (DAVIS_DIRECTION_RESOLUTION-1) * 360;
    int wind_direction = (int)(roundf(f_wind_direction * 10)) % 3600;

    DEBUG("adc_value: %d, wind_direction: %d, DAVIS_DIRECTION_RESOLUTION: %d\n",
    		adc_value, wind_direction, DAVIS_DIRECTION_RESOLUTION);

	return wind_direction + _north_offset_10e1;
}


/**
 * Read number of rotations (counter value)
 */
static int _read_rotations (void)
{

    int rotations = 0;

    rotations |= _read_single_counter_output(0,0,0) << 0;
    rotations |= _read_single_counter_output(0,0,1) << 1;
    rotations |= _read_single_counter_output(0,1,0) << 2;
    rotations |= _read_single_counter_output(0,1,1) << 3;

    rotations |= _read_single_counter_output(1,0,0) << 4;
    rotations |= _read_single_counter_output(1,0,1) << 5;
    rotations |= _read_single_counter_output(1,1,0) << 6;
    rotations |= _read_single_counter_output(1,1,1) << 7;

    DEBUG("Rotations: %d, %d, %d, %d, %d, %d, %d, %d\n",
        _read_single_counter_output(1,1,1),
        _read_single_counter_output(1,1,0),
        _read_single_counter_output(1,0,1),
        _read_single_counter_output(1,0,0),
        _read_single_counter_output(0,1,1),
        _read_single_counter_output(0,1,0),
        _read_single_counter_output(0,0,1),
        _read_single_counter_output(0,0,0)
    );

    return rotations;
}


/**
 * Read single counter output
 */
static int _read_single_counter_output (int mux_c_val, int mux_b_val, int mux_a_val)
{
    _set_address_bits(mux_c_val, mux_b_val, mux_a_val);
    _wait_for_mux_output_propagation();

    return gpio_read (anemo_davis.mux_out);
}


/**
 * Set muliplexer address bis
 */
static void _set_address_bits (int mux_c_val, int mux_b_val, int mux_a_val)
{
    gpio_write (anemo_davis.mux_c, mux_c_val);
    gpio_write (anemo_davis.mux_b, mux_b_val);
    gpio_write (anemo_davis.mux_a, mux_a_val);

    return;
}


/**
 * Reset and enable counter
 */
static int8_t _disable_and_reset_counter (void)
{
    _set_address_bits(0,0,0);

    _disable_counter_input();
    _wait_for_mux_output_propagation();
    _clear_counter();

    if (_is_counter_reset() != 0) {
        LOG_ERROR("Failed: _is_counter_reset\n");
        return -1;
    }

    //_enable_counter_input();
    //_wait_for_mux_output_propagation();
    //anemo_davis.last_read_time_us = xtimer_now_usec64();

    return 0;
}


/**
 * Check if counter is reset by reading it
 */
static int8_t _is_counter_reset (void)
{
    if (_read_rotations()) {
         return -1;
    }
    return 0;
}


/**
 * Set switch (1st MUX) to enable counter input
 */
static void _enable_counter_input (void)
{
    gpio_clear (anemo_davis.counter_n_en);
}


/**
 * Clear switch (1st MUX) to disable counter input
 */
static void _disable_counter_input (void)
{
    gpio_set (anemo_davis.counter_n_en);
}


/**
 * Clear (reset) counter
 */
static void _clear_counter (void)
{
    gpio_clear (anemo_davis.counter_rst);
    _wait_for_mux_output_propagation();

    gpio_set (anemo_davis.counter_rst);
    _wait_for_mux_output_propagation();

    gpio_clear (anemo_davis.counter_rst);
    _wait_for_mux_output_propagation();
}


/**
 * Wait till MUX internally switches signals (should be less than 0.5us)
 */
static void _wait_for_mux_output_propagation (void)
{
    xtimer_usleep(MUX_PROPAGATION_DELAY_US);
}
