/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdarg.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/pixel/stream.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pixel_stream, CONFIG_PIXEL_LOG_LEVEL);

struct pixel_stream *pixel_stream(struct pixel_stream *strm, ...)
{
	va_list ap;

	va_start(ap, strm);
	for (struct pixel_stream *step = strm; step != NULL;) {
		step = step->next = va_arg(ap, void *);
	}
	va_end(ap);

	return strm;
}

static void assert_complete(struct pixel_stream *strm, bool begin)
{
	for (; strm != NULL; strm = strm->next) {
		if (strm->run == NULL) {
			continue;
		}

		__ASSERT(ring_buf_size_get(&strm->ring) == 0,
			 "Core %s did not empty its input buffer, %u bytes out of %u left",
			 strm->name, ring_buf_size_get(&strm->ring), strm->ring.size);

		if (begin && strm->line_offset == 0) {
			continue;
		}

		__ASSERT(strm->line_offset == strm->height,
			 "Core %s did only process %u lines out of %u",
			 strm->name, strm->line_offset, strm->height);
	}
}

void pixel_stream_load(struct pixel_stream *strm, const uint8_t *buf, size_t size)
{
	struct pixel_stream root = {.next = strm};
	struct pixel_stream *step;

	if (strm == NULL) {
		LOG_INF("Pipeline empty, skipping execution");
		return;
	}

	__ASSERT_NO_MSG(buf != NULL);
	assert_complete(strm, true);

	LOG_DBG("Loading %zu bytes into this pipeline:", size);

	for (step = strm; step != NULL; step = step->next) {
		LOG_DBG("- %s", step->name);
		step->line_offset = 0;
		step->total_time = 0;
	}

	for (size_t i = 0; i + strm->pitch <= size; i += strm->pitch) {
		LOG_DBG("bytes %zu-%zu/%zu into %s", i, i + strm->pitch, size, strm->name);
		ring_buf_put(&strm->ring, &buf[i], strm->pitch);
		pixel_stream_done(&root);
	}

	LOG_DBG("Processed a full buffer of %zu bytes", size);

	for (step = root.next; step != NULL; step = step->next) {
		LOG_INF(" %4u us on %s", k_cyc_to_us_ceil32(step->total_time), step->name);
	}

	assert_complete(strm, false);
}

static inline void pixel_stream_to_frame(const uint8_t *src_buf, size_t src_size,
					 uint16_t src_width, uint8_t *dst_buf, size_t dst_size,
					 uint16_t dst_width, int bytes_per_pixel,
					 struct pixel_stream *strm)
{
	struct pixel_stream step_export = {
		.ring = RING_BUF_INIT(dst_buf, dst_size),
		.pitch = dst_width * bytes_per_pixel,
		.width = dst_width,
		.height = dst_size / bytes_per_pixel / dst_width,
		.name = "[export]",
	};
	struct pixel_stream root = {
		.width = src_width,
		.next = strm,
	};
	struct pixel_stream *step = &root;

	__ASSERT(dst_size % step_export.pitch == 0,
		"The output buffer has %zu bytes but lines are %zu bytes each, %zu bytes too much",
		dst_size, step_export.pitch, dst_size % step_export.pitch);

	__ASSERT(src_width == root.next->width,
		 "The width does not match between the arguments (%u) and first step %s (%u)",
		 src_width, root.next->name, root.next->width);

	/* Add an export step at the end */
	while (step->next != NULL) {
		step = step->next;
	}
	step->next = &step_export;

	/* Load the input buffer into the pipeline and flush the output */
	pixel_stream_load(root.next, src_buf, src_size);
	pixel_stream_get_all_input(&step_export);
}

void pixel_stream_to_raw8frame(const uint8_t *src_buf, size_t src_size, uint16_t src_width,
			       uint8_t *dst_buf, size_t dst_size, uint16_t dst_width,
			       struct pixel_stream *strm)
{
	pixel_stream_to_frame(src_buf, src_size, src_width, dst_buf, dst_size, dst_width, 1, strm);
}

void pixel_stream_to_raw16frame(const uint8_t *src_buf, size_t src_size, uint16_t src_width,
				uint8_t *dst_buf, size_t dst_size, uint16_t dst_width,
				struct pixel_stream *strm)
{
	pixel_stream_to_frame(src_buf, src_size, src_width, dst_buf, dst_size, dst_width, 2, strm);
}

void pixel_stream_to_raw24frame(const uint8_t *src_buf, size_t src_size, uint16_t src_width,
				uint8_t *dst_buf, size_t dst_size, uint16_t dst_width,
				struct pixel_stream *strm)
{
	pixel_stream_to_frame(src_buf, src_size, src_width, dst_buf, dst_size, dst_width, 3, strm);
}
