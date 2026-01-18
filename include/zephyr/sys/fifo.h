/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_FIFO_H_
#define ZEPHYR_INCLUDE_SYS_FIFO_H_

#include <zephyr/sys/ring_buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @defgroup fixed_size_fifo_apis fifo queue APIs
 * @ingroup datastructure_apis
 *
 * @brief Fixed size fifo queue implementation.
 *
 * @{
 */

/**
 * @brief A structure to represent a fifo queue
 */
struct fifo {
	/** @cond INTERNAL_HIDDEN */
	struct ring_buf rb;
	ring_buf_idx_t item_size;
	/** @endcond */
};

/** @cond INTERNAL_HIDDEN */

/** @endcond */

#define FIFO_INIT(buf, item_sz, item_capacity)				\
{									\
	.rb = RING_BUF_INIT(buf, item_sz * item_capacity),		\
	.item_size = item_sz,						\
}

/**
 * @brief Define and initialize a fifo queue for byte data.
 *
 * This macro establishes a fifo queue of an aqueueitrary size.
 *
 * @param name Name of the fifo queue instance.
 */
#define FIFO_DEFINE(name, item_size, item_capacity)						\
	static uint8_t __noinit _fifo_data_##name[item_size * item_capacity];			\
	struct fifo name = FIFO_INIT(_fifo_data_##name, item_size, item_capacity)

/**
 * @brief Initialize a fifo queue.
 *
 * This routine initializes a fifo queue.
 *
 * @param queue Address of fifo queue.
 * @param data fifo queue data area.
 * @param item_size Size of each fifo queue item (in bytes).
 * @param item_capacity fifo queue capacity (in number of items).
 */
void fifo_init(struct fifo *queue, void *data, size_t item_size, size_t item_capacity);

/**
 * @brief Reset fifo queue state to its initial state.
 *
 * @param queue Address of fifo queue.
 */
void fifo_reset(struct fifo *queue);

/**
 * @brief Determine if a fifo queue is empty.
 *
 * @param queue Address of fifo queue.
 *
 * @return true if the fifo queue is empty, else false.
 */
bool fifo_empty(const struct fifo *queue);

/**
 * @brief Determine if a fifo queue is full.
 *
 * @param queue Address of fifo queue.
 *
 * @return true if the fifo queue is full, else false.
 */
bool fifo_full(const struct fifo *queue);

/**
 * @brief Determine free space in a fifo queue.
 *
 * @param queue Address of fifo queue.
 *
 * @return fifo queue free space in number of elements.
 */
size_t fifo_space(const struct fifo *queue);

/**
 * @brief Return fifo queue capacity.
 *
 * @param queue Address of fifo queue.
 *
 * @return fifo queue capacity in number of elements.
 */
size_t fifo_capacity(const struct fifo *queue);

/**
 * @brief Determine size of available data in a fifo queue.
 *
 * @param queue Address of fifo queue.
 *
 * @return fifo queue size in number of elements.
 */
size_t fifo_size(const struct fifo *queue);

/**
 * @brief Write (copy) data to a fifo queue.
 *
 * This routine writes data to a fifo queue @a queue.
 *
 * @param queue Address of fifo queue.
 * @param element Address of item.
 *
 * @retval 0 if successful, -ENOSPC if there is no space in the fifo.
 */
int fifo_put(struct fifo *queue, const void *element);

/**
 * @brief Read data from a fifo queue.
 *
 * This routine reads data from a fifo queue @a buf.
 *
 * @param queue Address of fifo queue.
 * @param element Address of the output buffer.
 *
 * @retval 0 if successful, -ENODATA if the fifo is empty.
 */
int fifo_get(struct fifo *queue, void *element);

/**
 * @brief Peek first element from a fifo queue.
 *
 * @param queue Address of fifo queue.
 * @param data Address of the output buffer.
 *
 * @retval 0 if successful, -ENODATA if the fifo is empty.
 */
int fifo_peek(struct fifo *queue, void *data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_FIFO_H_ */
