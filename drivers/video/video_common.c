/*
 * Copyright (c) 2019, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>

#include <drivers/video.h>

K_MEM_POOL_DEFINE(video_buffer_pool,
		  CONFIG_VIDEO_BUFFER_POOL_ALIGN,
		  CONFIG_VIDEO_BUFFER_POOL_SZ_MAX,
		  CONFIG_VIDEO_BUFFER_POOL_NUM_MAX,
		  CONFIG_VIDEO_BUFFER_POOL_ALIGN);

static struct video_buffer video_buf[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX];

struct video_buffer *video_buffer_alloc(size_t size)
{
	struct video_buffer *vbuf = NULL;
	int i;

	/* find available video buffer */
	for (i = 0; i < ARRAY_SIZE(video_buf); i++) {
		if (video_buf[i].buffer == NULL) {
			vbuf = &video_buf[i];
			break;
		}
	}

	if (vbuf == NULL) {
		return NULL;
	}

	/* Alloc buffer memory */
	vbuf->buffer = k_mem_pool_malloc(&video_buffer_pool, size);
	if (vbuf->buffer == NULL) {
		return NULL;
	}

	vbuf->size = size;
	vbuf->bytesused = 0;

	return vbuf;
}

void video_buffer_release(struct video_buffer *vbuf)
{
	k_free(vbuf->buffer);
	vbuf->buffer = NULL;
}
