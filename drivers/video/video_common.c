/*
 * Copyright (c) 2019, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>

#include "video_common.h"

#if defined(CONFIG_VIDEO_BUFFER_USE_SHARED_MULTI_HEAP)
#include <zephyr/multi_heap/shared_multi_heap.h>

#define VIDEO_COMMON_HEAP_ALLOC(align, size, timeout)                                              \
	shared_multi_heap_aligned_alloc(CONFIG_VIDEO_BUFFER_SMH_ATTRIBUTE, align, size)
#define VIDEO_COMMON_FREE(block) shared_multi_heap_free(block)
#else
K_HEAP_DEFINE(video_buffer_pool, CONFIG_VIDEO_BUFFER_POOL_SZ_MAX*CONFIG_VIDEO_BUFFER_POOL_NUM_MAX);
#define VIDEO_COMMON_HEAP_ALLOC(align, size, timeout)                                              \
	k_heap_aligned_alloc(&video_buffer_pool, align, size, timeout);
#define VIDEO_COMMON_FREE(block) k_heap_free(&video_buffer_pool, block)
#endif

static struct video_buffer video_buf[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX];

struct mem_block {
	void *data;
};

static struct mem_block video_block[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX];

struct video_buffer *video_buffer_aligned_alloc(size_t size, size_t align)
{
	struct video_buffer *vbuf = NULL;
	struct mem_block *block;
	int i;

	/* find available video buffer */
	for (i = 0; i < ARRAY_SIZE(video_buf); i++) {
		if (video_buf[i].buffer == NULL) {
			vbuf = &video_buf[i];
			block = &video_block[i];
			break;
		}
	}

	if (vbuf == NULL) {
		return NULL;
	}

	/* Alloc buffer memory */
	block->data = VIDEO_COMMON_HEAP_ALLOC(align, size, K_FOREVER);
	if (block->data == NULL) {
		return NULL;
	}

	vbuf->buffer = block->data;
	vbuf->size = size;
	vbuf->bytesused = 0;

	return vbuf;
}

struct video_buffer *video_buffer_alloc(size_t size)
{
	return video_buffer_aligned_alloc(size, sizeof(void *));
}

void video_buffer_release(struct video_buffer *vbuf)
{
	struct mem_block *block = NULL;
	int i;

	/* vbuf to block */
	for (i = 0; i < ARRAY_SIZE(video_block); i++) {
		if (video_block[i].data == vbuf->buffer) {
			block = &video_block[i];
			break;
		}
	}

	vbuf->buffer = NULL;
	if (block) {
		VIDEO_COMMON_FREE(block->data);
	}
}

int video_get_range_int(unsigned int cid, int *value, int min, int max, int def)
{
	switch (cid & VIDEO_CTRL_GET_MASK) {
	case VIDEO_CTRL_GET_MIN:
		*value = min;
		return 0;
	case VIDEO_CTRL_GET_MAX:
		*value = max;
		return 0;
	case VIDEO_CTRL_GET_DEF:
		*value = def;
		return 0;
	case VIDEO_CTRL_GET_CUR:
		return 1;
	default:
		return -ENOTSUP;
	}
}

int video_get_range_int64(unsigned int cid, int64_t *value, int64_t min, int64_t max, int64_t def)
{
	switch (cid & VIDEO_CTRL_GET_MASK) {
	case VIDEO_CTRL_GET_MIN:
		*value = min;
		return 0;
	case VIDEO_CTRL_GET_MAX:
		*value = max;
		return 0;
	case VIDEO_CTRL_GET_DEF:
		*value = def;
		return 0;
	case VIDEO_CTRL_GET_CUR:
		return 1;
	default:
		return -ENOTSUP;
	}
}

int video_check_range_int(const struct device *dev, unsigned int cid, int value)
{
	int min;
	int max;
	int ret;

	ret = video_get_ctrl(dev, cid | VIDEO_CTRL_GET_MIN, &min);
	if (ret < 0) {
		return ret;
	}

	ret = video_get_ctrl(dev, cid | VIDEO_CTRL_GET_MAX, &max);
	if (ret < 0) {
		return ret;
	}

	if (value < min || value > max || min > max) {
		return -ERANGE;
	}

	return 0;
}

int video_check_range_int64(const struct device *dev, unsigned int cid, int64_t value)
{
	int64_t min;
	int64_t max;
	int ret;

	ret = video_get_ctrl(dev, cid | VIDEO_CTRL_GET_MIN, &min);
	if (ret < 0) {
		return ret;
	}

	ret = video_get_ctrl(dev, cid | VIDEO_CTRL_GET_MAX, &max);
	if (ret < 0) {
		return ret;
	}

	if (value < min || value > max || min > max) {
		return -ERANGE;
	}

	return 0;
}
