#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "connmgr.h"
#include "sbuffer.h"
#include "config.h"
#include "lib/tcpsock.h"

typedef struct {
    tcpsock_t *client;
    sbuffer_t *buffer;
} client_arg_t;

static void *client_thread(void *arg)
{
    client_arg_t *carg = (client_arg_t *)arg;
    tcpsock_t *client = carg->client;
    sbuffer_t *buffer = carg->buffer;
    free(carg);

    sensor_data_t data;

    while (1) {
        int bytes = sizeof(sensor_data_t);
        int ret = tcp_receive(client, &data, &bytes);

        /* client closed connection or error */
        if (ret != TCP_NO_ERROR || bytes == 0) {
            break;
        }

        sbuffer_insert(buffer, &data);
    }

    tcp_close(&client);
    write_to_log_process("Sensor node connection closed\n");
    return NULL;
}

void *connmgr_thread(void *arg)
{
    sbuffer_t *buffer = (sbuffer_t *)arg;

    tcpsock_t *server = NULL;
    tcpsock_t *client = NULL;

    pthread_t tid;

    /* open server socket */
    if (tcp_passive_open(&server, PORT) != TCP_NO_ERROR) {
        write_to_log_process("Error: server socket open failed\n");
        return NULL;
    }

    write_to_log_process("Server started\n");

    while (1) {

        /* wait for incoming client */
        if (tcp_wait_for_connection(server, &client) != TCP_NO_ERROR) {
            break;
        }

        write_to_log_process("Sensor node has opened a new connection\n");

        client_arg_t *arg = malloc(sizeof(client_arg_t));
        if (arg == NULL) {
            tcp_close(&client);
            continue;
        }

        arg->client = client;
        arg->buffer = buffer;

        pthread_create(&tid, NULL, client_thread, arg);
        pthread_detach(tid);   // no join needed
    }

    tcp_close(&server);
    write_to_log_process("Server stopped\n");
    return NULL;
}
