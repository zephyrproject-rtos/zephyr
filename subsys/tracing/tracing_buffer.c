/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/ring_buffer.h>

static struct ring_buffer tracing_ring_buf;
static uint8_t tracing_buffer[CONFIG_TRACING_BUFFER_SIZE + 1];
static uint8_t tracing_cmd_buffer[CONFIG_TRACING_CMD_BUFFER_SIZE];

uint32_t tracing_cmd_buffer_alloc(uint8_t **data)
{
	*data = &tracing_cmd_buffer[0];

	return sizeof(tracing_cmd_buffer);
}

uint32_t tracing_buffer_put_claim(uint8_t **data, uint32_t size)
{
	return MIN(ring_buffer_write_ptr(&tracing_ring_buf, data, size), size);
}

int tracing_buffer_put_finish(uint32_t size)
{
	ring_buffer_commit(&tracing_ring_buf, size);
	return 0;
}

uint32_t tracing_buffer_put(uint8_t *data, uint32_t size)
{
	return ring_buffer_write(&tracing_ring_buf, data, size);
}

uint32_t tracing_buffer_get_claim(uint8_t **data, uint32_t size)
{
	return MIN(ring_buffer_read_ptr(&tracing_ring_buf, data), size);
}

int tracing_buffer_get_finish(uint32_t size)
{
	ring_buffer_consume(&tracing_ring_buf, size);
	return 0;
}

uint32_t tracing_buffer_get(uint8_t *data, uint32_t size)
{
	return ring_buffer_read(&tracing_ring_buf, data, size);
}

void tracing_buffer_init(void)
{
	ring_buffer_init(&tracing_ring_buf, tracing_buffer, sizeof(tracing_buffer));
}

bool tracing_buffer_is_empty(void)
{
	return ring_buffer_empty(&tracing_ring_buf);
}

uint32_t tracing_buffer_capacity_get(void)
{
	return ring_buffer_capacity(&tracing_ring_buf);
}

uint32_t tracing_buffer_space_get(void)
{
	return ring_buffer_space(&tracing_ring_buf);
}
