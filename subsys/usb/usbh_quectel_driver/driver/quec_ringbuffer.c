/*
 * Copyright (c) 2025 Quectel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "quec_ringbuffer.h"

void ring_buffer_init(ring_buffer_t *ring_buffer,  uint8_t *buffer, int buff_size)
{
	if(buffer == NULL)
		return ;
	
	ring_buffer->buffer = buffer;
	ring_buffer->read_offset = 0;
    ring_buffer->write_offset = 0;
    ring_buffer->valid_size = 0;
    ring_buffer->total_size = buff_size;
}

void ring_buffer_reset(ring_buffer_t *ring_buffer)
{	
	ring_buffer->read_offset = 0;
    ring_buffer->write_offset = 0;
    ring_buffer->valid_size = 0;
}

void ring_buffer_write(void *buffer_to_write, int size, ring_buffer_t *ring_buffer)
{
    int32_t write_offset = ring_buffer->write_offset;
    int32_t total_size = ring_buffer->total_size;
    int32_t first_write_size = 0;

    if (ring_buffer->valid_size + size > total_size)
    {
        return;
    }

    if (size + write_offset <= total_size)
    {
        memcpy(ring_buffer->buffer + write_offset, buffer_to_write, size);
    }
    else
    {
        first_write_size = total_size - write_offset;
        memcpy(ring_buffer->buffer + write_offset, buffer_to_write, first_write_size);
        memcpy(ring_buffer->buffer, (uint8_t *)buffer_to_write + first_write_size, size - first_write_size);
    }
    ring_buffer->write_offset += size;
    ring_buffer->write_offset %= total_size;
    ring_buffer->valid_size += size;
}

void ring_buffer_read(ring_buffer_t *ring_buffer, void *buff, int size)
{
    int32_t read_offset = ring_buffer->read_offset;
    int32_t total_size = ring_buffer->total_size;
    int32_t first_read_size = 0;

    if (size > ring_buffer->valid_size)
    {
        return;
    }

    if (total_size - read_offset >= size)
    {
        memcpy(buff, ring_buffer->buffer + read_offset, size);
    }
    else
    {
        first_read_size = total_size - read_offset;
        memcpy(buff, ring_buffer->buffer + read_offset, first_read_size);
        memcpy((uint8_t *)buff + first_read_size, ring_buffer->buffer, size - first_read_size);
    }

    ring_buffer->read_offset += size;
    ring_buffer->read_offset %= total_size;
    ring_buffer->valid_size -= size;
}

extern inline uint8_t ring_buffer_is_empty(ring_buffer_t *buffer);
extern inline uint8_t ring_buffer_is_full(ring_buffer_t *buffer);
extern inline int ring_buffer_num_items(ring_buffer_t *buffer);
extern inline int ring_buffer_free_size(ring_buffer_t *buffer);


