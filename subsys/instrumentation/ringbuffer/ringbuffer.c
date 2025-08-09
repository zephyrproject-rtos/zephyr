/*
 * Copyright (c) 2025 Antmicro
 * Copyright (c) 2025 Analog Devices
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/ring_buffer.h>

static struct ring_buf instr_ring_buf;
static uint8_t instr_buffer[CONFIG_INSTRUMENTATION_MODE_CALLGRAPH_TRACE_BUFFER_SIZE + 1];

uint32_t instr_buffer_put_claim(uint8_t **data, uint32_t size)
{
	return ring_buf_put_claim(&instr_ring_buf, data, size);
}

int instr_buffer_put_finish(uint32_t size)
{
	return ring_buf_put_finish(&instr_ring_buf, size);
}

uint32_t instr_buffer_put(uint8_t *data, uint32_t size)
{
	return ring_buf_put(&instr_ring_buf, data, size);
}

uint32_t instr_buffer_get_claim(uint8_t **data, uint32_t size)
{
	return ring_buf_get_claim(&instr_ring_buf, data, size);
}

int instr_buffer_get_finish(uint32_t size)
{
	return ring_buf_get_finish(&instr_ring_buf, size);
}

uint32_t instr_buffer_get(uint8_t *data, uint32_t size)
{
	return ring_buf_get(&instr_ring_buf, data, size);
}

void instr_buffer_init(void)
{
	ring_buf_init(&instr_ring_buf,
		      sizeof(instr_buffer), instr_buffer);
}

bool instr_buffer_is_empty(void)
{
	return ring_buf_is_empty(&instr_ring_buf);
}

uint32_t instr_buffer_capacity_get(void)
{
	return ring_buf_capacity_get(&instr_ring_buf);
}

uint32_t instr_buffer_space_get(void)
{
	return ring_buf_space_get(&instr_ring_buf);
}
