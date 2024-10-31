/*
 * Copyright (c) 2019, Linaro Limited
 * Copyright (c) 2024, tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/video.h>

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
	vbuf->size = size;
	vbuf->bytesused = 0;

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

int video_format_caps_index(const struct video_format_cap *fmts, const struct video_format *fmt,
			    size_t *idx)
{
	for (int i = 0; fmts[i].pixelformat != 0; i++) {
		if (fmts[i].pixelformat == fmt->pixelformat &&
		    IN_RANGE(fmt->width, fmts[i].width_min, fmts[i].width_max) &&
		    IN_RANGE(fmt->height, fmts[i].height_min, fmts[i].height_max)) {
			*idx = i;
			return 0;
		}
	}
	return -ENOENT;
}

void video_closest_frmival_stepwise(const struct video_frmival_stepwise *stepwise,
				    const struct video_frmival *desired,
				    struct video_frmival *match)
{
	uint64_t min = stepwise->min.numerator;
	uint64_t max = stepwise->max.numerator;
	uint64_t step = stepwise->step.numerator;
	uint64_t goal = desired->numerator;

	/* Set a common denominator to all values */
	min *= stepwise->max.denominator * stepwise->step.denominator * desired->denominator;
	max *= stepwise->min.denominator * stepwise->step.denominator * desired->denominator;
	step *= stepwise->min.denominator * stepwise->max.denominator * desired->denominator;
	goal *= stepwise->min.denominator * stepwise->max.denominator * stepwise->step.denominator;

	/* Saturate the desired value to the min/max supported */
	goal = CLAMP(goal, min, max);

	/* Compute a numerator and denominator */
	match->numerator = min + DIV_ROUND_CLOSEST(goal - min, step) * step;
	match->denominator = stepwise->min.denominator * stepwise->max.denominator *
			     stepwise->step.denominator * desired->denominator;
}

void video_closest_frmival(const struct device *dev, enum video_endpoint_id ep,
			   struct video_frmival_enum *match)
{
	uint64_t best_diff_nsec = INT32_MAX;
	struct video_frmival desired = match->discrete;
	struct video_frmival_enum fie = {.format = match->format};

	__ASSERT(match->type != VIDEO_FRMIVAL_TYPE_STEPWISE,
		 "cannot find range matching the range, only a value matching the range");

	while (video_enum_frmival(dev, ep, &fie) == 0) {
		struct video_frmival tmp = {0};
		uint64_t diff_nsec = 0, a, b;

		switch (fie.type) {
		case VIDEO_FRMIVAL_TYPE_DISCRETE:
			tmp = fie.discrete;
			break;
		case VIDEO_FRMIVAL_TYPE_STEPWISE:
			video_closest_frmival_stepwise(&fie.stepwise, &desired, &tmp);
			break;
		default:
			__ASSERT(false, "invalid answer from the queried video device");
		}

		a = video_frmival_nsec(&desired);
		b = video_frmival_nsec(&tmp);
		diff_nsec = a > b ? a - b : b - a;
		if (diff_nsec < best_diff_nsec) {
			best_diff_nsec = diff_nsec;
			memcpy(&match->discrete, &tmp, sizeof(tmp));

			/* The video_enum_frmival() function will increment fie.index every time.
			 * Compensate for it to get the current index, not the next index.
			 */
			match->index = fie.index - 1;
		}
	}
}
