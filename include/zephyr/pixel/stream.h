/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PIXEL_STREAM_H
#define ZEPHYR_INCLUDE_PIXEL_STREAM_H

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/__assert.h>

struct pixel_stream {
	/* Display name, useful for debugging the stream */
	uint8_t *name;
	/* Ring buffer that keeps track of the position in bytes */
	struct ring_buf ring;
	/* Number of bytes returned while asking for an input or output line */
	size_t pitch;
	/* Current position within the frame */
	uint16_t line_offset;
	/* Total number of lines in the frame */
	uint16_t height;
	/* Connection to the next element of the stream */
	struct pixel_stream *next;
	/* Function that performs the I/O */
	void (*run)(struct pixel_stream *strm);
	/* Timestamp since the strm started working in CPU cycles */
	uint32_t start_time;
	/* Total time spent working in this strm through the stream in CPU cycles */
	uint32_t total_time;
};

#define PIXEL_STREAM_DEFINE(_name, _run, _width, _height, _pitch, _bufsize)                        \
	struct pixel_stream _name = {                                                              \
		.name = "[" #_name " " #_run " " STRINGIFY(_width) "x" STRINGIFY(_height) "]",     \
		.ring = RING_BUF_INIT((uint8_t [_bufsize]) {0}, _bufsize),                         \
		.pitch = (_pitch),                                                                 \
		.height = (_height),                                                               \
		.run = (_run),                                                                     \
	}

/**
 * @brief Load a buffer into the stream.
 *
 * The parameters such as line pitch or image height are to be configured inside each individual
 * strm before calling this function.
 *
 * @param strm Pipeline strm into which load the buffer one pitch worth of data at a time.
 * @param buf Buffer of data to load into the stream.
 * @param size Total of bytes in this buffer.
 */
void pixel_stream_load(struct pixel_stream *strm, const uint8_t *buf, size_t size);

static inline uint8_t *pixel_stream_peek_input_line(struct pixel_stream *strm)
{
	uint8_t *line;
	uint32_t size;

	__ASSERT_NO_MSG(strm != NULL);

	size = ring_buf_get_claim(&strm->ring, &line, strm->pitch);
	__ASSERT_NO_MSG(size == strm->pitch);

	return line;
}

static inline const uint8_t *pixel_stream_get_input_lines(struct pixel_stream *strm, size_t nb)
{
	uint8_t *lines;
	uint32_t size;

	__ASSERT_NO_MSG(strm != NULL);

	strm->line_offset += nb;
	__ASSERT_NO_MSG(strm->line_offset <= strm->height);

	size = ring_buf_get_claim(&strm->ring, &lines, strm->pitch * nb);
	ring_buf_get_finish(&strm->ring, size);
	__ASSERT(size == strm->pitch * nb,
		 "%s asked for %zu input bytes, obtained only %u, total used %u, total free %u",
		 strm->name, strm->pitch * nb, size, ring_buf_size_get(&strm->ring),
		 ring_buf_space_get(&strm->ring));

	return lines;
}

static inline const uint8_t *pixel_stream_get_input_line(struct pixel_stream *strm)
{
	return pixel_stream_get_input_lines(strm, 1);
}

static inline const uint8_t *pixel_stream_get_all_input(struct pixel_stream *strm)
{
	uint8_t *remaining;
	uint32_t size;

	__ASSERT_NO_MSG(strm != NULL);

	strm->line_offset = strm->height;
	__ASSERT_NO_MSG(strm->line_offset <= strm->height);

	size = ring_buf_get_claim(&strm->ring, &remaining, ring_buf_capacity_get(&strm->ring));
	ring_buf_get_finish(&strm->ring, size);
	__ASSERT(size == ring_buf_capacity_get(&strm->ring),
		"Could not dequeue the entire input buffer of %s, %u used, %u free",
		 strm->name, ring_buf_size_get(&strm->ring), ring_buf_space_get(&strm->ring));

	return remaining;
}

static inline uint8_t *pixel_stream_get_output_lines(struct pixel_stream *strm, size_t nb)
{
	uint8_t *lines;
	uint32_t size;

	__ASSERT_NO_MSG(strm != NULL);
	__ASSERT_NO_MSG(strm->next != NULL);

	size = ring_buf_put_claim(&strm->next->ring, &lines, strm->next->pitch * nb);
	ring_buf_put_finish(&strm->next->ring, size);
	__ASSERT(size == strm->next->pitch,
		  "%s asked for %zu output bytes, only have %u, total used %u, total free %u",
		 strm->name, strm->pitch * nb, size, ring_buf_size_get(&strm->ring),
		 ring_buf_space_get(&strm->ring));

	return lines;
}

static inline uint8_t *pixel_stream_get_output_line(struct pixel_stream *strm)
{
	return pixel_stream_get_output_lines(strm, 1);
}

static inline void pixel_stream_done(struct pixel_stream *strm)
{
	__ASSERT_NO_MSG(strm != NULL);

	/* Ignore any "peek" operation done previouslyl */
	ring_buf_get_finish(&strm->ring, 0);
	ring_buf_put_finish(&strm->ring, 0);

	/* Flush the timestamp to the counter */
	strm->total_time += strm->start_time - k_cycle_get_32();
	strm->start_time = k_cycle_get_32();

	if (strm->next != NULL && strm->next->run && ring_buf_space_get(&strm->next->ring) == 0) {
		/* Start the counter of the next stream */
		strm->next->start_time = k_cycle_get_32();

		/* Run the next strm */
		strm->next->run(strm->next);

		/* Resuming to this strm, upgrade the start time */
		strm->start_time = k_cycle_get_32();
	}
}

#endif /* ZEPHYR_INCLUDE_PIXEL_STREAM_H */
