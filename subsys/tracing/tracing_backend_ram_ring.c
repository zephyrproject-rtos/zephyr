/*
 * Copyright (c) 2021 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <kernel.h>
#include <string.h>
#include <tracing_core.h>
#include <tracing_buffer.h>
#include <tracing_backend.h>

/*
 * GDB can be used to dump data with following commands:
 *
 * set $start = ram_ring_tracing + ram_ring_tracing_write_head
 * set $end = ram_ring_tracing + sizeof(ram_ring_tracing)
 * dump binary memory /tmp/channel0_0.first $start $end
 * dump binary memory /tmp/channel0_0.second ram_ring_tracing $start
 */
static uint8_t ram_ring_tracing[CONFIG_RAM_RING_TRACING_BUFFER_SIZE];
static uint32_t ram_ring_tracing_write_head;

static void tracing_backend_ram_ring_output(
	const struct tracing_backend *backend,
	uint8_t *data, uint32_t length)
{
	if (length > sizeof(ram_ring_tracing)) {
		return;
	}

	uint32_t remaining_until_end = sizeof(ram_ring_tracing) - ram_ring_tracing_write_head;

	if (length > remaining_until_end) {
		memcpy(ram_ring_tracing + ram_ring_tracing_write_head, data, remaining_until_end);
		data += remaining_until_end;
		length -= remaining_until_end;
		ram_ring_tracing_write_head = 0;
	}

	memcpy(ram_ring_tracing + ram_ring_tracing_write_head, data, length);
	ram_ring_tracing_write_head += length;
}

static void tracing_backend_ram_ring_init(void)
{
	memset(ram_ring_tracing, 0, sizeof(ram_ring_tracing));
	ram_ring_tracing_write_head = 0;
}

const struct tracing_backend_api tracing_backend_ram_ring_api = {
	.init = tracing_backend_ram_ring_init,
	.output = tracing_backend_ram_ring_output
};

TRACING_BACKEND_DEFINE(tracing_backend_ram_ring, tracing_backend_ram_ring_api);
