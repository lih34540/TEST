/**
 * \author {AUTHOR}
 */

#ifndef _SBUFFER_H_
#define _SBUFFER_H_

#include "config.h"

#define SBUFFER_FAILURE -1
#define SBUFFER_SUCCESS 0
#define SBUFFER_NO_DATA 1

// consumer ids (simple and clear)
#define CONSUMER_DATAMGR  0
#define CONSUMER_STORAGE  1
#define CONSUMER_COUNT    2   // total number of consumers

typedef struct sbuffer sbuffer_t;

/**
 * Allocates and initializes a new shared buffer
 */
int sbuffer_init(sbuffer_t **buffer);

/**
 * All allocated resources are freed and cleaned up
 */
int sbuffer_free(sbuffer_t **buffer);

/**
 * Removes the first sensor data in 'buffer' (at the 'head') and returns this sensor data as '*data'
 * This buffer supports TWO consumers: datamgr and storage.
 * Each data item will be delivered to BOTH consumers. Only after both consumers read it,
 * the item is removed from the buffer.
 *
 * consumer_id: use CONSUMER_DATAMGR or CONSUMER_STORAGE
 */
int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data, int consumer_id);

/**
 * Inserts the sensor data in 'data' at the end of 'buffer' (at the 'tail')
 */
int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data);

#endif  //_SBUFFER_H_
