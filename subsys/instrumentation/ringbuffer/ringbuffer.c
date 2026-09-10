/*
 * Copyright (c) 2025 Antmicro
 * Copyright (c) 2025 Analog Devices
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/ring_buffer.h>

static struct ring_buf instr_ring_buf;
static uint8_t instr_buffer[CONFIG_INSTRUMENTATION_MODE_CALLGRAPH_TRACE_BUFFER_SIZE + 1];

struct ring_buf *instr_buffer_get_ring_buf(void)
{
	return &instr_ring_buf;
}

void instr_buffer_init(void)
{
	ring_buf_init(&instr_ring_buf,
		      sizeof(instr_buffer), instr_buffer);
}
