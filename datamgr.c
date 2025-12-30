#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "datamgr.h"
#include "lib/dplist.h"

typedef struct {
    sensor_id_t id;
    room_id_t room;
    sensor_value_t vals[RUN_AVG_LENGTH];
    int cnt;
    int idx;
    sensor_value_t mean;
    sensor_ts_t t;
} node_t;

static dplist_t *lst = NULL;

/* copy */
static void *cp(void *src)
{
    node_t *p = (node_t *)src;
    node_t *q = malloc(sizeof(node_t));
    if (q) *q = *p;
    return q;
}

/* free */
static void rm(void **e)
{
    if (e && *e) {
        free(*e);
        *e = NULL;
    }
}

/* compare by id */
static int cmp(void *a, void *b)
{
    node_t *x = (node_t *)a;
    node_t *y = (node_t *)b;
    return (int)(x->id - y->id);
}

/* find by id */
static node_t *find(sensor_id_t id)
{
    int n = dpl_size(lst);
    for (int i = 0; i < n; i++) {
        node_t *p = dpl_get_element_at_index(lst, i);
        if (p && p->id == id) return p;
    }
    return NULL;
}

void datamgr_init(FILE *fp_map)
{
    lst = dpl_create(cp, rm, cmp);

    room_id_t room;
    sensor_id_t sid;

    while (fscanf(fp_map, "%hu %hu", &room, &sid) == 2) {
        node_t n;

        n.id = sid;
        n.room = room;
        memset(n.vals, 0, sizeof(n.vals));
        n.cnt = 0;
        n.idx = 0;
        n.mean = 0;
        n.t = 0;

        dpl_insert_at_index(lst, &n, dpl_size(lst), true);
    }
}

void *datamgr_thread(void *arg)
{
    sbuffer_t *buffer = (sbuffer_t *)arg;
    sensor_data_t data;

    while (1) {

        /* get data from shared buffer */
        sbuffer_remove(buffer, &data, CONSUMER_DATAMGR);

        /* termination marker */
        if (data.id == 0)
            break;

        node_t *p = find(data.id);
        if (!p) {
            write_to_log_process("Warning: unknown sensor id\n");
            continue;
        }

        p->vals[p->idx] = data.value;
        p->idx = (p->idx + 1) % RUN_AVG_LENGTH;

        if (p->cnt < RUN_AVG_LENGTH)
            p->cnt++;

        double sum = 0;
        for (int i = 0; i < p->cnt; i++)
            sum += p->vals[i];

        p->mean = sum / p->cnt;
        p->t = data.ts;

        if (p->cnt == RUN_AVG_LENGTH) {
            if (p->mean < SET_MIN_TEMP)
                write_to_log_process("Warning: room too cold\n");
            if (p->mean > SET_MAX_TEMP)
                write_to_log_process("Warning: room too hot\n");
        }
    }

    return NULL;
}

sensor_value_t datamgr_get_avg(sensor_id_t id)
{
    node_t *p = find(id);
    if (!p)
        exit(EXIT_FAILURE);
    return p->mean;
}

room_id_t datamgr_get_room_id(sensor_id_t id)
{
    node_t *p = find(id);
    if (!p)
        exit(EXIT_FAILURE);
    return p->room;
}

sensor_ts_t datamgr_get_last_modified(sensor_id_t id)
{
    node_t *p = find(id);
    if (!p)
        exit(EXIT_FAILURE);
    return p->t;
}

int datamgr_get_total_sensors(void)
{
    if (!lst) return 0;
    return dpl_size(lst);
}

void datamgr_free(void)
{
    if (lst) {
        dpl_free(&lst, true);
        lst = NULL;
    }
}
