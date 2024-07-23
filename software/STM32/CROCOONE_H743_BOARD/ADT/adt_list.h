/*
 * adt_list.h
 *
 *  Created on: Jul 18, 2024
 *      Author: nikit
 */

#ifndef INC_ADT_LIST_H_
#define INC_ADT_LIST_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct _item_t {
    void *data;
    uint32_t size;
} item_t;

typedef struct _list_t list_t;

void list_init(list_t **list);
void list_deinit(list_t ** list);
void list_push_back(list_t **list, void *data, uint32_t data_size);
void list_push_front(list_t **list, void *data, uint32_t data_size);
void list_insert(list_t **list, uint16_t index, void *data, uint32_t data_size);
void list_pop_back(list_t **list);
void list_pop_front(list_t **list);
void list_remove_Nth(list_t **list, uint16_t index);
void list_clear(list_t **list);
item_t *list_get_Nth_item(list_t **list, uint16_t index);
uint16_t list_get_len(list_t **list);
void list_traverse(list_t **list, void (* pfun)(item_t item));
item_t *list_get_next(list_t **list, uint16_t index);
list_t *list_copy(list_t **list);
void list_merge(list_t **final_list, list_t **source_list);
void list_filter(list_t **list, bool(* condition)(item_t item));

#endif /* INC_ADT_LIST_H_ */
