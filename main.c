/*
 * Anemo_simple
 * 	Application intended for testing the capabilities of the RIOT-OS embedded
 * operating system and the connected modules. It cyclically shedules tasks,
 * where each task is connected to a modlule representing a peripheral measuring
 * device.
 *
 * Configuration
 * 	The use of different modules can be configured in "sys_config.h" by changing
 * the value of "SYS_CONFING". Correspinding to the system's configuration,
 * "DATA_FORMATER" must also be changed accordingly.
 *  Another parameter, which can be adjuste is the time period in between
 * consective measurements, which can be set using "DATA_SEND_PERIOD_MIN"
 *
 *
 * At this stage, in order to run, two modifications to RIOT-OS need to be made:
 *
 *  1) Add "riot-rhomb-zero" board to RIOT-OS, found at:
 *  	https://bitbucket.org/AlexanderMarinsek/riot-rhomb-zero/src/master/
 *
 *  2) Alter "cpu/samd21/periph/timer.c" as follows:
 *
 *   2.1) In "timer_init()", to choose normal frequency operation:
 * 		//TIMER_1_DEV.CTRLA.bit.WAVEGEN = TC_CTRLA_WAVEGEN_NFRQ_Val
 * 		TIMER_1_DEV.CTRLA.bit.WAVEGEN = TC_CTRLA_WAVEGEN_MFRQ_Val;
 *
 * 	 2.2) In "TIMER_1_ISR()", to not disable match/capture interrupt:
 *		//TIMER_1_DEV.INTENCLR.reg = TC_INTENCLR_MC0;
 *
 *
 */


#include "sys_control.h"
#include "pin_settings.h"
#include "hash.h"
#include "tasks/tasks.h"

#include "wind_data/wind_data.h"
#include "el_data/el_data.h"
#include "env_data/env_data.h"
#include "serial_data/serial_data.h"

#include "log.h"
#include "xtimer.h"
#include "periph/timer.h"
#include "thread.h"

#include <stdio.h>		// printf, ...
#include <stdint.h>		// uint16_t, ...
#include <stdlib.h>		// malloc, ...
#include <stddef.h>		// size_t, NULL, ...


#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG (1)
#endif
#include "debug.h"


/* Space in data buffer for symbols such as '{', '}' and ',' */
#define DATA_BUFFER_BALAST_LEN		16

/* Import tasks depending on system configuration */
extern kernel_pid_t pid_th_serial_data;
#if (SYS_CONFING & SYS_WIND_DATA_MASK)
	extern kernel_pid_t pid_th_wind_data;
#endif
#if (SYS_CONFING & SYS_ENV_DATA_MASK)
	extern kernel_pid_t pid_th_env_data;
#endif
#if (SYS_CONFING & SYS_EL_DATA_MASK)
	extern kernel_pid_t pid_th_el_data;
#endif
#if (SYS_CONFING & SYS_DV_DATA_MASK)
	extern kernel_pid_t pid_th_dv_data;
#endif


/* Catch errors related to malfunctioning modules. */
uint16_t sys_error = 0;

/* Length of buffer containing measurement data, used on averageing interval. */
size_t data_buffer_len = 0;

/* Length of buffer containing both meas. data, config, error, delimiters... */
size_t payload_buffer_len = 0;


static void cb(void *arg, int chan)
{
	static uint64_t last_time = 0;
	static uint64_t last_major_time = 0;

	//printf("elapsed - anemo: %lu\n", (uint32_t)(xtimer_now_usec64()-last_time));
	last_time = xtimer_now_usec64();

	static uint16_t ticks = 0;

	ticks ++;

#if (SYS_CONFING & SYS_WIND_DATA_MASK)
	if (!(sys_error & SYS_WIND_DATA_MASK)) {
		thread_wakeup(pid_th_wind_data);
	}
#endif
#if (SYS_CONFING & SYS_ENV_DATA_MASK)
	if (!(sys_error & SYS_ENV_DATA_MASK)) {
		thread_wakeup(pid_th_env_data);
	}
#endif
#if (SYS_CONFING & SYS_EL_DATA_MASK)
	if (!(sys_error & SYS_EL_DATA_MASK)) {
		thread_wakeup(pid_th_el_data);
	}
#endif
#if (SYS_CONFING & SYS_DV_DATA_MASK)
	if (!(sys_error & SYS_DV_DATA_MASK)) {
		thread_wakeup(pid_th_dv_data);
	}
#endif

	if (ticks == TICKS_PER_PERIOD) {
	//if (ticks == 2) {
		if (!(sys_error & SYS_SERIAL_DATA_MASK)) {
			thread_wakeup(pid_th_serial_data);
		}

		printf("elapsed - anemo: %lu\n", (uint32_t)(xtimer_now_usec64()-last_major_time));
		last_major_time = xtimer_now_usec64();

		ticks = 0;
	}

}


