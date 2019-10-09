#ifndef ANEMO_DAVIS_H
#define ANEMO_DAVIS_H

#include "periph/gpio.h"
#include "periph/adc.h"

#include <stdint.h>

#ifndef U_SEC_IN_SEC
#define U_SEC_IN_SEC                1000000
#endif

#define DAVIS_SPEED_MULTIPLIER_MPH		2.25
#define DAVIS_SPEED_MULTIPLIER_KMH      DAVIS_SPEED_MULTIPLIER_MPH * 1.609344
#define DAVIS_SPEED_MULTIPLIER_MS	    DAVIS_SPEED_MULTIPLIER_MPH * 0.44704

#define MUX_PROPAGATION_DELAY_US    	1

//#define DAVIS_ADC_RESOLUTION			ADC_RES_8BIT
#define DAVIS_ADC_RESOLUTION			ADC_RES_10BIT
//#define DAVIS_DIRECTION_RESOLUTION		(1 << 8)
#define DAVIS_DIRECTION_RESOLUTION		(1 << 10)


/* Anemometer physical interface and timer structure */
typedef struct {
    gpio_t mux_out;
    gpio_t mux_c;
    gpio_t mux_b;
    gpio_t mux_a;
    gpio_t counter_rst;
    gpio_t counter_n_en;
    adc_t adc_line;
    uint64_t last_read_time_us;
} Anemo_davis;

/**
 * @brief   Init anemometer counter, reseet and enable
 *
 * @param[in]  north_offset_10e1	Offset from north
 * @param[in]  mux_out          	Multiplexer output pin
 * @param[in]  mux_c            	Multiplexer select C pin
 * @param[in]  mux_b            	Multiplexer select B pin
 * @param[in]  mux_a            	Multiplexer select A pin
 * @param[in]  counter_rst      	Counter reset pin
 * @param[in]  counter_n_en     	Counter enable pin
 *
 * @returns     0 on success, -1 on fail
 */
int8_t anemo_davis_init (
	uint16_t north_offset_10e1,
    gpio_t mux_out, gpio_t mux_c, gpio_t mux_b, gpio_t mux_a,
    gpio_t counter_rst, gpio_t counter_n_en, adc_t adc_line);

/**
 * @brief   Read and reset anemometer counter, while converting to ms speed
 *
 * @returns     Wind speed in ms * 100
 */
int anemo_davis_calc_speed_ms_10e2 (void);

/**
 * @brief   Read wind vane direction
 *
 * @returns     Direction in degrees * 10
 */
int anemo_get_wind_direction_10e1 (void);



#endif
