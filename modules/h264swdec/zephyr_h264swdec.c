/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* Allocate the decoder's memory from a dedicated Zephyr heap. */

#include <stddef.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <H264SwDecApi.h>

LOG_MODULE_REGISTER(h264swdec, CONFIG_H264SWDEC_LOG_LEVEL);

K_HEAP_DEFINE(h264swdec_heap, CONFIG_H264SWDEC_HEAP_SIZE);

void *H264SwDecMalloc(u32 size, u32 num)
{
	size_t bytes;
	void *memory;

	if ((num != 0U) && (size > (SIZE_MAX / num))) {
		LOG_ERR("allocation size overflow: %u * %u", size, num);
		return NULL;
	}

	bytes = (size_t)size * num;
	memory = k_heap_alloc(&h264swdec_heap, bytes, K_NO_WAIT);
	if (memory == NULL) {
		LOG_ERR("allocation failed: %zu bytes", bytes);
	}

	return memory;
}

void H264SwDecFree(void *memory)
{
	if (memory != NULL) {
		k_heap_free(&h264swdec_heap, memory);
	}
}
