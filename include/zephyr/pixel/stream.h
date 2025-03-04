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

/**
 * @brief One step of a line stream pipeline
 *
 * @c pixel_stream structs are chained together into a linked list.
 * Each step of the linked list contain a ring buffer for the input data, and a pointer to a
 * conversion function processing it.
 * Along with extra metadata, this is used to process data as a stream of lines.
 */
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
	uint16_t width;
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

/**
 * @brief Load a buffer into a stream.
 *
 * The parameters such as line pitch or image height are to be configured inside each individual
 * strm before calling this function.
 *
 * @param first Pipeline stream into which load the buffer one pitch worth of data at a time.
 * @param buf Buffer of data to load into the stream.
 * @param size Total of bytes in this source buffer.
 */
void pixel_stream_load(struct pixel_stream *first, const uint8_t *buf, size_t size);

/**
 * @brief Load an input frame into a stream and store it into a 8-bits-per-pixel output frame.
 *
 * Convert an input buffer to an output buffer according to a list of steps passed as arguments.
 *
 * @param src_buf Source buffer to load into the stream
 * @param src_size Size of the source buffer in bytes
 * @param src_width Width of the source image in number
 * @param dst_buf Destination buffer to load into the stream
 * @param dst_size Size of the destination buffer in bytes
 * @param dst_width Width of the destination image in number
 * @param ... NULL-terminated list of @ref pixel_stream processing steps to apply.
 */
void pixel_stream_to_raw8frame(const uint8_t *src_buf, size_t src_size, uint16_t src_width,
			       uint8_t *dst_buf, size_t dst_size, uint16_t dst_width, ...);
/**
 * @brief Load an input frame into a stream and store it into a 16-bits-per-pixel output frame.
 * @copydetails pixel_stream_to_raw8frame()
 */
void pixel_stream_to_raw16frame(const uint8_t *src_buf, size_t src_size, uint16_t src_width,
				uint8_t *dst_buf, size_t dst_size, uint16_t dst_width, ...);
/**
 * @brief Load an input frame into a stream and store it into a 24-bits-per-pixel output frame.
 * @copydetails pixel_stream_to_raw8frame()
 */
void pixel_stream_to_raw24frame(const uint8_t *src_buf, size_t src_size, uint16_t src_width,
				uint8_t *dst_buf, size_t dst_size, uint16_t dst_width, ...);

/** @copydoc pixel_stream_to_raw8frame() */
#define pixel_stream_to_rgb332frame	pixel_stream_to_raw8frame
/** @copydoc pixel_stream_to_raw8frame() */
#define pixel_stream_to_rggb8frame	pixel_stream_to_raw8frame
/** @copydoc pixel_stream_to_raw8frame() */
#define pixel_stream_to_bggr8frame	pixel_stream_to_raw8frame
/** @copydoc pixel_stream_to_raw8frame() */
#define pixel_stream_to_gbrg8frame	pixel_stream_to_raw8frame
/** @copydoc pixel_stream_to_raw8frame() */
#define pixel_stream_to_grbg8frame	pixel_stream_to_raw8frame
/** @copydoc pixel_stream_to_raw16frame() */
#define pixel_stream_to_rgb565beframe	pixel_stream_to_raw16frame
/** @copydoc pixel_stream_to_raw16frame() */
#define pixel_stream_to_rgb565leframe	pixel_stream_to_raw16frame
/** @copydoc pixel_stream_to_raw16frame() */
#define pixel_stream_to_yuyvframe	pixel_stream_to_raw16frame
/** @copydoc pixel_stream_to_raw24frame() */
#define pixel_stream_to_rgb24frame	pixel_stream_to_raw24frame
/** @copydoc pixel_stream_to_raw24frame() */
#define pixel_stream_to_yuv24frame	pixel_stream_to_raw24frame

/**
 * @brief Get a pointer to an output line from the next step of the stream.
 *
 * The buffer obtained can then be used to store the output of the conversion.
 *
 * The lines will be considered as converted as soon as @ref pixel_stream_done() is called, which
 * will feed the line into the next step of the stream.
 *
 * There is no need to pass @c strm->next as argument as @c pixel_stream_get_output_line() will
 * take care of it internally.
 *
 * @param strm Stream from which the next output line buffer will be taken.
 * @return Pointer to the requested line buffer, never NULL.
 */
static inline uint8_t *pixel_stream_get_output_line(struct pixel_stream *strm)
{
	uint8_t *lines;
	uint32_t size;

	__ASSERT_NO_MSG(strm != NULL);
	__ASSERT_NO_MSG(strm->next != NULL);

	size = ring_buf_put_claim(&strm->next->ring, &lines, strm->next->pitch);
	ring_buf_put_finish(&strm->next->ring, size);
	__ASSERT(size == strm->next->pitch,
		 "%s asked for %zu output bytes, only have %u, total used %u, total free %u",
		 strm->name, strm->pitch, size, ring_buf_size_get(&strm->ring),
		 ring_buf_space_get(&strm->ring));

	return lines;
}

