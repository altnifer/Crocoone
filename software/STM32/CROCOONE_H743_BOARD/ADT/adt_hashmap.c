/*
 * adt_hashmap.c
 *
 *  Created on: Jul 22, 2024
 *      Author: nikit
 */
#include "adt_hashmap.h"
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#define FILL_FACTOR 0.7f

static const uint32_t hashmap_sizes[] = {
            5,        11,        23,        47,         97,        193,
          389,       769,      1543,      3072,       3079,      12289,
        24593,     49157,     98317,    196613,     393241,     786433,
      1572869,   3145739,   6291469,  12582917,   25165843,   50331653,
    100663319, 201326611, 402653189, 805306457, 1610612736, 2147483629
};
static const uint8_t hashmap_sizes_count = sizeof(hashmap_sizes) / sizeof(uint32_t);

typedef struct _hashmap_list_node_t {
    hitem_t hitem_data;
    struct _hashmap_list_node_t * next;
} hashmap_list_node_t;

typedef hashmap_list_node_t* hashmap_list_t;

struct _hashmap_t {
    hashmap_list_t *elements;
    uint32_t hitem_count;
    uint8_t size_index;
};

uint32_t get_hash(uint8_t *key, uint32_t key_size, uint32_t mod);
bool keys_compare(uint8_t *key1, uint32_t key_size1, uint8_t *key2, uint32_t key_size2);
void hashmap_create(hashmap_t **hashmap, uint8_t size_index);
void hashmap_resize(hashmap_t **hashmap);
void hashmap_list_push_node(hashmap_list_t *hashmap_list, hashmap_list_node_t *hashmap_list_node);
hashmap_list_node_t *hashmap_list_create_node(void *data, uint32_t data_size, uint8_t *key, uint32_t key_size);
void hashmap_list_rewrite_node_data(hashmap_list_node_t * node, void *data, uint32_t data_size);
void hashmap_list_free_node(hashmap_list_node_t * node);
bool hashmap_list_push(hashmap_list_t *hashmap_list, void *data, uint32_t data_size, uint8_t *key, uint32_t key_size);

void hashmap_init(hashmap_t **hashmap, uint32_t approximate_size) {
    for (uint8_t new_size_index = 0; new_size_index < hashmap_sizes_count; new_size_index++)
        if (hashmap_sizes[new_size_index] >= approximate_size)
            return hashmap_create(hashmap, new_size_index);
}

void hashmap_deinit(hashmap_t **hashmap) {
    hashmap_clear(hashmap);
    free((*hashmap)->elements);
    free(*hashmap);
    *hashmap = NULL;
}

void hashmap_clear(hashmap_t **hashmap) {
    uint32_t hashmap_size = hashmap_sizes[(*hashmap)->size_index];
    for (uint32_t i = 0; i < hashmap_size; i++) {
        hashmap_list_node_t *next_node_p = *((*hashmap)->elements + i);
        *((*hashmap)->elements + i) = NULL;
        while (next_node_p) {
            hashmap_list_node_t *curr_node_p = next_node_p;
            next_node_p = next_node_p->next;
            hashmap_list_free_node(curr_node_p);
        }
    }
}

uint32_t hashmap_item_count(hashmap_t **hashmap) {
    return (*hashmap)->hitem_count;
}

void hashmap_push(hashmap_t **hashmap, void *data, uint32_t data_size, uint8_t *key, uint32_t key_size) {
    uint32_t hash = get_hash(key, key_size, hashmap_sizes[(*hashmap)->size_index]);
    if (hashmap_list_push((*hashmap)->elements + hash, data, data_size, key, key_size))
        (*hashmap)->hitem_count++;

    if (
        (float)(*hashmap)->hitem_count / (float)hashmap_sizes[(*hashmap)->size_index] > FILL_FACTOR
        && (*hashmap)->size_index < hashmap_sizes_count
    )
        hashmap_resize(hashmap);
}

void hashmap_pop(hashmap_t **hashmap, uint8_t *key, uint32_t key_size) {
    uint32_t hash = get_hash(key, key_size, hashmap_sizes[(*hashmap)->size_index]);

    hashmap_list_node_t *prev_node_p = *((*hashmap)->elements + hash);
    if (!prev_node_p)
        return;

    if (keys_compare(prev_node_p->hitem_data.key, prev_node_p->hitem_data.key_size, key, key_size)) {
        *((*hashmap)->elements + hash) = prev_node_p->next;
        hashmap_list_free_node(prev_node_p);
        return;
    }

    hashmap_list_node_t *curr_node_p = prev_node_p->next;
    while (curr_node_p) {
        if (keys_compare(curr_node_p->hitem_data.key, curr_node_p->hitem_data.key_size, key, key_size)) {
            prev_node_p->next = curr_node_p->next;
            hashmap_list_free_node(curr_node_p);
            return;
        }
        prev_node_p = prev_node_p->next;
        curr_node_p = curr_node_p->next;
    }
}

hitem_t *hashmap_get(hashmap_t **hashmap, uint8_t *key, uint32_t key_size) {
    uint32_t hash = get_hash(key, key_size, hashmap_sizes[(*hashmap)->size_index]);

    hashmap_list_node_t *head = *((*hashmap)->elements + hash);
    while (head) {
        if (keys_compare(head->hitem_data.key, head->hitem_data.key_size, key, key_size))
            return &(head->hitem_data);
        head = head->next;
    }

    return NULL;
}

