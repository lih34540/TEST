/**
 * \author {AUTHOR}
 */

#ifndef DATAMGR_H_
#define DATAMGR_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "config.h"
#include "sbuffer.h"

/* default running average length */
#ifndef RUN_AVG_LENGTH
#define RUN_AVG_LENGTH 5
#endif

#ifndef SET_MAX_TEMP
#define SET_MAX_TEMP 30
#endif

#ifndef SET_MIN_TEMP
#define SET_MIN_TEMP 10
#endif

/**
 * Initialize the datamgr by reading the room_sensor.map file.
 */
void datamgr_init(FILE *fp_map);

/**
 * Datamgr thread function.
 * Continuously reads data from the shared buffer and processes it.
 */
void *datamgr_thread(void *arg);

/**
 * Clean up all allocated memory.
 */
void datamgr_free(void);

/**
 * Getters (may be useful for testing)
 */
sensor_value_t datamgr_get_avg(sensor_id_t id);
room_id_t datamgr_get_room_id(sensor_id_t id);
sensor_ts_t datamgr_get_last_modified(sensor_id_t id);
int datamgr_get_total_sensors(void);

#endif /* DATAMGR_H_ */
