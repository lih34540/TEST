/**
 * \author {AUTHOR}
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_
#define PORT 5678

#include <stdint.h>
#include <time.h>
typedef uint16_t room_id_t;
typedef uint16_t sensor_id_t;
typedef double sensor_value_t;
typedef time_t sensor_ts_t;         // UTC timestamp as returned by time() - notice that the size of time_t is different on 32/64 bit machine

typedef struct {
    sensor_id_t id;
    sensor_value_t value;
    sensor_ts_t ts;
} sensor_data_t;

int write_to_log_process(char *msg);
int create_log_process(void);
int end_log_process(void);
#endif /* _CONFIG_H_ */
