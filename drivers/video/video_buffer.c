/*
 * Copyright (c) 2019, Linaro Limited
 * Copyright (c) 2024-2025, tinyVision.ai Inc.
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/video.h>
#include <zephyr/rtio/rtio.h>

#include "video_buffer.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_buffer, CONFIG_VIDEO_LOG_LEVEL);

#if defined(CONFIG_VIDEO_BUFFER_USE_SHARED_MULTI_HEAP)
#include <zephyr/multi_heap/shared_multi_heap.h>

#define VIDEO_COMMON_HEAP_ALLOC(align, size, timeout)                                              \
	shared_multi_heap_aligned_alloc(CONFIG_VIDEO_BUFFER_SMH_ATTRIBUTE, align, size)
#define VIDEO_COMMON_FREE(block) shared_multi_heap_free(block)
#else
K_HEAP_DEFINE(video_buffer_pool, CONFIG_VIDEO_BUFFER_POOL_SZ_MAX *CONFIG_VIDEO_BUFFER_POOL_NUM_MAX);
#define VIDEO_COMMON_HEAP_ALLOC(align, size, timeout)                                              \
	k_heap_aligned_alloc(&video_buffer_pool, align, size, timeout);
#define VIDEO_COMMON_FREE(block) k_heap_free(&video_buffer_pool, block)
#endif

static struct video_buffer video_buf[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX];

struct mem_block {
	void *data;
};

static struct mem_block video_block[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX];

struct video_buffer *video_buffer_aligned_alloc(size_t size, size_t align, k_timeout_t timeout)
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
	block->data = VIDEO_COMMON_HEAP_ALLOC(align, size, timeout);
	if (block->data == NULL) {
		return NULL;
	}

	vbuf->buffer = block->data;

	return vbuf;
}

struct video_buffer *video_buffer_alloc(size_t size, k_timeout_t timeout)
{
	return video_buffer_aligned_alloc(size, sizeof(void *), timeout);
}

void video_buffer_release(struct video_buffer *vbuf)
{
	struct mem_block *block = NULL;
	int i;

	__ASSERT_NO_MSG(vbuf != NULL);

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

/* To be completed */
int video_request_buffers(uint8_t count, size_t size, enum video_buf_type type,
			  enum video_buf_memory memory)
{
	struct video_buffer *buffer;

	/* TODO: Input request vs Output request for m2m devices */
	if (count > CONFIG_VIDEO_BUFFER_POOL_NUM_MAX) {
		return -EINVAL;
	}

	/* TODO: Buffer request is not cumulative. For each new request, free all the buffers
	 * that are previously requested
	 */

	/* Allocate buffers */
	for (uint8_t i = 0; i < count; i++) {
		if (memory == VIDEO_MEMORY_VIDEO) {
			buffer = video_buffer_aligned_alloc(size, CONFIG_VIDEO_BUFFER_POOL_ALIGN,
							    K_FOREVER);
		} else {
			buffer = &video_buf[i];
		}

		buffer->type = type;
		buffer->index = i;
		buffer->size = size;
		buffer->bytesused = 0;
		buffer->state = VIDEO_BUF_STATE_DONE;
	}

	return 0;
}

RTIO_DEFINE(rtio, CONFIG_VIDEO_BUFFER_POOL_NUM_MAX, CONFIG_VIDEO_BUFFER_POOL_NUM_MAX);

int video_enqueue(const struct device *dev, struct video_buffer *buf)
{
	int ret;

	__ASSERT_NO_MSG(dev != NULL);

	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;
	if (api->enqueue == NULL) {
		return -ENOSYS;
	}

	if (video_buf[buf->index].type != buf->type ||
	    video_buf[buf->index].memory != buf->memory ||
	    video_buf[buf->index].state != VIDEO_BUF_STATE_DONE) {
		return -EINVAL;
	}

	if (buf->memory == VIDEO_MEMORY_USER) {
		if (buf->size != video_buf[buf->index].size) {
			return -EINVAL;
		}

		video_buf[buf->index].buffer = buf->buffer;
	}

	ret = api->enqueue(dev, &video_buf[buf->index]);
	if (ret < 0) {
		return ret;
	}

	/* RTIO submission */
	struct rtio_iodev *ri = video_find_iodev(dev);
	struct rtio_sqe *sqe = rtio_sqe_acquire(&rtio);

	__ASSERT_NO_MSG(ri != NULL);
	__ASSERT_NO_MSG(sqe != NULL);

	rtio_sqe_prep_read(sqe, ri, RTIO_PRIO_NORM, video_buf[buf->index].buffer,
			   video_buf[buf->index].size, &video_buf[buf->index]);

	sqe->flags |= RTIO_SQE_MULTISHOT;

	/* Do not wait for complete */
	rtio_submit(&rtio, 0);

	video_buf[buf->index].state = VIDEO_BUF_STATE_QUEUED;

	return 0;
}

int video_dequeue(struct video_buffer **buf)
{
	struct rtio_cqe *cqe = rtio_cqe_consume_block(&rtio);
	*buf = cqe->userdata;

	if (cqe->result < 0) {
		LOG_ERR("I/O operation failed");
		return cqe->result;
	}

	return 0;
}

void video_release_buf()
{
	/* Buffer will be re-queued thanks to RTIO_SQE_MULTISHOT */
	rtio_cqe_release(&rtio, rtio_cqe_consume_block(&rtio));
}

struct video_buffer *video_get_buf_sqe(struct mpsc *io_q)
{
	struct mpsc_node *node = mpsc_pop(io_q);
	if (node == NULL) {
		return NULL;
	}

	struct rtio_iodev_sqe *iodev_sqe = CONTAINER_OF(node, struct rtio_iodev_sqe, q);
	struct rtio_sqe *sqe = &iodev_sqe->sqe;

	if (sqe->op != RTIO_OP_RX) {
		LOG_ERR("Invalid operation %d of length %u for submission %p", sqe->op,
			sqe->rx.buf_len, (void *)iodev_sqe);
		rtio_iodev_sqe_err(iodev_sqe, -EINVAL);
		return NULL;
	}

	return sqe->userdata;
}

static void video_iodev_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	struct video_interface *vi = iodev_sqe->sqe.iodev->data;

	mpsc_push(vi->io_q, &iodev_sqe->q);
}

const struct rtio_iodev_api _video_iodev_api = {
	.submit = video_iodev_submit,
};