int main(void)
{


	/* Save module's buffer lenght */
	size_t tmp_buffer_len;

	/* Add up lenght of all data buffers
	 * Include balast ('{', '}', ',')
	 */
    data_buffer_len = DATA_BUFFER_BALAST_LEN;

	/* INIT MODULES */

#if (SYS_CONFING & SYS_WIND_DATA_MASK)
	/*tmp_buffer_len = init_wind_data(NORTH_OFFSET_10E1);
	if (tmp_buffer_len > 0){
		data_buffer_len += tmp_buffer_len;
	} else {
		LOG_ERROR("Failed: init_wind_data\n");
		sys_error |= SYS_WIND_DATA_MASK;
	}*/
    //init_wind_data(NORTH_OFFSET_10E1, &tmp_buffer_len);
	if (init_wind_data(NORTH_OFFSET_10E1, &tmp_buffer_len) != 0){
		LOG_ERROR("Failed: init_wind_data\n");
		sys_error |= SYS_WIND_DATA_MASK;
	}
	data_buffer_len += tmp_buffer_len;
#endif


#if (SYS_CONFING & SYS_ENV_DATA_MASK)
	if (init_env_data(&tmp_buffer_len) != 0){
		LOG_ERROR("Failed: init_env_data\n");
		sys_error |= SYS_ENV_DATA_MASK;
	}
	data_buffer_len += tmp_buffer_len;
#endif


#if (SYS_CONFING & SYS_EL_DATA_MASK)
	if (init_el_data(&tmp_buffer_len) != 0){
		LOG_ERROR("Failed: init_el_data\n");
		sys_error |= SYS_EL_DATA_MASK;
	}
	data_buffer_len += tmp_buffer_len;
#endif


#if (SYS_CONFING & SYS_DV_DATA_MASK)
#endif


#if (SYS_CONFING & SYS_SERIAL_DATA_MASK)
	init_serial_data(data_buffer_len, DEVICE_HASH, DEVICE_HASH_LEN);
	/*if (init_serial_data(DEVICE_HASH) != 0) {
		LOG_ERROR("Failed: init_serial_data\n");
		sys_error |= SYS_SERIAL_DATA_MASK;
		// GLOW RED, ERROR
	}*/
#endif


	/* CREATE TASKS */

#if (SYS_CONFING & SYS_WIND_DATA_MASK)
	if (!(sys_error & SYS_WIND_DATA_MASK)) {
		create_wind_data_task();
	}
#endif

#if (SYS_CONFING & SYS_ENV_DATA_MASK)
	if (!(sys_error & SYS_ENV_DATA_MASK)) {
		create_env_data_task();
	}
#endif

#if (SYS_CONFING & SYS_EL_DATA_MASK)
	if (!(sys_error & SYS_EL_DATA_MASK)) {
		create_el_data_task();
	}
#endif

#if (SYS_CONFING & SYS_DV_DATA_MASK)
	if (!(sys_error & SYS_DV_DATA_MASK)) {
		create_dv_data_task();
	}
#endif

#if (SYS_CONFING & SYS_SERIAL_DATA_MASK)
	if (!(sys_error & SYS_SERIAL_DATA_MASK)) {
		create_serial_data_task();
	}
#endif


	/* INIT TIMERS */

	xtimer_init	();

	/* initialize timer */
	//timer_init(TIMER_DEV(1), TIMER_FREQ, cb, (void *)(COOKIE * 5));
	/* Halt timer */
	//timer_stop(TIMER_DEV(1));
	/* Set timer period */
	//timer_set_absolute(TIMER_DEV(1), 0, TIMER_PERIOD_US);
	/* Start the timer */
	//timer_start(TIMER_DEV(1));

	timer_init(ATIMER_DEV, ATIMER_FREQ, cb, (void *)(COOKIE * 5));
	/* Halt timer */
	timer_stop(ATIMER_DEV);
	/* Set timer period */
	timer_set_absolute(ATIMER_DEV, 0, ATIMER_PERIOD_TICKS);
	/* Start the timer */
	timer_start(ATIMER_DEV);

	DEBUG("XTIMER_DEV: %d\n", XTIMER_DEV);

	while (1) {
		// Loop
		// Missing sleep...

		/* The following has no effect on task execution (timer #1 based).
		 * It only acts as a break for the loop (tested at 10 s).
		 */
		xtimer_usleep(5000);
	}

	printf("\nTEST END\n");

    return 0;
}
