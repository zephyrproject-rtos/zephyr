/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <tracing_core.h>
#include <tracing_buffer.h>
#include <tracing_backend.h>

#include <adsp_memory.h>
#include <adsp_debug_window.h>

/* structure of memory window */
struct tracing_backend_adsp_memory_window {
	uint32_t head_offset;	/* offset of the first not used byte in data[] */
	uint8_t data[];		/* tracing data */
} __packed __aligned(8);

#define ADSP_TRACING_WINDOW_DATA_SIZE		\
	(ADSP_DW_SLOT_SIZE - offsetof(struct tracing_backend_adsp_memory_window, data))

static volatile struct tracing_backend_adsp_memory_window *mem_window;

static void tracing_backend_adsp_memory_window_output(
		const struct tracing_backend *backend,
		uint8_t *data, uint32_t length)
{
	/* copy data to ring buffer,
	 * to make FW part fast, there's no sync with the data reader
	 * the reader MUST read data before they got overwritten
	 */
	size_t to_copy = MIN(length, ADSP_TRACING_WINDOW_DATA_SIZE - mem_window->head_offset);

	memcpy((void *)(mem_window->data + mem_window->head_offset), data, to_copy);

	length -= to_copy;
	if (length) {
		memcpy((void *)mem_window->data, data + to_copy, length);
		mem_window->head_offset = length;
	} else {
		mem_window->head_offset += to_copy;
	}
}

static void tracing_backend_adsp_memory_window_init(void)
{
	volatile struct adsp_debug_window *window = ADSP_DW;

	window->descs[ADSP_DW_SLOT_NUM_TRACE].type = ADSP_DW_SLOT_TRACE;
	window->descs[ADSP_DW_SLOT_NUM_TRACE].resource_id = 0;
	mem_window = (struct tracing_backend_adsp_memory_window *)
			ADSP_DW->slots[ADSP_DW_SLOT_NUM_TRACE];

	mem_window->head_offset = 0;
}

const struct tracing_backend_api tracing_backend_adsp_memory_window_api = {
	.init = tracing_backend_adsp_memory_window_init,
	.output  = tracing_backend_adsp_memory_window_output
};

TRACING_BACKEND_DEFINE(tracing_backend_adsp_memory_window, tracing_backend_adsp_memory_window_api);
