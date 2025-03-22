/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/pixel/stream.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pixel_stream, CONFIG_PIXEL_LOG_LEVEL);

static void _assert_complete(struct pixel_stream *strm, bool begin)
{
	for (; strm != NULL; strm = strm->next) {
		if (strm->run == NULL) {
			continue;
		}

		__ASSERT(ring_buf_size_get(&strm->ring) == 0,
			 "Core %s did not empty its input buffer, %u bytes left", strm->name,
			 ring_buf_size_get(&strm->ring));

		if (begin && strm->line_offset == 0) {
			continue;
		}

		__ASSERT(strm->line_offset == strm->height,
			 "Core %s did only process %u lines out of %u", strm->name,
			 strm->line_offset, strm->height);
	}
}

void pixel_stream_load(struct pixel_stream *strm, const uint8_t *buf, size_t size)
{
	struct pixel_stream prev = {.next = strm};

	__ASSERT_NO_MSG(strm != NULL);
	__ASSERT_NO_MSG(buf != NULL);

	_assert_complete(strm, true);

	LOG_DBG("Loading %zu bytes into this pipeline:", size);

	for (struct pixel_stream *x = strm; x != NULL; x = x->next) {
		LOG_DBG("- %s", x->name);
		x->line_offset = 0;
		x->total_time = 0;
	}

	for (size_t i = 0; i + strm->pitch <= size; i += strm->pitch) {
		LOG_DBG("bytes %zu-%zu/%zu into %s", i, i + strm->pitch, size, strm->name);
		ring_buf_put(&strm->ring, &buf[i], strm->pitch);
		pixel_stream_done(&prev);
	}

	LOG_DBG("Processed a full buffer of %zu bytes", size);

	_assert_complete(strm, false);
}
