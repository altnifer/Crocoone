#include "adt_ringBuffer.h"
#include "malloc.h"

bool ringBuffer_init(ring_buffer_t *buffer, uint8_t poverOf2) {
    if (buffer == NULL || (poverOf2 > MAX_BUFF_POVER || poverOf2 < MIN_BUFF_POVER)) return false;

    uint16_t buff_size = 1 << poverOf2;

    uint8_t *new_head = (uint8_t *)malloc(buff_size);
    if (new_head == NULL) return false;

    buffer->buffer = new_head;
    buffer->buffer_mask = buff_size - 1;

    buffer->head_indx = 0;
    buffer->tail_indx = 0;

    buffer->bytes_to_write = buff_size;
    buffer->bytes_to_read = 0;

    return true;
}

bool ringBuffer_deInit(ring_buffer_t *buffer) {
    if (buffer == NULL) return false;
    if (buffer->buffer == NULL) return false;

    free(buffer->buffer);

    buffer->buffer = NULL;
    buffer->buffer_mask = 0;

    buffer->head_indx = 0;
    buffer->tail_indx = 0;

    buffer->bytes_to_write = 0;
    buffer->bytes_to_read = 0;

    return true;
}

void ringBuffer_clear(ring_buffer_t *buffer) {
    buffer->head_indx = 0;
    buffer->tail_indx = 0;
    buffer->bytes_to_write = buffer->buffer_mask + 1;
    buffer->bytes_to_read = 0;
}

uint16_t ringBuffer_getBytesToRead(ring_buffer_t *buffer) {
    return buffer->bytes_to_read;
}

uint16_t ringBuffer_getBytesToWrite(ring_buffer_t *buffer) {
    return buffer->bytes_to_write;
}

bool ringBuffer_add(ring_buffer_t *buffer, uint8_t *data_buff, uint16_t data_buff_size) {
    if (data_buff_size > buffer->bytes_to_write) return false;

    for(uint16_t i = 0; i < data_buff_size; i++){
        buffer->buffer[(buffer->tail_indx + i) & buffer->buffer_mask] = data_buff[i];
    }
    buffer->tail_indx = (buffer->tail_indx + data_buff_size) & buffer->buffer_mask;
    buffer->bytes_to_write -= data_buff_size;
    buffer->bytes_to_read += data_buff_size;

    return true;
}

int32_t ringBuffer_findSequence(ring_buffer_t *buffer, uint8_t *data_buff, uint16_t data_buff_size) {
    uint16_t bytesToRread = buffer->bytes_to_read;
    for (uint16_t i = 0; i < bytesToRread; i++) {
        if ((bytesToRread - i) < data_buff_size) break;

        if (buffer->buffer[(buffer->head_indx + i) & buffer->buffer_mask] == *data_buff) {
            bool isSequence = true;
            for (uint16_t j = 1; j < data_buff_size; j++) {
                if (buffer->buffer[(buffer->head_indx + i + j) & buffer->buffer_mask] != data_buff[j]) {
                    isSequence = false;
                    break;
                }
            }
            if (isSequence)
                return i;
        }
    }
    return -1;
}

void ringBuffer_get(ring_buffer_t *buffer, uint8_t *data_buff, uint16_t bytes_to_get) {
    if (bytes_to_get > buffer->bytes_to_read)
        bytes_to_get = buffer->bytes_to_read;

    for(uint16_t i = 0; i < bytes_to_get; i++) {
        data_buff[i] = buffer->buffer[(buffer->head_indx + i) & buffer->buffer_mask];
    }
}

void ringBuffer_clearNBytes(ring_buffer_t *buffer, uint16_t n) {
	if (n == 0) return;

    if (n > buffer->bytes_to_read)
        n = buffer->bytes_to_read;

    buffer->head_indx = (buffer->head_indx + n) & buffer->buffer_mask;
    buffer->bytes_to_write += n;
    buffer->bytes_to_read -= n;
}
