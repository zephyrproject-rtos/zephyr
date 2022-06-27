/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Generic double buffer data structure header.
 *
 * The structure may be used as a header for any kind of data structure that requires
 * double buffering.
 */
struct dbuf_hdr {
	/* The current element that is in use. */
	uint8_t volatile first;
	/* Last enqueued element. It will be used after buffer is swapped. */
	uint8_t last;
	/* Size in a bytes of a single element stored in double buffer. */
	uint8_t elem_size;
	/* Pointer for actual buffer memory. Its size should be 2 times @p elem_size. */
	uint8_t data[0];
};

/**
 * @brief Allocate element in double buffer.
 *
 * The function provides pointer to new element that may be modified by the user.
 * For consecutive calls it always returns the same pointer until buffer is swapped
 * by dbuf_latest_get operation.
 *
 * @param hdr Pointer to double buffer header.
 * @param idx Pointer to return new element index.
 *
 * @return Pointer to new element memory.
 */
void *dbuf_alloc(struct dbuf_hdr *hdr, uint8_t *idx);

/**
 * @brief Provides pointer to last allocated element.
 *
 * If it is called before dbuf_alloc it will return pointer to memory that was recently
 * enqueued.
 *
 * @param hdr Pointer to double buffer header.
 *
 * @return Pointer to last allocated element.
 */
static inline void *dbuf_peek(struct dbuf_hdr *hdr)
{
	return &hdr->data[hdr->last * hdr->elem_size];
}

/**
 * @brief  Enqueue new element for buffer swap.
 *
 * @param hdr Pointer to double buffer header.
 * @param idx Index of element to be swapped.
 */
static inline void dbuf_enqueue(struct dbuf_hdr *hdr, uint8_t idx)
{
	hdr->last = idx;
}

/**
 * @brief Get latest element in buffer.
 *
 * Latest enqueued element is pointed by last member of dbuf_hdr.
 * If it points to a different index than member first, then buffer will be
 * swapped and @p is_modified will be set to true.
 *
 * Pointer to latest element is returned.
 *
 * @param[in] hdr Pointer to double buffer header.
 * @param[out] is_modified Pointer to return information if buffer was swapped.
 *
 * @return Pointer to latest enqueued buffer element.
 */
void *dbuf_latest_get(struct dbuf_hdr *hdr, uint8_t *is_modified);

/**
 * @brief Returns pointer to the current element, the one after last swap operation.
 *
 * The function provides access to element that is pointed by member first of dbuf_hrd.
 * Returned value always points to latest one, that was swapped after most recent call to
 * dbuf_latest_get. The pointer will be the same as the one pointed by last if the dbuf_alloc
 * is not called after last dbuf_latest_get call.
 *
 * @param hdr Pointer to double buffer header.

 * @return Pointer to element after last buffer swap.
 */
static inline void *dbuf_curr_get(struct dbuf_hdr *hdr)
{
	return &hdr->data[hdr->first * hdr->elem_size];
}

/**
 * @brief Return information whether new element was enqueued to a buffer.
 *
 * @param hdr Pointer to double buffer header.
 *
 * @return True if there were new element enqueued, false in other case.
 */
static inline bool dbuf_is_modified(const struct dbuf_hdr *hdr)
{
	return hdr->first != hdr->last;
}
