/*
 * SPDX-FileCopyrightText: Copyright Linaro Limited
 * SPDX-FileCopyrightText: Copyright tinyVision.ai Inc.
 * SPDX-FileCopyrightText: The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/video/video.h>

LOG_MODULE_REGISTER(video_buffer, CONFIG_VIDEO_LOG_LEVEL);

#if defined(CONFIG_VIDEO_BUFFER_USE_SHARED_MULTI_HEAP)
#include <zephyr/multi_heap/shared_multi_heap.h>

#define VIDEO_COMMON_HEAP_ALLOC(align, size, timeout)                                              \
	shared_multi_heap_aligned_alloc(CONFIG_VIDEO_BUFFER_SMH_ATTRIBUTE, align, size)
#define VIDEO_COMMON_FREE(block) shared_multi_heap_free(block)
#else

#if !defined(CONFIG_VIDEO_BUFFER_POOL_ZEPHYR_REGION)
#define VIDEO_BUFFER_POOL_REGION_NAME __noinit_named(kheap_buf_video_buffer_pool)
#else
#define VIDEO_BUFFER_POOL_REGION_NAME Z_GENERIC_SECTION(CONFIG_VIDEO_BUFFER_POOL_ZEPHYR_REGION_NAME)
#endif

/*
 * The k_heap is manually initialized instead of using directly Z_HEAP_DEFINE_IN_SECT
 * since the section might not be yet accessible from the beginning, making it impossible
 * to initialize it if done via Z_HEAP_DEFINE_IN_SECT
 */
static char VIDEO_BUFFER_POOL_REGION_NAME __aligned(8)
	video_buffer_pool_mem[MAX(CONFIG_VIDEO_BUFFER_POOL_HEAP_SIZE, Z_HEAP_MIN_SIZE)];
static struct k_heap video_buffer_pool;
static bool video_buffer_pool_initialized;

static void *video_buffer_k_heap_aligned_alloc(size_t align, size_t bytes, k_timeout_t timeout)
{
	if (!video_buffer_pool_initialized) {
		k_heap_init(&video_buffer_pool, video_buffer_pool_mem,
			    MAX(CONFIG_VIDEO_BUFFER_POOL_HEAP_SIZE, Z_HEAP_MIN_SIZE));
		video_buffer_pool_initialized = true;
	}

	return k_heap_aligned_alloc(&video_buffer_pool, align, bytes, timeout);
}

#define VIDEO_COMMON_HEAP_ALLOC(align, size, timeout)                                              \
	video_buffer_k_heap_aligned_alloc(align, size, timeout)
#define VIDEO_COMMON_FREE(block) k_heap_free(&video_buffer_pool, block)
#endif

static struct video_buffer video_buf[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX];

enum {
	/** buffer is allocated from the internal video heap */
	VIDEO_MEMORY_INTERNAL = 1,
	/** buffer is allocated from outside */
	VIDEO_MEMORY_EXTERNAL = 2,
};

struct video_buffer *video_buffer_aligned_alloc(size_t size, size_t align, k_timeout_t timeout)
{
	struct video_buffer *vbuf = NULL;

	/* find available video buffer */
	for (uint16_t i = 0; i < ARRAY_SIZE(video_buf); i++) {
		if (video_buf[i].buffer == NULL) {
			vbuf = &video_buf[i];
			vbuf->index = i;
			break;
		}
	}

	if (vbuf == NULL) {
		return NULL;
	}

	/* Alloc buffer memory */
	vbuf->buffer = VIDEO_COMMON_HEAP_ALLOC(align, size, timeout);
	if (vbuf->buffer == NULL) {
		return NULL;
	}

	vbuf->memory = VIDEO_MEMORY_INTERNAL;
	vbuf->size = size;
	vbuf->bytesused = 0;

	return vbuf;
}

struct video_buffer *video_buffer_alloc(size_t size, k_timeout_t timeout)
{
	return video_buffer_aligned_alloc(size, sizeof(void *), timeout);
}

int video_buffer_release(struct video_buffer *vbuf)
{
	if (vbuf == NULL || vbuf->index >= ARRAY_SIZE(video_buf)) {
		LOG_ERR("Invalid buffer index: %u", vbuf->index);
		return -EINVAL;
	}

	if (video_buf[vbuf->index].buffer == NULL) {
		LOG_ERR("Buffer %u is already released", vbuf->index);
		return -EINVAL;
	}

	if (video_buf[vbuf->index].memory == VIDEO_MEMORY_INTERNAL) {
		VIDEO_COMMON_FREE(video_buf[vbuf->index].buffer);
	}

	video_buf[vbuf->index].buffer = NULL;

	return 0;
}

int video_import_buffer(uint8_t *mem, size_t sz, uint16_t *idx)
{
	uint16_t ind;

	if (mem == NULL || sz == 0) {
		LOG_ERR("Invalid memory address or size");
		return -EINVAL;
	}

	/* Find the 1st available slot in the video buffer pool */
	for (ind = 0; ind < ARRAY_SIZE(video_buf); ind++) {
		if (video_buf[ind].buffer == NULL) {
			break;
		}
	}

	if (ind == ARRAY_SIZE(video_buf)) {
		return -ENOBUFS;
	}

	/* Populate the internal buffer */
	video_buf[ind].index = ind;
	video_buf[ind].size = sz;
	video_buf[ind].memory = VIDEO_MEMORY_EXTERNAL;
	video_buf[ind].buffer = mem;
	video_buf[ind].bytesused = 0;
	video_buf[ind].timestamp = 0;

	/* Return the buffer index to the requester */
	*idx = ind;

	return 0;
}

int video_enqueue(const struct device *dev, struct video_buffer *buf)
{
	if (dev == NULL || buf == NULL || buf->index >= CONFIG_VIDEO_BUFFER_POOL_NUM_MAX ||
	    (buf->type != VIDEO_BUF_TYPE_INPUT && buf->type != VIDEO_BUF_TYPE_OUTPUT)) {
		return -EINVAL;
	}

	video_buf[buf->index].type = buf->type;

	return video_driver_enqueue(dev, &video_buf[buf->index]);
}

int video_dequeue(const struct device *dev, struct video_buffer **vbuf,
				       k_timeout_t timeout)
{
	return video_driver_dequeue(dev, vbuf, timeout);
}

int video_transfer_buffer(const struct device *src, const struct device *sink,
			  enum video_buf_type src_type, enum video_buf_type sink_type,
			  k_timeout_t timeout)
{
	struct video_buffer *buf = &(struct video_buffer){.type = src_type};
	int ret;

	ret = video_driver_dequeue(src, &buf, timeout);
	if (ret < 0) {
		return ret;
	}

	buf->type = sink_type;

	return video_driver_enqueue(sink, buf);
}
