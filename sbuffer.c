/**
 * \author {AUTHOR}
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "sbuffer.h"

/**
 * basic node for the buffer, these nodes are linked together to create the buffer
 */
typedef struct sbuffer_node {
    struct sbuffer_node *next;   /**< a pointer to the next node*/
    sensor_data_t data;          /**< a structure containing the data */
    int read_by[CONSUMER_COUNT]; /**< read flags: 0/1 for each consumer */
} sbuffer_node_t;

/**
 * a structure to keep track of the buffer
 */
struct sbuffer {
    sbuffer_node_t *head;      /**< a pointer to the first node in the buffer */
    sbuffer_node_t *tail;      /**< a pointer to the last node in the buffer */
    pthread_mutex_t mutex;     // lock
    pthread_cond_t cond;       // wait
};

static int all_consumers_read(sbuffer_node_t *node)
{
    for (int i = 0; i < CONSUMER_COUNT; i++) {
        if (node->read_by[i] == 0) return 0;
    }
    return 1;
}

/**
 * Find the first node that has not been read by this consumer yet.
 * Return NULL if none exists.
 */
static sbuffer_node_t *find_unread_node_for_consumer(sbuffer_t *buffer, int consumer_id)
{
    sbuffer_node_t *cur = buffer->head;
    while (cur != NULL) {
        if (cur->read_by[consumer_id] == 0) return cur;
        cur = cur->next;
    }
    return NULL;
}

/**
 * Remove nodes from the head that are already read by all consumers.
 * This keeps the buffer clean.
 */
static void cleanup_head(sbuffer_t *buffer)
{
    while (buffer->head != NULL && all_consumers_read(buffer->head)) {
        sbuffer_node_t *old = buffer->head;
        buffer->head = buffer->head->next;

        if (buffer->head == NULL) {
            buffer->tail = NULL;
        }
        free(old);
    }
}

int sbuffer_init(sbuffer_t **buffer)
{
    *buffer = malloc(sizeof(sbuffer_t));
    if (*buffer == NULL) return SBUFFER_FAILURE;

    (*buffer)->head = NULL;
    (*buffer)->tail = NULL;

    pthread_mutex_init(&(*buffer)->mutex, NULL);   // init lock
    pthread_cond_init(&(*buffer)->cond, NULL);     // init cond

    return SBUFFER_SUCCESS;
}

int sbuffer_free(sbuffer_t **buffer)
{
    sbuffer_node_t *dummy;

    if ((buffer == NULL) || (*buffer == NULL)) {
        return SBUFFER_FAILURE;
    }

    pthread_mutex_lock(&(*buffer)->mutex);         // lock

    while ((*buffer)->head)
    {
        dummy = (*buffer)->head;
        (*buffer)->head = (*buffer)->head->next;
        free(dummy);
    }
    (*buffer)->tail = NULL;

    pthread_mutex_unlock(&(*buffer)->mutex);       // unlock

    pthread_mutex_destroy(&(*buffer)->mutex);      // destroy lock
    pthread_cond_destroy(&(*buffer)->cond);        // destroy cond

    free(*buffer);
    *buffer = NULL;

    return SBUFFER_SUCCESS;
}

int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data, int consumer_id)
{
    if (buffer == NULL || data == NULL) return SBUFFER_FAILURE;
    if (consumer_id < 0 || consumer_id >= CONSUMER_COUNT) return SBUFFER_FAILURE;

    pthread_mutex_lock(&buffer->mutex);            // lock

    while (1)
    {
        // first, clean up fully-consumed head nodes
        cleanup_head(buffer);

        // find a node not read yet by this consumer
        sbuffer_node_t *node = find_unread_node_for_consumer(buffer, consumer_id);
        if (node != NULL) {
            // deliver data
            *data = node->data;

            // mark as read for this consumer
            node->read_by[consumer_id] = 1;

            // after marking, maybe head can be cleaned (if this was the head)
            cleanup_head(buffer);

            pthread_mutex_unlock(&buffer->mutex);
            return SBUFFER_SUCCESS;
        }

        // no unread data for this consumer => wait
        pthread_cond_wait(&buffer->cond, &buffer->mutex);
    }
}

int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data)
{
    sbuffer_node_t *dummy;

    if (buffer == NULL || data == NULL) return SBUFFER_FAILURE;

    dummy = malloc(sizeof(sbuffer_node_t));
    if (dummy == NULL) return SBUFFER_FAILURE;

    dummy->data = *data;
    dummy->next = NULL;
    for (int i = 0; i < CONSUMER_COUNT; i++) {
        dummy->read_by[i] = 0;
    }

    pthread_mutex_lock(&buffer->mutex);            // lock

    if (buffer->tail == NULL) // buffer empty
    {
        buffer->head = buffer->tail = dummy;
    }
    else // buffer not empty
    {
        buffer->tail->next = dummy;
        buffer->tail = dummy;
    }

    // wake up waiting consumers (both may need to wake)
    pthread_cond_broadcast(&buffer->cond);
    pthread_mutex_unlock(&buffer->mutex);          // unlock

    return SBUFFER_SUCCESS;
}