/**
 * @brief Get a pointer to a given number of input lines, and consume them from the stream.
 *
 * The lines are considered as processed, which will free them from the input ring buffer, and
 * allow more data to flow in.
 *
 * @param strm Stream from which get the input lines.
 * @param nb Number of lines to get in one block.
 * @return Pointer to the requested number of lines, never NULL.
 */
static inline const uint8_t *pixel_stream_get_input_lines(struct pixel_stream *strm, size_t nb)
{
	uint8_t *lines;
	uint32_t size;

	__ASSERT_NO_MSG(strm != NULL);

	strm->line_offset += nb;
	__ASSERT(strm->line_offset <= strm->height,
		"Trying to read at position %u beyond the height of the frame %u",
		strm->line_offset, strm->height);

	size = ring_buf_get_claim(&strm->ring, &lines, strm->pitch * nb);
	ring_buf_get_finish(&strm->ring, size);
	__ASSERT(size == strm->pitch * nb,
		 "%s asked for %zu input bytes, obtained only %u, total used %u, total free %u",
		 strm->name, strm->pitch * nb, size, ring_buf_size_get(&strm->ring),
		 ring_buf_space_get(&strm->ring));

	return lines;
}

/**
 * @brief Shorthand for @ref pixel_stream_get_input_lines() to get a single input line.
 *
 * @param strm Stream from which get the input lines.
 * @return Pointer to the requested number of lines, never NULL.
 */
static inline const uint8_t *pixel_stream_get_input_line(struct pixel_stream *strm)
{
	return pixel_stream_get_input_lines(strm, 1);
}

/**
 * @brief Request a pointer to the next line of data without affecting the input sream.
 *
 * This permits to implement a lookahead operation when one or several lines of context is needed
 * in addition to the line converted.
 *
 * @return The pointer to the input data.
 */
static inline uint8_t *pixel_stream_peek_input_line(struct pixel_stream *strm)
{
	uint8_t *line;
	uint32_t size;

	__ASSERT_NO_MSG(strm != NULL);

	size = ring_buf_get_claim(&strm->ring, &line, strm->pitch);
	__ASSERT_NO_MSG(size == strm->pitch);

	return line;
}

/**
 * @brief Request a pointer to the entire input buffer content, consumed from the input stream.
 *
 * @param strm Stream from which get the buffer
 * @return The pointer to the input data.
 */
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
		 "Could not dequeue the entire input buffer of %s, %u used, %u free", strm->name,
		 ring_buf_size_get(&strm->ring), ring_buf_space_get(&strm->ring));

	return remaining;
}

/**
 * @brief Mark the line obtained with @ref pixel_stream_get_output_line as converted.
 *
 * This will let the next step of the stream know that a new line was converted.
 * This allows the pipeline to trigger the next step if there is enough data submitted to it.
 *
 * @param strm Stream to which confirm the line conversion.
 */
static inline void pixel_stream_done(struct pixel_stream *strm)
{
	__ASSERT_NO_MSG(strm != NULL);

	/* Ignore any "peek" operation done previouslyl */
	ring_buf_get_finish(&strm->ring, 0);
	ring_buf_put_finish(&strm->ring, 0);

	/* Flush the timestamp to the counter */
	strm->total_time += strm->start_time == 0 ? 0 : k_cycle_get_32() - strm->start_time;
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

/**
 * @brief Helper to turn a line-conversion function into a stream-conversion function
 *
 * The line conversion function is free to perform any processing on the input line.
 * The @c w argument is the width of both the source and destination buffers.
 *
 * @param strm Input stream to process.
 * @param fn Line conversion function to turn into a stream conversion function.
 */
static inline void pixel_line_to_stream(struct pixel_stream *strm,
					void (*fn)(const uint8_t *src, uint8_t *dst, uint16_t w))
{
	fn(pixel_stream_get_input_line(strm), pixel_stream_get_output_line(strm), strm->width);
	pixel_stream_done(strm);
}

/** @cond INTERNAL_HIDDEN */
#define PIXEL_STREAM_DEFINE(_name, _run, _width, _height, _pitch, _bufsize)                        \
	struct pixel_stream _name = {                                                              \
		.name = "[" #_name " " #_run " " STRINGIFY(_width) "x" STRINGIFY(_height) "]",     \
		.ring = RING_BUF_INIT((uint8_t [_bufsize]) {0}, _bufsize),                         \
		.pitch = (_pitch),                                                                 \
		.width = (_width),                                                                 \
		.height = (_height),                                                               \
		.run = (_run),                                                                     \
	}
/* @endcond */

#endif /* ZEPHYR_INCLUDE_PIXEL_STREAM_H */
