/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PIXEL_OPERATION_H
#define ZEPHYR_INCLUDE_PIXEL_OPERATION_H

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/__assert.h>

/**
 * @brief One step of a line operation pipeline
 *
 * @c pixel_operation structs are chained together into a linked list.
 * Each step of the linked list contain a ring buffer for the input data, and a pointer to a
 * conversion function processing it.
 * Along with extra metadata, this is used to process data as a operation of lines.
 */
struct pixel_operation {
	sys_snode_t node;
	/** Name of the operation, useful for debugging the operation */
	const uint8_t *name;
	/** Operation type in case there are several variants for an operation */
	uint32_t type;
	/** Pixel input format as a four character code */
	uint32_t fourcc_in;
	/** Pixel output format as a four character code */
	uint32_t fourcc_out;
	/** Width of the image in pixels */
	uint16_t width;
	/** Height of the image in pixels */
	uint16_t height;
	/** Current position within the frame */
	uint16_t line_offset;
	/** Number of lines of context around the line being converted */
	uint16_t window_size;
	/** Number of bytes of input needed to call @c run() */
	uint16_t threshold;
	/** Whether the ring buffer memory is allocated on the pixel heap or externally */
	bool is_heap;
	/** Ring buffer that keeps track of the position in bytes */
	struct ring_buf ring;
	/** Function that performs the I/O */
	void (*run)(struct pixel_operation *op);
	/** Operation-specific data */
	void *arg;
	/** Timestamp since the op started working in CPU cycles */
	uint32_t start_time;
	/** Total time spent working in this op through the operation in CPU cycles */
	uint32_t total_time;
};

/**
 * @brief Get a pointer to an output line from the next step of the operation.
 *
 * The buffer obtained can then be used to store the output of the conversion.
 *
 * The lines will be considered as converted as soon as @ref pixel_operation_done() is called,
 * which will feed the line into the next step of the operation.
 *
 * @param op Current operation in progress.
 * @return Pointer to the requested line buffer, never NULL.
 */
static inline uint8_t *pixel_operation_get_output_line(struct pixel_operation *op)
{
	struct pixel_operation *next = SYS_SLIST_PEEK_NEXT_CONTAINER(op, node);
	uint32_t pitch;
	uint8_t *lines;
	uint32_t size;

	__ASSERT_NO_MSG(next != NULL);

	pitch = next->width * video_bits_per_pixel(next->fourcc_in) / BITS_PER_BYTE;
	size = ring_buf_put_claim(&next->ring, &lines, pitch);
	ring_buf_put_finish(&next->ring, size);

	__ASSERT(size == pitch,
		 "%s asked for %u output bytes, only have %u out of %u",
		 op->name, pitch, size, ring_buf_capacity_get(&next->ring));

	return lines;
}

/**
 * @brief Get a pointer to a given number of input lines, and consume them from the operation.
 *
 * The lines are considered as processed, which will free them from the input ring buffer, and
 * allow more data to flow in.
 *
 * @param op Current operation in progress.
 * @param nb Number of lines to get in one block.
 * @return Pointer to the requested number of lines, never NULL.
 */
static inline const uint8_t *pixel_operation_get_input_lines(struct pixel_operation *op, size_t nb)
{
	uint32_t pitch;
	uint32_t size;
	uint8_t *lines;

	op->line_offset += nb;

	__ASSERT(op->line_offset <= op->height,
		"Trying to read at position %u beyond the height of the frame %u",
		op->line_offset, op->height);

	pitch = op->width * video_bits_per_pixel(op->fourcc_in) / BITS_PER_BYTE;
	size = ring_buf_get_claim(&op->ring, &lines, pitch * nb);
	ring_buf_get_finish(&op->ring, size);

	__ASSERT(size == pitch * nb,
		 "%s asked for %u input bytes, only have %u out of %u",
		 op->name, pitch, size, ring_buf_capacity_get(&op->ring));

	return lines;
}

/**
 * @brief Shorthand for @ref pixel_operation_get_input_lines() to get a single input line.
 *
 * @param op Current operation in progress.
 * @return Pointer to the requested number of lines, never NULL.
 */
static inline const uint8_t *pixel_operation_get_input_line(struct pixel_operation *op)
{
	return pixel_operation_get_input_lines(op, 1);
}

/**
 * @brief Request a pointer to the next line of data without affecting the input sream.
 *
 * This permits to implement a lookahead operation when one or several lines of context is needed
 * in addition to the line converted.
 *
 * @return The pointer to the input data.
 */
static inline uint8_t *pixel_operation_peek_input_line(struct pixel_operation *op)
{
	uint32_t pitch = op->width * video_bits_per_pixel(op->fourcc_in) / BITS_PER_BYTE;
	uint8_t *line;
	uint32_t size = ring_buf_get_claim(&op->ring, &line, pitch);

	__ASSERT_NO_MSG(size == pitch);

	return line;
}

/**
 * @brief Request a pointer to the entire input buffer content, consumed from the input operation.
 *
 * @param op Current operation in progress.
 * @return The pointer to the input data.
 */
static inline const uint8_t *pixel_operation_get_all_input(struct pixel_operation *op)
{
	uint8_t *remaining;
	uint32_t size;

	__ASSERT_NO_MSG(op != NULL);

	op->line_offset = op->height;

	__ASSERT_NO_MSG(op->line_offset <= op->height);

	size = ring_buf_get_claim(&op->ring, &remaining, ring_buf_capacity_get(&op->ring));
	ring_buf_get_finish(&op->ring, size);

	__ASSERT(size == ring_buf_capacity_get(&op->ring),
		 "Could not dequeue the entire input buffer of %s, %u used, %u free", op->name,
		 ring_buf_size_get(&op->ring), ring_buf_space_get(&op->ring));

	return remaining;
}

static inline void pixel_operation_run(struct pixel_operation *op)
{
	if (op != NULL && op->run != NULL) {
		/* Start the counter of the next operation */
		op->start_time = k_cycle_get_32();

		while (ring_buf_size_get(&op->ring) >= op->threshold &&
		       op->line_offset < op->height) {
			op->run(op);
		}
	}
}

/**
 * @brief Mark the line obtained with @ref pixel_operation_get_output_line as converted.
 *
 * This will let the next step of the operation know that a new line was converted.
 * This allows the pipeline to trigger the next step if there is enough data submitted to it.
 *
 * @param op Current operation in progress.
 */
static inline void pixel_operation_done(struct pixel_operation *op)
{
	/* Ignore any "peek" operation done previouslyl */
	ring_buf_get_finish(&op->ring, 0);
	ring_buf_put_finish(&op->ring, 0);

	/* Flush the timestamp to the counter */
	op->total_time += (op->start_time == 0) ? (0) : (k_cycle_get_32() - op->start_time);

	/* Run the next operation in the chain now that more data is available */
	pixel_operation_run(SYS_SLIST_PEEK_NEXT_CONTAINER(op, node));

	/* Resuming to this operation, reset the time counter */
	op->start_time = k_cycle_get_32();
}

#endif /* ZEPHYR_INCLUDE_PIXEL_OPERATION_H */
