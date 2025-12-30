#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"

struct dplist_node {
    dplist_node_t *prev, *next;
    void *element;
};

struct dplist {
    dplist_node_t *head;
    void *(*element_copy)(void *src_element);
    void (*element_free)(void **element);
    int (*element_compare)(void *x, void *y);
};

dplist_t *dpl_create(
        void *(*element_copy)(void *src_element),
        void (*element_free)(void **element),
        int (*element_compare)(void *x, void *y)
) {
    dplist_t *list = malloc(sizeof(dplist_t));
    assert(list != NULL);

    list->head = NULL;
    list->element_copy = element_copy;
    list->element_free = element_free;
    list->element_compare = element_compare;
    return list;
}

void dpl_free(dplist_t **list, bool free_element)
{
    if (list == NULL || *list == NULL) return;

    dplist_node_t *ref = (*list)->head;
    while (ref != NULL)
    {
        dplist_node_t *temp = ref;
        ref = ref->next;

        if (free_element && (*list)->element_free != NULL)
            (*list)->element_free(&(temp->element));

        free(temp);
    }
    free(*list);
    *list = NULL;
}

int dpl_size(dplist_t *list)
{
    if (list == NULL) return -1;
    int count = 0;
    dplist_node_t *ref = list->head;
    while (ref != NULL){ count++; ref = ref->next; }
    return count;
}

dplist_t *dpl_insert_at_index(dplist_t *list, void *element, int index, bool insert_copy)
{
    if (list == NULL) return NULL;

    dplist_node_t *new_node = malloc(sizeof(dplist_node_t));
    assert(new_node != NULL);

    if (insert_copy && list->element_copy != NULL)
        new_node->element = list->element_copy(element);
    else
        new_node->element = element;

    int size = dpl_size(list);
    if (list->head == NULL || index <= 0)
    {
        new_node->prev = NULL;
        new_node->next = list->head;
        if (list->head != NULL) list->head->prev = new_node;
        list->head = new_node;
        return list;
    }
    if (index >= size)
    {
        dplist_node_t *last = dpl_get_reference_at_index(list, size-1);
        new_node->next = NULL;
        new_node->prev = last;
        last->next = new_node;
        return list;
    }
    dplist_node_t *ref = dpl_get_reference_at_index(list, index);
    new_node->next = ref;
    new_node->prev = ref->prev;
    ref->prev->next = new_node;
    ref->prev = new_node;

    return list;
}

dplist_t *dpl_remove_at_index(dplist_t *list, int index, bool free_element)
{
    if (list == NULL) return NULL;
    if (list->head == NULL) return list;

    dplist_node_t *ref = dpl_get_reference_at_index(list, index);

    if (ref->prev != NULL) ref->prev->next = ref->next;
    else list->head = ref->next;

    if (ref->next != NULL) ref->next->prev = ref->prev;

    if (free_element && list->element_free != NULL)
        list->element_free(&(ref->element));

    free(ref);
    return list;
}

void *dpl_get_element_at_index(dplist_t *list, int index)
{
    dplist_node_t *ref = dpl_get_reference_at_index(list, index);
    if (ref == NULL) return NULL;
    return ref->element;
}

int dpl_get_index_of_element(dplist_t *list, void *element)
{
    if (list == NULL) return -1;
    int index = 0;
    dplist_node_t *ref = list->head;
    while (ref != NULL){
        if (list->element_compare(ref->element, element) == 0)
            return index;
        ref = ref->next;
        index++;
    }
    return -1;
}

dplist_node_t *dpl_get_reference_at_index(dplist_t *list, int index)
{
    if (list == NULL || list->head == NULL) return NULL;
    dplist_node_t *ref = list->head;
    if (index <=0) return ref;
    int i=0;
    while (ref->next != NULL && i<index) { ref=ref->next; i++; }
    return ref;
}

void *dpl_get_element_at_reference(dplist_t *list, dplist_node_t *reference)
{
    if (list == NULL || reference == NULL) return NULL;
    return reference->element;
}
