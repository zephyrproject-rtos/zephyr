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

struct ring_buf *tracing_buffer_get_ring_buf(void)
{
	return &tracing_ring_buf;
}

void tracing_buffer_init(void)
{
	ring_buf_init(&tracing_ring_buf,
		      sizeof(tracing_buffer), tracing_buffer);
}
