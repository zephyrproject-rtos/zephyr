/*
 * Copyright (c) 2019, Linaro Limited
 * Copyright (c) 2024-2025, tinyVision.ai Inc.
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/video.h>

#include "video_device.h"

LOG_MODULE_REGISTER(video_buffer, CONFIG_VIDEO_LOG_LEVEL);

#if defined(CONFIG_VIDEO_BUFFER_USE_SHARED_MULTI_HEAP)
#include <zephyr/multi_heap/shared_multi_heap.h>

#define VIDEO_COMMON_HEAP_ALLOC(align, size, timeout)                                              \
	shared_multi_heap_aligned_alloc(CONFIG_VIDEO_BUFFER_SMH_ATTRIBUTE, align, size)
#define VIDEO_COMMON_FREE(block) shared_multi_heap_free(block)
#else
K_HEAP_DEFINE(video_buffer_pool, CONFIG_VIDEO_BUFFER_POOL_SZ_MAX * CONFIG_VIDEO_BUFFER_POOL_NUM_MAX);
#define VIDEO_COMMON_HEAP_ALLOC(align, size, timeout)                                              \
	k_heap_aligned_alloc(&video_buffer_pool, align, size, timeout);
#define VIDEO_COMMON_FREE(block) k_heap_free(&video_buffer_pool, block)
#endif

static struct video_buffer video_buf[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX];

static K_MUTEX_DEFINE(video_buffer_mutex);

/*
 * Find "count" contiguously availalble buffer slots in the video buffer pool.
 * Return the index of the 1st slot or -1 if failed.
 */
static int find_contig_free_buffers(uint8_t count)
{
	uint8_t idx = 0, start = 0;

	for (uint16_t i = 0; i < ARRAY_SIZE(video_buf); i++) {
		if (video_buf[i].buffer == NULL) {
			if (idx == 0) {
				start = i;
			}
			if (++idx == count) {
				return (int)start;
			}
		} else {
			idx = 0;
		}
	}

	return -1;
}

/* Release a buffer via its index */
static void release_buffer(uint16_t idx)
{
	if (video_buf[idx].buffer != NULL) {
		if (video_buf[idx].memory == VIDEO_MEMORY_INTERNAL) {
			VIDEO_COMMON_FREE(video_buf[idx].buffer);
		}
		video_buf[idx].buffer = NULL;
	}

	video_buf[idx].memory = 0;
	video_buf[idx].size = 0;
	video_buf[idx].bytesused = 0;
	video_buf[idx].timestamp = 0;
}

/* Release a range [start, start + count) of buffers */
static void release_buffers_range(uint16_t start, uint8_t count)
{
	for (uint8_t i = 0; i < count; i++) {
		uint16_t idx = start + i;
		if (idx >= CONFIG_VIDEO_BUFFER_POOL_NUM_MAX) {
			break;
		}
		release_buffer(idx);
	}
}

int video_release_buffers(uint16_t start_idx, uint8_t count)
{
	if (count == 0 || start_idx >= CONFIG_VIDEO_BUFFER_POOL_NUM_MAX ||
	    start_idx + count > CONFIG_VIDEO_BUFFER_POOL_NUM_MAX) {
		return -EINVAL;
	}

	k_mutex_lock(&video_buffer_mutex, K_FOREVER);

	release_buffers_range(start_idx, count);

	k_mutex_unlock(&video_buffer_mutex);

	return 0;
}

int video_request_buffers(struct video_buffer_request *const vbr)
{
	if (vbr == NULL || vbr->size == 0 || vbr->count == 0 ||
	    vbr->size > CONFIG_VIDEO_BUFFER_POOL_SZ_MAX ||
	    vbr->count > CONFIG_VIDEO_BUFFER_POOL_NUM_MAX ||
	    (vbr->memory != VIDEO_MEMORY_INTERNAL && vbr->memory != VIDEO_MEMORY_EXTERNAL)) {
		return -EINVAL;
	}

	uint16_t idx;
	int start_idx = 0;
	uint8_t *mem = NULL;

	k_mutex_lock(&video_buffer_mutex, K_FOREVER);

	start_idx = find_contig_free_buffers(vbr->count);

	if (start_idx < 0) {
		k_mutex_unlock(&video_buffer_mutex);

		return -ENOBUFS;
	}

	for (uint8_t i = 0; i < vbr->count; i++) {
		if (vbr->memory == VIDEO_MEMORY_INTERNAL) {
			mem = VIDEO_COMMON_HEAP_ALLOC(CONFIG_VIDEO_BUFFER_POOL_ALIGN, vbr->size,
						      vbr->timeout);
			if (mem == NULL) {
				release_buffers_range((uint16_t)start_idx, i);
				k_mutex_unlock(&video_buffer_mutex);

				return -ENOMEM;
			}
		}

		idx = (uint16_t)start_idx + i;

		video_buf[idx].index = (uint16_t)idx;
		video_buf[idx].buffer = mem;
		video_buf[idx].memory = vbr->memory;
		video_buf[idx].size = vbr->size;
		video_buf[idx].bytesused = 0;
		video_buf[idx].timestamp = 0;
	}

	k_mutex_unlock(&video_buffer_mutex);

	vbr->start_index = (uint16_t)start_idx;

	return 0;
}

int video_enqueue(const struct device *const dev, const struct video_buffer *const buf)
{
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(buf != NULL);

	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api == NULL || api->enqueue == NULL) {
		return -ENOSYS;
	}

	video_buf[buf->index].type = buf->type;

	if (video_buf[buf->index].memory == VIDEO_MEMORY_EXTERNAL) {
		video_buf[buf->index].buffer = buf->buffer;
	}

	int ret = api->enqueue(dev, &video_buf[buf->index]);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
