#include "tasks.h"
#include "../sys_control.h"
#include "../wind_data/wind_data.h"
#include "../el_data/el_data.h"
#include "../env_data/env_data.h"
#include "../serial_data/serial_data.h"

#include "thread.h"
#include "log.h"

#include <stdio.h>		// printf, ...
#include <stdlib.h>		// malloc, ...
#include <stddef.h>		// size_t, NULL, ...


extern uint16_t sys_error;
extern size_t data_buffer_len;


/* DEFINE PROCESS AND STACK ***************************************************/

#if (SYS_CONFING & SYS_WIND_DATA_MASK)
char stack_th_wind_data[THREAD_STACKSIZE_DEFAULT];
kernel_pid_t pid_th_wind_data;
#endif

#if (SYS_CONFING & SYS_ENV_DATA_MASK)
char stack_th_env_data[THREAD_STACKSIZE_DEFAULT];
kernel_pid_t pid_th_env_data;
#endif

#if (SYS_CONFING & SYS_EL_DATA_MASK)
char stack_th_el_data[THREAD_STACKSIZE_DEFAULT];
kernel_pid_t pid_th_el_data;
#endif

#if (SYS_CONFING & SYS_DV_DATA_MASK)
char stack_th_dv_data[THREAD_STACKSIZE_DEFAULT];
kernel_pid_t pid_th_dv_data;
#endif

char stack_th_serial_data[THREAD_STACKSIZE_DEFAULT];
kernel_pid_t pid_th_serial_data;


/* TASK HANDLERS **************************************************************/

/* Wind data handler */
#if (SYS_CONFING & SYS_WIND_DATA_MASK)
void *th_wind_data_handler (void *arg)
{
    (void) arg;

    while (1) {
    	if (read_intermediate_wind_data() != 0) {
    		LOG_ERROR("Failed: read_intermediate_wind_data\n");
    		sys_error |= SYS_WIND_DATA_MASK;
    	}
    	thread_sleep();
    }

    return NULL;
}
#endif


/* Environmental data handler */
#if (SYS_CONFING & SYS_ENV_DATA_MASK)
void *th_env_data_handler (void *arg)
{
	(void) arg;

	    while (1) {
	    	if (read_intermediate_env_data() != 0) {
	    		LOG_ERROR("Failed: read_intermediate_env_data\n");
	    		sys_error |= SYS_ENV_DATA_MASK;
	    	}
	    	thread_sleep();
	    }

	    return NULL;
}
#endif


/* Electrical data handler */
#if (SYS_CONFING & SYS_EL_DATA_MASK)
void *th_el_data_handler (void *arg)
{
	(void) arg;
	int8_t intermediate_data_status;

	    while (1) {
	    	intermediate_data_status = read_intermediate_el_data();
	    	switch (intermediate_data_status) {
	    	case 0:
	    		thread_sleep();
	    		break;
	    	case 1:
	    		// Busy
	    		break;
	    	case -1:
	    		LOG_ERROR("Failed: read_intermediate_el_data\n");
	    		sys_error |= SYS_EL_DATA_MASK;
	    		thread_sleep();
	    		break;
	    	default:
	    		break;
	    	}
	    }

	    return NULL;
}
#endif


/* Wind gradient data handler */
#if (SYS_CONFING & SYS_DV_DATA_MASK)
void *th_dv_data_handler (void *arg)
{
	(void) arg;
	    while (1) {
	    	thread_sleep();
	    }
	    return NULL;
}
#endif


/* Serial data handler */
#if (SYS_CONFING & SYS_SERIAL_DATA_MASK)
void *th_serial_data_handler (void *arg)
{
    (void) arg;

    char *data_buf;
    data_buf = malloc(data_buffer_len);

    while (1) {

    	/* Add data from modules that are in use */
    	snprintf (data_buf, data_buffer_len, DATA_FORMATER
#if (SYS_CONFING & SYS_WIND_DATA_MASK)
    			,get_avg_json_wind_data()
#endif
#if (SYS_CONFING & SYS_ENV_DATA_MASK)
    			,get_avg_json_env_data()
#endif
#if (SYS_CONFING & SYS_EL_DATA_MASK)
    			,get_avg_json_el_data()
#endif
#if (SYS_CONFING & SYS_DV_DATA_MASK)
    			//,get_avg_json_dv_data()
#endif
    	);

    	//printf("%s\n", data_buf);

    	if(send_serial_data(data_buf, (uint16_t)SYS_CONFING, sys_error) != 0) {
    		// GLOW RED
    	    return NULL;
    	}

    	thread_sleep();
    }

    return NULL;
}
#endif


/* CREATE TASKS (THREADS) **************************************************/

#if (SYS_CONFING & SYS_WIND_DATA_MASK)
void create_wind_data_task(void) {
	pid_th_wind_data = thread_create(
		stack_th_wind_data,
		sizeof(stack_th_wind_data),
		THREAD_PRIORITY_MAIN - 4,
		THREAD_CREATE_SLEEPING,
		th_wind_data_handler, NULL,
		"th_wind_data_handler");
}
#endif


#if (SYS_CONFING & SYS_ENV_DATA_MASK)
void create_env_data_task(void) {
		pid_th_env_data = thread_create(
			stack_th_env_data,
			sizeof(stack_th_env_data),
			THREAD_PRIORITY_MAIN - 5,
			THREAD_CREATE_SLEEPING,
			th_env_data_handler, NULL,
			"th_env_data_handler");
}
#endif


#if (SYS_CONFING & SYS_EL_DATA_MASK)
void create_el_data_task(void) {
	pid_th_el_data = thread_create(
		stack_th_el_data,
		sizeof(stack_th_el_data),
		THREAD_PRIORITY_MAIN - 3,
		THREAD_CREATE_SLEEPING,
		th_el_data_handler, NULL,
		"th_el_data_handler");
}
#endif


#if (SYS_CONFING & SYS_DV_DATA_MASK)
void create_dv_data_task(void) {
	pid_th_dv_data = thread_create(
		stack_th_dv_data,
		sizeof(stack_th_dv_data),
		THREAD_PRIORITY_MAIN - 2,
		THREAD_CREATE_SLEEPING,
		th_dv_data_handler, NULL,
		"th_dv_data_handler");
}
#endif


#if (SYS_CONFING & SYS_SERIAL_DATA_MASK)
void create_serial_data_task(void) {
	pid_th_serial_data = thread_create(
		stack_th_serial_data,
		sizeof(stack_th_serial_data),
		THREAD_PRIORITY_MAIN - 1,
		THREAD_CREATE_SLEEPING,
		th_serial_data_handler, NULL,
		"th_serial_data");
}
#endif