void hashmap_traverse(hashmap_t **hashmap, void (* pfun)(hitem_t *hitem)) {
    for (uint32_t i = 0; i < hashmap_sizes[(*hashmap)->size_index]; i++) {
        hashmap_list_node_t *head = *((*hashmap)->elements + i);
        while (head) {
            pfun(&(head->hitem_data));
            head = head->next;
        }
    }
}



uint32_t get_hash(uint8_t *key, uint32_t key_size, uint32_t mod) {
    //k - a prime number larger than the alphabet size (uint8_t size)
    static const uint32_t k = 761;

    //  hash = (key[0] * k^n + key[1] * k^(n - 1) + ... + key[n]) % mod
    //  n = key_size - 1
    uint32_t hash = 0;
    for (int i = 0; i < key_size; i++)
        hash = (hash * k + (int32_t)key[i]) % mod;
    return hash;
}

bool keys_compare(uint8_t *key1, uint32_t key_size1, uint8_t *key2, uint32_t key_size2) {
    if (key_size1 != key_size2)
        return false;
    for (uint32_t i = 0; i < key_size1; i++)
        if (key1[i] != key2[i])
            return false;
    return true;
}

void hashmap_create(hashmap_t **hashmap, uint8_t size_index) {
    uint32_t hashmap_size = hashmap_sizes[size_index];
    (*hashmap) = (hashmap_t *)malloc(sizeof(hashmap_t));
    (*hashmap)->elements = (hashmap_list_t *)malloc(hashmap_size * sizeof(hashmap_list_t));
    for (uint32_t i = 0; i < hashmap_size; i++)
        *((*hashmap)->elements + i) = NULL;
    (*hashmap)->hitem_count = 0;
    (*hashmap)->size_index = size_index;
}

void hashmap_resize(hashmap_t **hashmap) {
    uint8_t new_size_index = (*hashmap)->size_index + 1;
    hashmap_t *resized_hashmap;
    hashmap_create(&resized_hashmap, new_size_index);

    uint32_t hashmap_size = hashmap_sizes[(*hashmap)->size_index];
    for (uint32_t i = 0; i < hashmap_size; i++) {
        hashmap_list_node_t *next_node_p = *((*hashmap)->elements + i);
        while (next_node_p) {
            hashmap_list_node_t *curr_node_p = next_node_p;
            next_node_p = next_node_p->next;
            uint32_t hash = get_hash(curr_node_p->hitem_data.key, curr_node_p->hitem_data.key_size, hashmap_sizes[new_size_index]);
            hashmap_list_push_node(resized_hashmap->elements + hash, curr_node_p);
        }
    }
    resized_hashmap->hitem_count = (*hashmap)->hitem_count;
    resized_hashmap->size_index = new_size_index;

    free((*hashmap)->elements);
    free((*hashmap));

    *hashmap = resized_hashmap;
}

void hashmap_list_push_node(hashmap_list_t *hashmap_list, hashmap_list_node_t *hashmap_list_node) {
    hashmap_list_node_t *head = *hashmap_list;
    hashmap_list_node->next = NULL;
    if (!head) {
        *hashmap_list = hashmap_list_node;
        return;
    }
    while (head->next)
        head = head->next;
    head->next = hashmap_list_node;
}

hashmap_list_node_t *hashmap_list_create_node(void *data, uint32_t data_size, uint8_t *key, uint32_t key_size) {
    hashmap_list_node_t *new_list_node = (hashmap_list_node_t *)malloc(sizeof(hashmap_list_node_t));
    if (!new_list_node)
    	exit(2);

    new_list_node->hitem_data.data = (void *)malloc(data_size);
    if (!new_list_node->hitem_data.data)
        exit(2);
    memcpy(new_list_node->hitem_data.data, data, data_size);
    new_list_node->hitem_data.data_size = data_size;

    new_list_node->hitem_data.key = (uint8_t *)malloc(key_size);
    if (!new_list_node->hitem_data.key)
        exit(2);
    memcpy(new_list_node->hitem_data.key, key, key_size);
    new_list_node->hitem_data.key_size = key_size;

    new_list_node->next = NULL;
    return new_list_node;
}

void hashmap_list_rewrite_node_data(hashmap_list_node_t * node, void *data, uint32_t data_size) {
    if (node->hitem_data.data_size == data_size) {
        memcpy(node->hitem_data.data, data, data_size);
        return;
    }
    free(node->hitem_data.data);
    node->hitem_data.data = (void *)malloc(data_size);
    if (!node->hitem_data.data)
        exit(2);
    memcpy(node->hitem_data.data, data, data_size);
    node->hitem_data.data_size = data_size;
}

void hashmap_list_free_node(hashmap_list_node_t * node) {
    free(node->hitem_data.data);
    free(node->hitem_data.key);
    free(node);
}

bool hashmap_list_push(hashmap_list_t *hashmap_list, void *data, uint32_t data_size, uint8_t *key, uint32_t key_size) {
    hashmap_list_node_t *head = *hashmap_list;
    if (!head) {
        *hashmap_list = hashmap_list_create_node(data, data_size, key, key_size);
        return true;
    }

    while (1) {
        if (keys_compare(head->hitem_data.key, head->hitem_data.key_size, key, key_size)) {
            hashmap_list_rewrite_node_data(head, data, data_size);
            return false;
        }

        if (!head->next)
            break;
        else
            head = head->next;
    }
    head->next = hashmap_list_create_node(data, data_size, key, key_size);
    return true;
}
