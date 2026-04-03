/*
 * SPDX-Copyright: Copyright (c) 2022 Intel Corporation
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RTIO Completion Queue Events and Related Functions
 */


#ifndef ZEPHYR_INCLUDE_RTIO_CQE_H_
#define ZEPHYR_INCLUDE_RTIO_CQE_H_

#include <stdint.h>
#include <string.h>
#include <zephyr/sys/mpsc_lockfree.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup rtio
 * @{
 */

/**
 * @brief RTIO CQE Flags
 * @defgroup rtio_cqe_flags RTIO CQE Flags
 * @ingroup rtio
 * @{
 */

/**
 * @brief The entry's buffer was allocated from the RTIO's mempool
 *
 * If this bit is set, the buffer was allocated from the memory pool and should be recycled as
 * soon as the application is done with it.
 */
#define RTIO_CQE_FLAG_MEMPOOL_BUFFER BIT(0)

#define RTIO_CQE_FLAG_GET(flags) FIELD_GET(GENMASK(7, 0), (flags))

/**
 * @brief Get the block index of a mempool flags
 *
 * @param flags The CQE flags value
 * @return The block index portion of the flags field.
 */
#define RTIO_CQE_FLAG_MEMPOOL_GET_BLK_IDX(flags) FIELD_GET(GENMASK(19, 8), (flags))

/**
 * @brief Get the block count of a mempool flags
 *
 * @param flags The CQE flags value
 * @return The block count portion of the flags field.
 */
#define RTIO_CQE_FLAG_MEMPOOL_GET_BLK_CNT(flags) FIELD_GET(GENMASK(31, 20), (flags))

/**
 * @brief Prepare CQE flags for a mempool read.
 *
 * @param blk_idx The mempool block index
 * @param blk_cnt The mempool block count
 * @return A shifted and masked value that can be added to the flags field with an OR operator.
 */
#define RTIO_CQE_FLAG_PREP_MEMPOOL(blk_idx, blk_cnt)                                               \
	(FIELD_PREP(GENMASK(7, 0), RTIO_CQE_FLAG_MEMPOOL_BUFFER) |                                 \
	 FIELD_PREP(GENMASK(19, 8), blk_idx) | FIELD_PREP(GENMASK(31, 20), blk_cnt))

/**
 * @}
 */

/**
 * @brief A completion queue event
 */
struct rtio_cqe {
	struct mpsc_node q;

	int32_t result; /**< Result from operation */
	void *userdata; /**< Associated userdata with operation */
	uint32_t flags; /**< Flags associated with the operation */
};

/* Private structures and functions used for the pool of cqe structures */
/** @cond ignore */

struct rtio_cqe_pool {
	struct mpsc free_q;
	const uint16_t pool_size;
	uint16_t pool_free;
	struct rtio_cqe *pool;
};


static inline struct rtio_cqe *rtio_cqe_pool_alloc(struct rtio_cqe_pool *pool)
{
	struct mpsc_node *node = mpsc_pop(&pool->free_q);

	if (node == NULL) {
		return NULL;
	}

	struct rtio_cqe *cqe = CONTAINER_OF(node, struct rtio_cqe, q);

	memset(cqe, 0, sizeof(struct rtio_cqe));

	pool->pool_free--;

	return cqe;
}

static inline void rtio_cqe_pool_free(struct rtio_cqe_pool *pool, struct rtio_cqe *cqe)
{
	mpsc_push(&pool->free_q, &cqe->q);

	pool->pool_free++;
}

/* Do not try and reformat the macros */
/* clang-format off */

#define Z_RTIO_CQE_POOL_DEFINE(name, sz)			\
	static struct rtio_cqe CONCAT(_cqe_pool_, name)[sz];	\
	STRUCT_SECTION_ITERABLE(rtio_cqe_pool, name) = {	\
		.free_q = MPSC_INIT((name.free_q)),	\
		.pool_size = sz,				\
		.pool_free = sz,				\
		.pool = CONCAT(_cqe_pool_, name),		\
	}

/* clang-format on */
/** @endcond */


/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RTIO_CQE_H_ */
