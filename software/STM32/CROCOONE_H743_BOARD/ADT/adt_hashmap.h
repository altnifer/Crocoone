/*
 * adt_hashmap.h
 *
 *  Created on: Jul 22, 2024
 *      Author: nikit
 */
#ifndef ADT_HASHMAP_H_
#define ADT_HASHMAP_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct _hitem_t {
    uint8_t *key;
    uint32_t key_size;
    void *data;
    uint32_t data_size;
} hitem_t;

typedef struct _hashmap_t hashmap_t;

void hashmap_init(hashmap_t **hashmap, uint32_t approximate_size);
void hashmap_deinit(hashmap_t **hashmap);
void hashmap_clear(hashmap_t **hashmap);
uint32_t hashmap_item_count(hashmap_t **hashmap);
void hashmap_push(hashmap_t **hashmap, void *data, uint32_t data_size, uint8_t *key, uint32_t key_size);
void hashmap_pop(hashmap_t **hashmap, uint8_t *key, uint32_t key_size);
hitem_t *hashmap_get(hashmap_t **hashmap, uint8_t *key, uint32_t key_size);
void hashmap_traverse(hashmap_t **hashmap, void (* pfun)(hitem_t *hitem));

#endif /* ADT_HASHMAP_H_ */
