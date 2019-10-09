#ifndef TASKS_H
#define TASKS_H

/* Wind data handler */
void *th_wind_data_handler (void *arg);
void create_wind_data_task(void);

/* Environmental data handler */
void *th_env_data_handler (void *arg);
void create_env_data_task(void);

/* Electrical data handler */
void *th_el_data_handler (void *arg);
void create_el_data_task(void);

/* Wind gradient data handler */
void *th_dv_data_handler (void *arg);
void create_dv_data_task(void);

/* Serial data handler */
void *th_serial_data_handler (void *arg);
void create_serial_data_task(void);

#endif
