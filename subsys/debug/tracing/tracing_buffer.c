/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/ring_buffer.h>

static struct ring_buf tracing_ring_buf;
static u8_t tracing_buffer[CONFIG_TRACING_BUFFER_SIZE + 1];
static u8_t tracing_cmd_buffer[CONFIG_TRACING_CMD_BUFFER_SIZE];

u32_t tracing_cmd_buffer_alloc(u8_t **data)
{
	*data = &tracing_cmd_buffer[0];

	return sizeof(tracing_cmd_buffer);
}

u32_t tracing_buffer_put_claim(u8_t **data, u32_t size)
{
	return ring_buf_put_claim(&tracing_ring_buf, data, size);
}

int tracing_buffer_put_finish(u32_t size)
{
	return ring_buf_put_finish(&tracing_ring_buf, size);
}

u32_t tracing_buffer_put(u8_t *data, u32_t size)
{
	return ring_buf_put(&tracing_ring_buf, data, size);
}

u32_t tracing_buffer_get_claim(u8_t **data, u32_t size)
{
	return ring_buf_get_claim(&tracing_ring_buf, data, size);
}

int tracing_buffer_get_finish(u32_t size)
{
	return ring_buf_get_finish(&tracing_ring_buf, size);
}

u32_t tracing_buffer_get(u8_t *data, u32_t size)
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

u32_t tracing_buffer_capacity_get(void)
{
	return ring_buf_capacity_get(&tracing_ring_buf);
}

u32_t tracing_buffer_space_get(void)
{
	return ring_buf_space_get(&tracing_ring_buf);
}
