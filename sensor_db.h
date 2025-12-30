/**
 * \author {AUTHOR}
 */

#ifndef _SENSOR_DB_H_
#define _SENSOR_DB_H_

#include <stdio.h>
#include <stdbool.h>
#include "config.h"

/**
 * Open (or create) the csv file.
 * filename: name of the csv file
 * append: true -> append mode, false -> overwrite mode
 */
FILE *open_db(char *filename, bool append);

/**
 * Insert one sensor measurement into the csv file.
 */
int insert_sensor(FILE *f,
                  sensor_id_t id,
                  sensor_value_t value,
                  sensor_ts_t ts);

/**
 * Close the csv file.
 */
int close_db(FILE *f);

#endif /* _SENSOR_DB_H_ */
