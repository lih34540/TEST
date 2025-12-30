#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "config.h"
#include "sbuffer.h"
#include "datamgr.h"
#include "sensor_db.h"
#include "connmgr.h"

/* shared buffer */
static sbuffer_t *buffer = NULL;

static void push_termination_marker(void)
{
    sensor_data_t end;
    end.id = 0;
    end.value = 0;
    end.ts = 0;
    sbuffer_insert(buffer, &end);
}

static void *storage_manager_thread(void *arg)
{
    sbuffer_t *buf = (sbuffer_t *)arg;
    sensor_data_t data;

    FILE *fp = open_db("data.csv", false);
    if (fp == NULL) {
        write_to_log_process("Error: cannot open data.csv\n");
        return NULL;
    }

    while (1) {

        sbuffer_remove(buf, &data, CONSUMER_STORAGE);

        if (data.id == 0)
            break;

        insert_sensor(fp, data.id, data.value, data.ts);
    }

    close_db(fp);
    return NULL;
}

int main(int argc, char *argv[])
{
    FILE *fp_map = NULL;

    pthread_t tid_datamgr;
    pthread_t tid_storage;
    pthread_t tid_connmgr;

    /* start logger once */
    if (create_log_process() != 0) {
        printf("logger start failed\n");
        return -1;
    }

    /* open map file */
    fp_map = fopen("room_sensor.map", "r");
    if (fp_map == NULL) {
        write_to_log_process("Error: cannot open room_sensor.map\n");
        end_log_process();
        return -1;
    }

    /* init datamgr (read map) */
    datamgr_init(fp_map);
    fclose(fp_map);

    /* init shared buffer */
    if (sbuffer_init(&buffer) != SBUFFER_SUCCESS) {
        write_to_log_process("Error: sbuffer_init failed\n");
        datamgr_free();
        end_log_process();
        return -1;
    }

    /* start threads */
    pthread_create(&tid_datamgr, NULL, datamgr_thread, buffer);
    pthread_create(&tid_storage, NULL, storage_manager_thread, buffer);

    /* connmgr will push data into sbuffer and return when finished */
    pthread_create(&tid_connmgr, NULL, connmgr_thread, buffer);

    /* wait for connmgr to finish */
    pthread_join(tid_connmgr, NULL);

    /* stop datamgr + storage */
    push_termination_marker();

    pthread_join(tid_datamgr, NULL);
    pthread_join(tid_storage, NULL);

    /* cleanup */
    sbuffer_free(&buffer);
    datamgr_free();

    end_log_process();
    return 0;
}
