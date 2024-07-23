/*
 * adt_list.c
 *
 *  Created on: Jul 18, 2024
 *      Author: nikit
 */
#include "adt_list.h"
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

typedef struct _list_node_t {
    item_t item;
    struct _list_node_t * next;
} list_node_t;

struct _list_t {
    list_node_t *head;
    list_node_t *tail;
    uint16_t item_count;
};

void remove_node(list_node_t *node);
list_node_t *get_Nth_node(list_t **list, uint16_t index);

void list_init(list_t ** list) {
    (*list) = (list_t *)malloc(sizeof(list_t));
    if ((*list) == NULL)
        exit(1);
    (*list)->head = NULL;
    (*list)->tail = NULL;
    (*list)->item_count = 0;
}

void list_deinit(list_t ** list) {
    list_clear(list);
    free(*list);
    (*list) = NULL;
}

void list_clear(list_t **list) {
    list_node_t * psave;
    while ((*list)->head != NULL) {
        psave = (*list)->head->next;
        remove_node((*list)->head);
        (*list)->head = psave;
    }
    (*list)->head = NULL;
    (*list)->tail = NULL;
    (*list)->item_count = 0;
}

uint16_t list_get_len(list_t **list) {
    return (*list)->item_count;
}

void list_push_back(list_t **list, void *data, uint32_t data_size) {
    list_node_t * newElement = (list_node_t*)malloc(sizeof(list_node_t));
    if (newElement == NULL)
        exit(1);
    newElement->item.data = (void *)malloc(data_size);
    if (newElement->item.data == NULL)
        exit(1);
    memcpy(newElement->item.data, data, data_size);
    newElement->item.size = data_size;

    newElement->next = NULL;
    if ((*list)->head == NULL) {
        (*list)->head = newElement;
        (*list)->tail = newElement;
    } else {
        (*list)->tail->next = newElement;
        (*list)->tail = (*list)->tail->next;
    }
    (*list)->item_count++;
}

void list_push_front(list_t **list, void *data, uint32_t data_size) {
    list_node_t * newElement = (list_node_t*)malloc(sizeof(list_node_t));
    if (newElement == NULL)
        exit(1);
    newElement->item.data = (void *)malloc(data_size);
    if (newElement->item.data == NULL)
        exit(1);
    memcpy(newElement->item.data, data, data_size);
    newElement->item.size = data_size;

    if ((*list)->head == NULL) {
        (*list)->head = newElement;
        (*list)->tail = newElement;
        newElement->next = NULL;
    } else {
        newElement->next = (*list)->head;
        (*list)->head = newElement;
    }
    (*list)->item_count++;
}

void list_pop_back(list_t **list) {
    if ((*list)->head == (*list)->tail)
        return list_clear(list);
    list_node_t * new_tail = get_Nth_node(list, list_get_len(list) - 2);
    remove_node(new_tail->next);
    new_tail->next = NULL;
    (*list)->item_count--;
}

void list_pop_front(list_t **list) {
    if ((*list)->head == (*list)->tail) {
        list_clear(list);
        return;
    }
    list_node_t *head = (*list)->head;
    (*list)->head = (*list)->head->next;
    remove_node(head);
    (*list)->item_count--;
}

item_t *list_get_Nth_item(list_t **list, uint16_t index) {
    if (!list_get_len(list))
        return NULL;

    return &(get_Nth_node(list, index)->item);
}

void list_remove_Nth(list_t **list, uint16_t index) {
    if (!index || list_get_len(list) < 2)
        return list_pop_front(list);

    list_node_t *prev_node = get_Nth_node(list, index - 1);
    list_node_t *node = prev_node->next;
    if (node == NULL)
        return list_pop_back(list);
    prev_node->next = node->next;
    remove_node(node);

    (*list)->item_count--;
}

void list_insert(list_t **list, uint16_t index, void *data, uint32_t data_size) {
    if (!index || !list_get_len(list)) {
        list_push_front(list, data, data_size);
        return;
    }

    list_node_t *prev_node = get_Nth_node(list, index - 1);
    list_node_t *node = prev_node->next;

    list_node_t * newElement = (list_node_t*)malloc(sizeof(list_node_t));
    if (newElement == NULL)
        exit(1);
    newElement->item.data = (void *)malloc(data_size);
    memcpy(newElement->item.data, data, data_size);
    if (newElement->item.data == NULL)
        exit(1);
    newElement->item.size = data_size;

    newElement->next = node;
    prev_node->next = newElement;

    (*list)->item_count++;
}

void list_traverse(list_t **list, void (* pfun)(item_t item)) {
    list_node_t * head = (*list)->head;
    while (head) {
        pfun(head->item);
        head = head->next;
    }
}

item_t *list_get_next(list_t **list, uint16_t index) {
    static list_t **prev_list;
    static uint16_t prev_index = 0;
    static list_node_t *prev_head = NULL;

    if (prev_head && prev_index == index - 1 && prev_list == list) {
        prev_head = prev_head->next;
        prev_index = index;
    }
    prev_head = get_Nth_node(list, index);

    return (prev_head) ? &(prev_head->item) : NULL;
}

list_t *list_copy(list_t **list) {
    list_node_t *head = (*list)->head;
    list_t *new_list;
    list_init(&new_list);
    while (head) {
        list_push_back(&new_list, head->item.data, head->item.size);
        head = head->next;
    }
    return new_list;
}

void list_merge(list_t **final_list, list_t **source_list) {
    (*final_list)->tail->next = (*source_list)->head;
    (*final_list)->tail = (*source_list)->tail;
    (*final_list)->item_count += (*source_list)->item_count;
    free(*source_list);
    (*source_list) = NULL;
}

void list_filter(list_t **list, bool(* condition)(item_t item)) {
    list_node_t *prev_node = (*list)->head;
    list_node_t *curr_node = prev_node;

    while (curr_node) {
        if (!condition(curr_node->item)) {
            if (curr_node == prev_node) {
                list_pop_front(list);
                prev_node = (*list)->head;
                curr_node = prev_node;
            } else {
                prev_node->next = curr_node->next;
                remove_node(curr_node);
                curr_node = prev_node;
                (*list)->item_count--;
            }
        } else {
            if (curr_node == prev_node) {
                curr_node = (curr_node->next) ? curr_node->next : NULL;
            } else {
                prev_node = prev_node->next;
                curr_node = (prev_node) ? prev_node->next : NULL;
            }
        }
    }
}



void remove_node(list_node_t *node) {
    free(node->item.data);
    free(node);
    node = NULL;
}

list_node_t *get_Nth_node(list_t **list, uint16_t index) {
    list_node_t *head = (*list)->head;
    uint16_t i = 0;

    if (!head)
        return NULL;

    while (head->next && i < index) {
        head = head->next;
        i++;
    }

    return head;
}
