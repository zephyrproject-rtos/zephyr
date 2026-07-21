/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/ring_buffer.h>

static struct ring_buf tracing_ring_buf;
static uint8_t tracing_buffer[CONFIG_TRACING_BUFFER_SIZE + 1];
static uint8_t tracing_cmd_buffer[CONFIG_TRACING_CMD_BUFFER_SIZE];

uint32_t tracing_cmd_buffer_alloc(uint8_t **data)
{
	*data = &tracing_cmd_buffer[0];

	return sizeof(tracing_cmd_buffer);
}

uint32_t tracing_buffer_put_claim(uint8_t **data, uint32_t size)
{
	return ring_buf_put_claim(&tracing_ring_buf, data, size);
}

int tracing_buffer_put_finish(uint32_t size)
{
	return ring_buf_put_finish(&tracing_ring_buf, size);
}

uint32_t tracing_buffer_put(uint8_t *data, uint32_t size)
{
	return ring_buf_put(&tracing_ring_buf, data, size);
}

uint32_t tracing_buffer_get_claim(uint8_t **data, uint32_t size)
{
	return ring_buf_get_claim(&tracing_ring_buf, data, size);
}

int tracing_buffer_get_finish(uint32_t size)
{
	return ring_buf_get_finish(&tracing_ring_buf, size);
}

uint32_t tracing_buffer_get(uint8_t *data, uint32_t size)
{
	return ring_buf_get(&tracing_ring_buf, data, size);
}

void tracing_buffer_init(void)
{
	ring_buf_init(&tracing_ring_buf,
		      sizeof(tracing_buffer), tracing_buffer);
}

bool tracing_buffer_is_empty(void)
{
	return ring_buf_is_empty(&tracing_ring_buf);
}

uint32_t tracing_buffer_capacity_get(void)
{
	return ring_buf_capacity_get(&tracing_ring_buf);
}

uint32_t tracing_buffer_space_get(void)
{
	return ring_buf_space_get(&tracing_ring_buf);
}
