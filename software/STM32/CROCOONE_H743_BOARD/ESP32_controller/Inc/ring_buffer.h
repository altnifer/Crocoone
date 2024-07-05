#ifndef INC_RING_BUFFER_H
#define INC_RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_BUFF_POVER 16
#define MIN_BUFF_POVER 4

typedef struct {
    uint8_t *buffer;
    uint16_t buffer_mask;

    uint16_t head_indx;
    uint16_t tail_indx;

    uint16_t bytes_to_read;
    uint16_t bytes_to_write;

    bool readFlag;
    bool writeFlag;
} ring_buffer_t; 

bool ringBuffer_init(ring_buffer_t *buffer, uint8_t poverOf2);

bool ringBuffer_deInit(ring_buffer_t *buffer);

void ringBuffer_clear(ring_buffer_t *buffer);

uint16_t ringBuffer_getBytesToWrite(ring_buffer_t *buffer);

uint16_t ringBuffer_getBytesToRead(ring_buffer_t *buffer);

bool ringBuffer_add(ring_buffer_t *buffer, uint8_t *data_buff, uint16_t data_buff_size);

int32_t ringBuffer_findSequence(ring_buffer_t *buffer, uint8_t *data_buff, uint16_t data_buff_size);

void ringBuffer_get(ring_buffer_t *buffer, uint8_t *data_buff, uint16_t bytes_to_get);

void ringBuffer_clearNBytes(ring_buffer_t *buffer, uint16_t n);

#endif
