/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header-file for fixed size circular queue
 * @ingroup sys_ringq_apis
 */

#ifndef ZEPHYR_INCLUDE_SYS_RINGQ_H_
#define ZEPHYR_INCLUDE_SYS_RINGQ_H_

#include <zephyr/sys/ring_buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup sys_ringq_apis Ring queue
 * @ingroup datastructure_apis
 * @brief Fixed size circular queue
 * @{
 */

/**
 * @brief An opaque structure representing a ringq queue
 */
struct sys_ringq {
	/** @cond INTERNAL_HIDDEN */
	struct ring_buf rb;
	ring_buf_idx_t item_size;
	/** @endcond */
};

/**
 * @brief Backing-buffer size (in bytes) required to store
 *        @p item_capacity elements of @p item_size bytes each.
 *
 * Includes the single byte reserved by the underlying ring buffer
 * to disambiguate the full and empty states.
 *
 * @param item_size     Size of each ringq element (in bytes).
 * @param item_capacity Capacity (in number of elements).
 */
#define SYS_RINGQ_STORAGE_SIZE(item_size, item_capacity) \
	RING_BUF_STORAGE_SIZE((item_size) * (item_capacity))

/** @cond INTERNAL_HIDDEN */

#define SYS_RINGQ_INIT(buf, item_sz, item_capacity)				\
{										\
	.rb = RING_BUF_INIT(buf, SYS_RINGQ_STORAGE_SIZE(item_sz, item_capacity)),\
	.item_size = item_sz,							\
}

/** @endcond */

/**
 * @brief Define and initialize a ring queue.
 *
 * This macro establishes a sys_ringq queue of a fixed size.
 *
 * @param name Name of the ringq instance.
 * @param item_size Size of each ringq element (in bytes).
 * @param item_capacity capacity (in number of elements).
 */
#define SYS_RINGQ_DEFINE(name, item_size, item_capacity)					\
	static uint8_t __noinit									\
		CONCAT(_ringq_data_, name)[SYS_RINGQ_STORAGE_SIZE(item_size, item_capacity)];	\
	struct sys_ringq name = SYS_RINGQ_INIT(CONCAT(_ringq_data_, name), item_size, item_capacity)

/**
 * @brief Initialize a ringq queue.
 *
 * This routine initializes a ringq queue.
 *
 * @param ringq Address of sys_ringq struct.
 * @param data ringq sys_ringq data area.
 * @param data_size Size of the ringq data area (in bytes). Must be at least
 * @p SYS_RINGQ_STORAGE_SIZE(item_size, 1) bytes; one byte is reserved
 * internally to disambiguate full vs empty.
 * @param item_size Size of each ringq element (in bytes).
 *
 * @note Usable capacity is @p (data_size - 1) / item_size elements. If
 * @p (data_size - 1) is not a multiple of @p item_size, it is rounded down.
 */
static inline void sys_ringq_init(struct sys_ringq *ringq, uint8_t *data, size_t data_size,
				size_t item_size)
{
	__ASSERT(data != NULL, "Data buffer should not be NULL");
	__ASSERT(item_size > 0, "item_size should be greater than 0");
	__ASSERT(data_size > item_size, "data_size must be > item_size");

	size_t items = (data_size - 1U) / item_size;

	__ASSERT(items > 0, "data_size must hold at least one item plus the sacrificed byte");

	ring_buf_init(&ringq->rb, SYS_RINGQ_STORAGE_SIZE(item_size, items), data);
	ringq->item_size = item_size;
}

/**
 * @brief Reset ringq queue state to its initial state.
 *
 * @param ringq Address of sys_ringq struct.
 */
static inline void sys_ringq_reset(struct sys_ringq *ringq)
{
	ring_buf_reset(&ringq->rb);
}

/**
 * @brief Determine if a sys_ringq is empty.
 *
 * @param ringq Address of sys_ringq struct.
 *
 * @return true if the sys_ringq is empty, else false.
 */
static inline bool sys_ringq_empty(const struct sys_ringq *ringq)
{
	return ring_buf_is_empty(&ringq->rb);
}

/**
 * @brief Determine if a sys_ringq is full.
 *
 * @param ringq Address of sys_ringq struct.
 *
 * @return true if the sys_ringq is full, else false.
 */
static inline bool sys_ringq_full(const struct sys_ringq *ringq)
{
	return ring_buf_space_get(&ringq->rb) == 0;
}

/**
 * @brief Determine free space in a ringq queue.
 *
 * @param ringq Address of ringq queue.
 *
 * @return ringq queue free space in number of elements.
 */
static inline size_t sys_ringq_space(const struct sys_ringq *ringq)
{
	return ring_buf_space_get(&ringq->rb) / ringq->item_size;
}

/**
 * @brief Return ringq queue capacity.
 *
 * @param ringq Address of sys_ringq struct.
 *
 * @return sys_ringq capacity in number of elements.
 */
static inline size_t sys_ringq_capacity(const struct sys_ringq *ringq)
{
	return ring_buf_capacity_get(&ringq->rb) / ringq->item_size;
}

/**
 * @brief Determine size of available data in a ringq queue.
 *
 * @param ringq Address of sys_ringq struct.
 *
 * @return sys_ringq size in number of elements.
 */
static inline size_t sys_ringq_size(const struct sys_ringq *ringq)
{
	return ring_buf_size_get(&ringq->rb) / ringq->item_size;
}

/**
 * @brief Write (copy) data to a ringq queue.
 *
 * This routine writes data to a ringq queue @a queue.
 *
 * @param ringq Address of sys_ringq queue.
 * @param element Address of item.
 *
 * @retval 0 if successful, -ENOSPC if there is no space in the ringq.
 */
static inline int sys_ringq_put(struct sys_ringq *ringq, const void *element)
{
	return ring_buf_put(&ringq->rb, (const uint8_t *)element, ringq->item_size) == 0 ?
		-ENOSPC : 0;
}

/**
 * @brief Read data from a sys_ringq queue.
 *
 * This routine reads data from a ringq queue @a buf.
 *
 * @param ringq Address of the sys_ringq struct.
 * @param element Address of the output buffer.
 *
 * @retval 0 if successful, -ENODATA if the ringq is empty.
 */
static inline int sys_ringq_get(struct sys_ringq *ringq, void *element)
{
	return ring_buf_get(&ringq->rb, (uint8_t *)element, ringq->item_size) == 0 ? -ENODATA : 0;
}

/**
 * @brief Peek first element from a sys_ringq queue.
 *
 * @param ringq Address of sys_ringq struct.
 * @param data Address of the output buffer.
 *
 * @retval 0 if successful, -ENODATA if the ringq is empty.
 */
static inline int sys_ringq_peek(struct sys_ringq *ringq, void *data)
{
	return ring_buf_peek(&ringq->rb, (uint8_t *)data, ringq->item_size) == 0 ? -ENODATA : 0;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_RINGQ_H_ */
