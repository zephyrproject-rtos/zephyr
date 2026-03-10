/*
 * SPDX-FileCopyrightText: Copyright (c) 2022 Intel Corporation
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Real-Time IO device API for moving bytes with low effort
 *
 * RTIO is a context for asynchronous batch operations using a submission and completion queue.
 *
 * Asynchronous I/O operation are setup in a submission queue. Each entry in the queue describes
 * the operation it wishes to perform with some understood semantics.
 *
 * These operations may be chained in a such a way that only when the current
 * operation is complete the next will be executed. If the current operation fails
 * all chained operations will also fail.
 *
 * Operations may also be submitted as a transaction where a set of operations are considered
 * to be one operation.
 *
 * The completion of these operations typically provide one or more completion queue events.
 */

#ifndef ZEPHYR_INCLUDE_RTIO_RTIO_H_
#define ZEPHYR_INCLUDE_RTIO_RTIO_H_

#include <string.h>

#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/mem_blocks.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/rtio/sqe.h>
#include <zephyr/rtio/cqe.h>
#include <zephyr/rtio/iodev.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief RTIO
 * @defgroup rtio RTIO
 * @since 3.2
 * @version 0.2.0
 * @ingroup os_services
 * @{
 */

/**
 * @brief An RTIO context containing what can be viewed as a pair of queues.
 *
 * A queue for submissions (available and in queue to be produced) as well as a queue
 * of completions (available and ready to be consumed).
 *
 * The rtio executor along with any objects implementing the rtio_iodev interface are
 * the consumers of submissions and producers of completions.
 *
 * No work is started until rtio_submit() is called.
 */
struct rtio {
#ifdef CONFIG_RTIO_SUBMIT_SEM
	/* A wait semaphore which may suspend the calling thread
	 * to wait for some number of completions when calling submit
	 */
	struct k_sem *submit_sem;

	uint32_t submit_count;
#endif

#ifdef CONFIG_RTIO_CONSUME_SEM
	/* A wait semaphore which may suspend the calling thread
	 * to wait for some number of completions while consuming
	 * them from the completion queue
	 */
	struct k_sem *consume_sem;
#endif

	/* Total number of completions */
	atomic_t cq_count;

	/* Number of completions that were unable to be submitted with results
	 * due to the cq spsc being full
	 */
	atomic_t xcqcnt;

	/* Submission queue object pool with free list */
	struct rtio_sqe_pool *sqe_pool;

	/* Complete queue object pool with free list */
	struct rtio_cqe_pool *cqe_pool;

#ifdef CONFIG_RTIO_SYS_MEM_BLOCKS
	/* Mem block pool */
	struct sys_mem_blocks *block_pool;
#endif

	/* Submission queue */
	struct mpsc sq;

	/* Completion queue */
	struct mpsc cq;
};

/* @cond ignore */
#define Z_RTIO_DEFINE(name, _sqe_pool, _cqe_pool, _block_pool)                                     \
	IF_ENABLED(CONFIG_RTIO_SUBMIT_SEM,                                                         \
		   (static K_SEM_DEFINE(CONCAT(_submit_sem_, name), 0, K_SEM_MAX_LIMIT)))          \
	IF_ENABLED(CONFIG_RTIO_CONSUME_SEM,                                                        \
		   (static K_SEM_DEFINE(CONCAT(_consume_sem_, name), 0, K_SEM_MAX_LIMIT)))         \
	STRUCT_SECTION_ITERABLE(rtio, name) = {                                                    \
		IF_ENABLED(CONFIG_RTIO_SUBMIT_SEM, (.submit_sem = &CONCAT(_submit_sem_, name),))   \
		IF_ENABLED(CONFIG_RTIO_SUBMIT_SEM, (.submit_count = 0,))                           \
		IF_ENABLED(CONFIG_RTIO_CONSUME_SEM, (.consume_sem = &CONCAT(_consume_sem_, name),))\
		.cq_count = ATOMIC_INIT(0),                                                        \
		.xcqcnt = ATOMIC_INIT(0),                                                          \
		.sqe_pool = _sqe_pool,                                                             \
		.cqe_pool = _cqe_pool,                                                             \
		IF_ENABLED(CONFIG_RTIO_SYS_MEM_BLOCKS, (.block_pool = _block_pool,))               \
		.sq = MPSC_INIT((name.sq)),                                                        \
		.cq = MPSC_INIT((name.cq)),                                                        \
	}
/* @endcond */

/**
 * @brief Statically define and initialize an RTIO context
 *
 * @param name Name of the RTIO
 * @param sq_sz Size of the submission queue entry pool
 * @param cq_sz Size of the completion queue entry pool
 */
#define RTIO_DEFINE(name, sq_sz, cq_sz)						\
	Z_RTIO_SQE_POOL_DEFINE(CONCAT(name, _sqe_pool), sq_sz);			\
	Z_RTIO_CQE_POOL_DEFINE(CONCAT(name, _cqe_pool), cq_sz);			\
	Z_RTIO_DEFINE(name, &CONCAT(name, _sqe_pool),				\
		      &CONCAT(name, _cqe_pool), NULL)


/**
 * @brief Get the mempool block size of the RTIO context
 *
 * @param[in] r The RTIO context
 * @return The size of each block in the context's mempool
 * @return 0 if the context doesn't have a mempool
 */
static inline size_t rtio_mempool_block_size(const struct rtio *r)
{
#ifndef CONFIG_RTIO_SYS_MEM_BLOCKS
	ARG_UNUSED(r);
	return 0;
#else
	if (r == NULL || r->block_pool == NULL) {
		return 0;
	}
	return BIT(r->block_pool->info.blk_sz_shift);
#endif
}

/**
 * @brief Compute the mempool block index for a given pointer
 *
 * @param[in] r RTIO context
 * @param[in] ptr Memory pointer in the mempool
 * @return Index of the mempool block associated with the pointer. Or UINT16_MAX if invalid.
 */
#if defined(CONFIG_RTIO_SYS_MEM_BLOCKS) || defined(__DOXYGEN__)
static inline uint16_t __rtio_compute_mempool_block_index(const struct rtio *r, const void *ptr)
{
	uintptr_t addr = (uintptr_t)ptr;
	struct sys_mem_blocks *mem_pool = r->block_pool;
	uint32_t block_size = rtio_mempool_block_size(r);

	uintptr_t buff = (uintptr_t)mem_pool->buffer;
	uint32_t buff_size = mem_pool->info.num_blocks * block_size;

	if (addr < buff || addr >= buff + buff_size) {
		return UINT16_MAX;
	}
	return (addr - buff) / block_size;
}
#endif

static inline int rtio_block_pool_alloc(struct rtio *r, size_t min_sz,
					  size_t max_sz, uint8_t **buf, uint32_t *buf_len)
{
#ifndef CONFIG_RTIO_SYS_MEM_BLOCKS
	ARG_UNUSED(r);
	ARG_UNUSED(min_sz);
	ARG_UNUSED(max_sz);
	ARG_UNUSED(buf);
	ARG_UNUSED(buf_len);
	return -ENOTSUP;
#else
	const uint32_t block_size = rtio_mempool_block_size(r);
	uint32_t bytes = max_sz;

	/* Not every context has a block pool and the block size may return 0 in
	 * that case
	 */
	if (block_size == 0) {
		return -ENOMEM;
	}

	do {
		size_t num_blks = DIV_ROUND_UP(bytes, block_size);
		int rc = sys_mem_blocks_alloc_contiguous(r->block_pool, num_blks, (void **)buf);

		if (rc == 0) {
			*buf_len = num_blks * block_size;
			return 0;
		}

		if (bytes <= block_size) {
			break;
		}

		bytes -= block_size;
	} while (bytes >= min_sz);

	return -ENOMEM;
#endif
}

static inline void rtio_block_pool_free(struct rtio *r, void *buf, uint32_t buf_len)
{
#ifndef CONFIG_RTIO_SYS_MEM_BLOCKS
	ARG_UNUSED(r);
	ARG_UNUSED(buf);
	ARG_UNUSED(buf_len);
#else
	size_t num_blks = buf_len >> r->block_pool->info.blk_sz_shift;

	sys_mem_blocks_free_contiguous(r->block_pool, buf, num_blks);
#endif
}


/** The memory partition associated with all RTIO context information */
extern struct k_mem_partition rtio_partition;


/* Do not try and reformat the macros */

/**
 * @brief Allocate to bss if available
 *
 * If CONFIG_USERSPACE is selected, allocate to the rtio_partition bss. Maps to:
 *   K_APP_BMEM(rtio_partition) static
 *
 * If CONFIG_USERSPACE is disabled, allocate as plain static:
 *   static
 */
#define RTIO_BMEM COND_CODE_1(CONFIG_USERSPACE, (K_APP_BMEM(rtio_partition) static), (static))

/**
 * @brief Allocate as initialized memory if available
 *
 * If CONFIG_USERSPACE is selected, allocate to the rtio_partition init. Maps to:
 *   K_APP_DMEM(rtio_partition) static
 *
 * If CONFIG_USERSPACE is disabled, allocate as plain static:
 *   static
 */
#define RTIO_DMEM COND_CODE_1(CONFIG_USERSPACE, (K_APP_DMEM(rtio_partition) static), (static))

/* clang-format off */
/* @cond ignore */
#define Z_RTIO_BLOCK_POOL_DEFINE(name, blk_sz, blk_cnt, blk_align)                                 \
	RTIO_BMEM uint8_t __aligned(WB_UP(blk_align))                                              \
	CONCAT(_block_pool_, name)[blk_cnt*WB_UP(blk_sz)];                                         \
	_SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF(name, WB_UP(blk_sz), blk_cnt,                          \
					    CONCAT(_block_pool_, name),	RTIO_DMEM)

/* @endcond */


/* clang-format on */

/**
 * @brief Statically define and initialize an RTIO context
 *
 * @param name Name of the RTIO
 * @param sq_sz Size of the submission queue, must be power of 2
 * @param cq_sz Size of the completion queue, must be power of 2
 * @param num_blks Number of blocks in the memory pool
 * @param blk_size The number of bytes in each block
 * @param balign The block alignment
 */
#define RTIO_DEFINE_WITH_MEMPOOL(name, sq_sz, cq_sz, num_blks, blk_size, balign) \
	Z_RTIO_SQE_POOL_DEFINE(name##_sqe_pool, sq_sz);		\
	Z_RTIO_CQE_POOL_DEFINE(name##_cqe_pool, cq_sz);			\
	Z_RTIO_BLOCK_POOL_DEFINE(name##_block_pool, blk_size, num_blks, balign); \
	Z_RTIO_DEFINE(name, &name##_sqe_pool, &name##_cqe_pool, &name##_block_pool)

/* clang-format on */

/**
 * @brief Count of acquirable submission queue events
 *
 * @param r RTIO context
 *
 * @return Count of acquirable submission queue events
 */
static inline uint32_t rtio_sqe_acquirable(struct rtio *r)
{
	return r->sqe_pool->pool_free;
}

/**
 * @brief Acquire a single submission queue event if available
 *
 * @param r RTIO context
 *
 * @retval sqe A valid submission queue event acquired from the submission queue
 * @retval NULL No subsmission queue event available
 */
static inline struct rtio_sqe *rtio_sqe_acquire(struct rtio *r)
{
	SYS_PORT_TRACING_FUNC_ENTER(rtio, sqe_acquire, r);
	struct rtio_iodev_sqe *iodev_sqe = rtio_sqe_pool_alloc(r->sqe_pool);

	if (iodev_sqe == NULL) {
		SYS_PORT_TRACING_FUNC_EXIT(rtio, sqe_acquire, r, NULL);
		return NULL;
	}

	mpsc_push(&r->sq, &iodev_sqe->q);

	SYS_PORT_TRACING_FUNC_EXIT(rtio, sqe_acquire, r, &iodev_sqe->sqe);
	return &iodev_sqe->sqe;
}

/**
 * @brief Acquire a number of submission queue events if available
 *
 * @warning The sqe array is in an undefined state if the return value is -ENOMEM
 *
 * @param r RTIO context
 * @param n Number of submission queue events to acquire
 * @param sqes A pointer to an array of rtio_sqe struct pointers of size n
 *
 * @retval 0 success
 * @retval -ENOMEM out of memory
 */
static inline int rtio_sqe_acquire_array(struct rtio *r, size_t n, struct rtio_sqe **sqes)
{
	struct rtio_iodev_sqe *iodev_sqe;
	size_t i;

	for (i = 0; i < n; i++) {
		iodev_sqe = rtio_sqe_pool_alloc(r->sqe_pool);
		if (iodev_sqe == NULL) {
			break;
		}
		sqes[i] = &iodev_sqe->sqe;
	}

	/* Not enough SQEs in the pool */
	if (i < n) {
		while (i > 0) {
			i--;
			iodev_sqe = CONTAINER_OF(sqes[i], struct rtio_iodev_sqe, sqe);
			rtio_sqe_pool_free(r->sqe_pool, iodev_sqe);
			sqes[i] = NULL;
		}

		return -ENOMEM;
	}

	for (i = 0; i < n; i++) {
		iodev_sqe = CONTAINER_OF(sqes[i], struct rtio_iodev_sqe, sqe);
		mpsc_push(&r->sq, &iodev_sqe->q);
	}

	return 0;
}

/**
 * @brief Drop all previously acquired sqe
 *
 * @param r RTIO context
 */
static inline void rtio_sqe_drop_all(struct rtio *r)
{
	struct rtio_iodev_sqe *iodev_sqe;
	struct mpsc_node *node = mpsc_pop(&r->sq);

	while (node != NULL) {
		iodev_sqe = CONTAINER_OF(node, struct rtio_iodev_sqe, q);
		rtio_sqe_pool_free(r->sqe_pool, iodev_sqe);
		node = mpsc_pop(&r->sq);
	}
}

/**
 * @brief Acquire a complete queue event if available
 */
static inline struct rtio_cqe *rtio_cqe_acquire(struct rtio *r)
{
	SYS_PORT_TRACING_FUNC_ENTER(rtio, cqe_acquire, r);
	struct rtio_cqe *cqe = rtio_cqe_pool_alloc(r->cqe_pool);

	if (cqe == NULL) {
		SYS_PORT_TRACING_FUNC_EXIT(rtio, cqe_acquire, r, NULL);
		return NULL;
	}

	memset(cqe, 0, sizeof(struct rtio_cqe));

	SYS_PORT_TRACING_FUNC_EXIT(rtio, cqe_acquire, r, cqe);
	return cqe;
}

/**
 * @brief Produce a complete queue event if available
 */
static inline void rtio_cqe_produce(struct rtio *r, struct rtio_cqe *cqe)
{
	mpsc_push(&r->cq, &cqe->q);
}

/**
 * @brief Consume a single completion queue event if available
 *
 * If a completion queue event is returned rtio_cq_release(r) must be called
 * at some point to release the cqe spot for the cqe producer.
 *
 * @param r RTIO context
 *
 * @retval cqe A valid completion queue event consumed from the completion queue
 * @retval NULL No completion queue event available
 */
static inline struct rtio_cqe *rtio_cqe_consume(struct rtio *r)
{
	SYS_PORT_TRACING_FUNC_ENTER(rtio, cqe_consume, r);
	struct mpsc_node *node;
	struct rtio_cqe *cqe = NULL;

#ifdef CONFIG_RTIO_CONSUME_SEM
	if (k_sem_take(r->consume_sem, K_NO_WAIT) != 0) {
		SYS_PORT_TRACING_FUNC_EXIT(rtio, cqe_consume, r, NULL);
		return NULL;
	}
#endif

	node = mpsc_pop(&r->cq);
	if (node == NULL) {
		SYS_PORT_TRACING_FUNC_EXIT(rtio, cqe_consume, r, NULL);
		return NULL;
	}
	cqe = CONTAINER_OF(node, struct rtio_cqe, q);

	SYS_PORT_TRACING_FUNC_EXIT(rtio, cqe_consume, r, cqe);
	return cqe;
}

/**
 * @brief Wait for and consume a single completion queue event
 *
 * If a completion queue event is returned rtio_cq_release(r) must be called
 * at some point to release the cqe spot for the cqe producer.
 *
 * @param r RTIO context
 *
 * @retval cqe A valid completion queue event consumed from the completion queue
 */
static inline struct rtio_cqe *rtio_cqe_consume_block(struct rtio *r)
{
	struct mpsc_node *node;
	struct rtio_cqe *cqe;

#ifdef CONFIG_RTIO_CONSUME_SEM
	k_sem_take(r->consume_sem, K_FOREVER);
#endif
	node = mpsc_pop(&r->cq);
	while (node == NULL) {
		Z_SPIN_DELAY(1);
		node = mpsc_pop(&r->cq);
	}
	cqe = CONTAINER_OF(node, struct rtio_cqe, q);

	return cqe;
}

/**
 * @brief Release consumed completion queue event
 *
 * @param r RTIO context
 * @param cqe Completion queue entry
 */
static inline void rtio_cqe_release(struct rtio *r, struct rtio_cqe *cqe)
{
	SYS_PORT_TRACING_FUNC(rtio, cqe_release, r, cqe);
	rtio_cqe_pool_free(r->cqe_pool, cqe);
}

/**
 * @brief Flush completion queue
 *
 * @param r RTIO context
 * @return The operation completion result
 * @retval 0 if the queued operations completed with no error
 * @retval <0 on error
 */
static inline int rtio_flush_completion_queue(struct rtio *r)
{
	struct rtio_cqe *cqe;
	int res = 0;

	do {
		cqe = rtio_cqe_consume(r);
		if (cqe != NULL) {
			if ((cqe->result < 0) && (res == 0)) {
				res = cqe->result;
			}
			rtio_cqe_release(r, cqe);
		}
	} while (cqe != NULL);

	return res;
}

/**
 * @brief Signal an AWAIT SQE
 *
 * If the SQE is currently blocking execution, execution is unblocked. If the SQE is not
 * currently blocking execution, The SQE will be skipped.
 *
 * @note To await the AWAIT SQE blocking execution, chain a nop or callback SQE before
 * the await SQE.
 *
 * @param[in] sqe The SQE to signal
 */
__syscall void rtio_sqe_signal(struct rtio_sqe *sqe);

static inline void z_impl_rtio_sqe_signal(struct rtio_sqe *sqe)
{
	struct rtio_iodev_sqe *iodev_sqe = CONTAINER_OF(sqe, struct rtio_iodev_sqe, sqe);

	if (!atomic_cas(&iodev_sqe->sqe.await.ok, 0, 1)) {
		iodev_sqe->sqe.await.callback(iodev_sqe, iodev_sqe->sqe.await.userdata);
	}
}

/**
 * @brief Compute the CQE flags from the rtio_iodev_sqe entry
 *
 * @param iodev_sqe The SQE entry in question.
 * @return The value that should be set for the CQE's flags field.
 */
static inline uint32_t rtio_cqe_compute_flags(struct rtio_iodev_sqe *iodev_sqe)
{
	uint32_t flags = 0;

#ifdef CONFIG_RTIO_SYS_MEM_BLOCKS
	if (iodev_sqe->sqe.op == RTIO_OP_RX && iodev_sqe->sqe.flags & RTIO_SQE_MEMPOOL_BUFFER) {
		struct rtio *r = iodev_sqe->r;
		struct sys_mem_blocks *mem_pool = r->block_pool;
		unsigned int blk_index = 0;
		unsigned int blk_count = 0;

		if (iodev_sqe->sqe.rx.buf) {
			blk_index = (iodev_sqe->sqe.rx.buf - mem_pool->buffer) >>
				    mem_pool->info.blk_sz_shift;
			blk_count = iodev_sqe->sqe.rx.buf_len >> mem_pool->info.blk_sz_shift;
		}
		flags = RTIO_CQE_FLAG_PREP_MEMPOOL(blk_index, blk_count);
	}
#else
	ARG_UNUSED(iodev_sqe);
#endif

	return flags;
}

/**
 * @brief Retrieve the mempool buffer that was allocated for the CQE.
 *
 * If the RTIO context contains a memory pool, and the SQE was created by calling
 * rtio_sqe_read_with_pool(), this function can be used to retrieve the memory associated with the
 * read. Once processing is done, it should be released by calling rtio_release_buffer().
 *
 * @param[in] r RTIO context
 * @param[in] cqe The CQE handling the event.
 * @param[out] buff Pointer to the mempool buffer
 * @param[out] buff_len Length of the allocated buffer
 * @return 0 on success
 * @return -EINVAL if the buffer wasn't allocated for this cqe
 * @return -ENOTSUP if memory blocks are disabled
 */
__syscall int rtio_cqe_get_mempool_buffer(const struct rtio *r, struct rtio_cqe *cqe,
					  uint8_t **buff, uint32_t *buff_len);

static inline int z_impl_rtio_cqe_get_mempool_buffer(const struct rtio *r, struct rtio_cqe *cqe,
						     uint8_t **buff, uint32_t *buff_len)
{
#ifdef CONFIG_RTIO_SYS_MEM_BLOCKS
	if (RTIO_CQE_FLAG_GET(cqe->flags) == RTIO_CQE_FLAG_MEMPOOL_BUFFER) {
		unsigned int blk_idx = RTIO_CQE_FLAG_MEMPOOL_GET_BLK_IDX(cqe->flags);
		unsigned int blk_count = RTIO_CQE_FLAG_MEMPOOL_GET_BLK_CNT(cqe->flags);
		uint32_t blk_size = rtio_mempool_block_size(r);

		*buff_len = blk_count * blk_size;

		if (blk_count > 0) {
			*buff = r->block_pool->buffer + blk_idx * blk_size;

			__ASSERT_NO_MSG(*buff >= r->block_pool->buffer);
			__ASSERT_NO_MSG(*buff <
				r->block_pool->buffer + blk_size * r->block_pool->info.num_blocks);
		} else {
			*buff = NULL;
		}
		return 0;
	}
	return -EINVAL;
#else
	ARG_UNUSED(r);
	ARG_UNUSED(cqe);
	ARG_UNUSED(buff);
	ARG_UNUSED(buff_len);

	return -ENOTSUP;
#endif
}

void rtio_executor_submit(struct rtio *r);
void rtio_executor_ok(struct rtio_iodev_sqe *iodev_sqe, int result);
void rtio_executor_err(struct rtio_iodev_sqe *iodev_sqe, int result);

/**
 * @brief Inform the executor of a submission completion with success
 *
 * This may start the next asynchronous request if one is available.
 *
 * @param iodev_sqe IODev Submission that has succeeded
 * @param result Result of the request
 */
static inline void rtio_iodev_sqe_ok(struct rtio_iodev_sqe *iodev_sqe, int result)
{
	rtio_executor_ok(iodev_sqe, result);
}

/**
 * @brief Inform the executor of a submissions completion with error
 *
 * This SHALL fail the remaining submissions in the chain.
 *
 * @param iodev_sqe Submission that has failed
 * @param result Result of the request
 */
static inline void rtio_iodev_sqe_err(struct rtio_iodev_sqe *iodev_sqe, int result)
{
	rtio_executor_err(iodev_sqe, result);
}

/**
 * Submit a completion queue event with a given result and userdata
 *
 * Called by the executor to produce a completion queue event, no inherent
 * locking is performed and this is not safe to do from multiple callers.
 *
 * @param r RTIO context
 * @param result Integer result code (could be -errno)
 * @param userdata Userdata to pass along to completion
 * @param flags Flags to use for the CEQ see RTIO_CQE_FLAG_*
 */
static inline void rtio_cqe_submit(struct rtio *r, int result, void *userdata, uint32_t flags)
{
	SYS_PORT_TRACING_FUNC_ENTER(rtio, cqe_submit, r, result, flags);
	struct rtio_cqe *cqe = rtio_cqe_acquire(r);

	if (cqe == NULL) {
		atomic_inc(&r->xcqcnt);
	} else {
		cqe->result = result;
		cqe->userdata = userdata;
		cqe->flags = flags;
		rtio_cqe_produce(r, cqe);
#ifdef CONFIG_RTIO_CONSUME_SEM
		k_sem_give(r->consume_sem);
#endif
	}

	/* atomic_t isn't guaranteed to wrap correctly as it could be signed, so
	 * we must resort to a cas loop.
	 */
	atomic_t val, new_val;

	do {
		val = atomic_get(&r->cq_count);
		new_val = (atomic_t)((uintptr_t)val + 1);
	} while (!atomic_cas(&r->cq_count, val, new_val));

#ifdef CONFIG_RTIO_SUBMIT_SEM
	if (r->submit_count > 0) {
		r->submit_count--;
		if (r->submit_count == 0) {
			k_sem_give(r->submit_sem);
		}
	}
#endif
	SYS_PORT_TRACING_FUNC_EXIT(rtio, cqe_submit, r);
}

#define __RTIO_MEMPOOL_GET_NUM_BLKS(num_bytes, blk_size) (((num_bytes) + (blk_size)-1) / (blk_size))

/**
 * @brief Get the buffer associate with the RX submission
 *
 * @param[in] iodev_sqe   The submission to probe
 * @param[in] min_buf_len The minimum number of bytes needed for the operation
 * @param[in] max_buf_len The maximum number of bytes needed for the operation
 * @param[out] buf        Where to store the pointer to the buffer
 * @param[out] buf_len    Where to store the size of the buffer
 *
 * @return 0 if @p buf and @p buf_len were successfully filled
 * @return -ENOMEM Not enough memory for @p min_buf_len
 */
static inline int rtio_sqe_rx_buf(const struct rtio_iodev_sqe *iodev_sqe, uint32_t min_buf_len,
				  uint32_t max_buf_len, uint8_t **buf, uint32_t *buf_len)
{
	struct rtio_sqe *sqe = (struct rtio_sqe *)&iodev_sqe->sqe;

#ifdef CONFIG_RTIO_SYS_MEM_BLOCKS
	if (sqe->op == RTIO_OP_RX && sqe->flags & RTIO_SQE_MEMPOOL_BUFFER) {
		struct rtio *r = iodev_sqe->r;

		if (sqe->rx.buf != NULL) {
			if (sqe->rx.buf_len < min_buf_len) {
				return -ENOMEM;
			}
			*buf = sqe->rx.buf;
			*buf_len = sqe->rx.buf_len;
			return 0;
		}

		int rc = rtio_block_pool_alloc(r, min_buf_len, max_buf_len, buf, buf_len);
		if (rc == 0) {
			sqe->rx.buf = *buf;
			sqe->rx.buf_len = *buf_len;
			return 0;
		}

		return -ENOMEM;
	}
#else
	ARG_UNUSED(max_buf_len);
#endif

	if (sqe->rx.buf_len < min_buf_len) {
		return -ENOMEM;
	}

	*buf = sqe->rx.buf;
	*buf_len = sqe->rx.buf_len;
	return 0;
}

/**
 * @brief Release memory that was allocated by the RTIO's memory pool
 *
 * If the RTIO context was created by a call to RTIO_DEFINE_WITH_MEMPOOL(), then the cqe data might
 * contain a buffer that's owned by the RTIO context. In those cases (if the read request was
 * configured via rtio_sqe_read_with_pool()) the buffer must be returned back to the pool.
 *
 * Call this function when processing is complete. This function will validate that the memory
 * actually belongs to the RTIO context and will ignore invalid arguments.
 *
 * @param r RTIO context
 * @param buff Pointer to the buffer to be released.
 * @param buff_len Number of bytes to free (will be rounded up to nearest memory block).
 */
__syscall void rtio_release_buffer(struct rtio *r, void *buff, uint32_t buff_len);

static inline void z_impl_rtio_release_buffer(struct rtio *r, void *buff, uint32_t buff_len)
{
#ifdef CONFIG_RTIO_SYS_MEM_BLOCKS
	if (r == NULL || buff == NULL || r->block_pool == NULL || buff_len == 0) {
		return;
	}

	rtio_block_pool_free(r, buff, buff_len);
#else
	ARG_UNUSED(r);
	ARG_UNUSED(buff);
	ARG_UNUSED(buff_len);
#endif
}

/**
 * Grant access to an RTIO context to a user thread
 *
 * @param r RTIO context
 * @param t Thread to grant permissions to
 */
static inline void rtio_access_grant(struct rtio *r, struct k_thread *t)
{
	k_object_access_grant(r, t);

#ifdef CONFIG_RTIO_SUBMIT_SEM
	k_object_access_grant(r->submit_sem, t);
#endif

#ifdef CONFIG_RTIO_CONSUME_SEM
	k_object_access_grant(r->consume_sem, t);
#endif
}


/**
 * Revoke access to an RTIO context from a user thread
 *
 * @param r RTIO context
 * @param t Thread to revoke permissions from
 */
static inline void rtio_access_revoke(struct rtio *r, struct k_thread *t)
{
	k_object_access_revoke(r, t);

#ifdef CONFIG_RTIO_SUBMIT_SEM
	k_object_access_revoke(r->submit_sem, t);
#endif

#ifdef CONFIG_RTIO_CONSUME_SEM
	k_object_access_revoke(r->consume_sem, t);
#endif
}

/**
 * @brief Attempt to cancel an SQE
 *
 * If possible (not currently executing), cancel an SQE and generate a failure with -ECANCELED
 * result.
 *
 * @param[in] sqe The SQE to cancel
 * @return 0 if the SQE was flagged for cancellation
 * @return <0 on error
 */
__syscall int rtio_sqe_cancel(struct rtio_sqe *sqe);

static inline int z_impl_rtio_sqe_cancel(struct rtio_sqe *sqe)
{
	SYS_PORT_TRACING_FUNC(rtio, sqe_cancel, sqe);
	struct rtio_iodev_sqe *iodev_sqe = CONTAINER_OF(sqe, struct rtio_iodev_sqe, sqe);

	do {
		iodev_sqe->sqe.flags |= RTIO_SQE_CANCELED;
		iodev_sqe = rtio_iodev_sqe_next(iodev_sqe);
	} while (iodev_sqe != NULL);

	return 0;
}

/**
 * @brief Copy an array of SQEs into the queue and get resulting handles back
 *
 * Copies one or more SQEs into the RTIO context and optionally returns their generated SQE handles.
 * Handles can be used to cancel events via the rtio_sqe_cancel() call.
 *
 * @param[in]  r RTIO context
 * @param[in]  sqes Pointer to an array of SQEs
 * @param[out] handle Optional pointer to @ref rtio_sqe pointer to store the handle of the
 *             first generated SQE. Use NULL to ignore.
 * @param[in]  sqe_count Count of sqes in array
 *
 * @retval 0 success
 * @retval -ENOMEM not enough room in the queue
 */
__syscall int rtio_sqe_copy_in_get_handles(struct rtio *r, const struct rtio_sqe *sqes,
					   struct rtio_sqe **handle, size_t sqe_count);

static inline int z_impl_rtio_sqe_copy_in_get_handles(struct rtio *r, const struct rtio_sqe *sqes,
						      struct rtio_sqe **handle,
						      size_t sqe_count)
{
	struct rtio_sqe *sqe;
	uint32_t acquirable = rtio_sqe_acquirable(r);

	if (acquirable < sqe_count) {
		return -ENOMEM;
	}

	for (unsigned long i = 0; i < sqe_count; i++) {
		sqe = rtio_sqe_acquire(r);
		__ASSERT_NO_MSG(sqe != NULL);
		if (handle != NULL && i == 0) {
			*handle = sqe;
		}
		*sqe = sqes[i];
	}

	return 0;
}

/**
 * @brief Copy an array of SQEs into the queue
 *
 * Useful if a batch of submissions is stored in ROM or
 * RTIO is used from user mode where a copy must be made.
 *
 * Partial copying is not done as chained SQEs need to be submitted
 * as a whole set.
 *
 * @param r RTIO context
 * @param sqes Pointer to an array of SQEs
 * @param sqe_count Count of sqes in array
 *
 * @retval 0 success
 * @retval -ENOMEM not enough room in the queue
 */
static inline int rtio_sqe_copy_in(struct rtio *r, const struct rtio_sqe *sqes, size_t sqe_count)
{
	return rtio_sqe_copy_in_get_handles(r, sqes, NULL, sqe_count);
}

/**
 * @brief Copy an array of CQEs from the queue
 *
 * Copies from the RTIO context and its queue completion queue
 * events, waiting for the given time period to gather the number
 * of completions requested.
 *
 * @param r RTIO context
 * @param cqes Pointer to an array of SQEs
 * @param cqe_count Count of sqes in array
 * @param timeout Timeout to wait for each completion event. Total wait time is
 *                potentially timeout*cqe_count at maximum.
 *
 * @retval copy_count Count of copied CQEs (0 to cqe_count)
 */
__syscall int rtio_cqe_copy_out(struct rtio *r,
				struct rtio_cqe *cqes,
				size_t cqe_count,
				k_timeout_t timeout);
static inline int z_impl_rtio_cqe_copy_out(struct rtio *r,
					   struct rtio_cqe *cqes,
					   size_t cqe_count,
					   k_timeout_t timeout)
{
	size_t copied = 0;
	struct rtio_cqe *cqe;
	k_timepoint_t end = sys_timepoint_calc(timeout);

	do {
		cqe = K_TIMEOUT_EQ(timeout, K_FOREVER) ? rtio_cqe_consume_block(r)
						       : rtio_cqe_consume(r);
		if (cqe == NULL) {
			Z_SPIN_DELAY(25);
			continue;
		}
		cqes[copied++] = *cqe;
		rtio_cqe_release(r, cqe);
	} while (copied < cqe_count && !sys_timepoint_expired(end));

	return copied;
}

/**
 * @brief Submit I/O requests to the underlying executor
 *
 * Submits the queue of submission queue events to the executor.
 * The executor will do the work of managing tasks representing each
 * submission chain, freeing submission queue events when done, and
 * producing completion queue events as submissions are completed.
 *
 * @warning It is undefined behavior to have re-entrant calls to submit
 *
 * @param r RTIO context
 * @param wait_count Number of submissions to wait for completion of.
 *
 * @retval 0 On success
 */
__syscall int rtio_submit(struct rtio *r, uint32_t wait_count);

#ifdef CONFIG_RTIO_SUBMIT_SEM
static inline int z_impl_rtio_submit(struct rtio *r, uint32_t wait_count)
{
	SYS_PORT_TRACING_FUNC_ENTER(rtio, submit, r, wait_count);
	int res = 0;

	if (wait_count > 0) {
		__ASSERT(!k_is_in_isr(),
			 "expected rtio submit with wait count to be called from a thread");

		k_sem_reset(r->submit_sem);
		r->submit_count = wait_count;
	}

	rtio_executor_submit(r);

	if (wait_count > 0) {
		res = k_sem_take(r->submit_sem, K_FOREVER);
		__ASSERT(res == 0,
			 "semaphore was reset or timed out while waiting on completions!");
	}

	SYS_PORT_TRACING_FUNC_EXIT(rtio, submit, r);
	return res;
}
#else
static inline int z_impl_rtio_submit(struct rtio *r, uint32_t wait_count)
{

	SYS_PORT_TRACING_FUNC_ENTER(rtio, submit, r, wait_count);
	int res = 0;
	uintptr_t cq_count = (uintptr_t)atomic_get(&r->cq_count);
	uintptr_t cq_complete_count = cq_count + wait_count;
	bool wraps = cq_complete_count < cq_count;

	rtio_executor_submit(r);

	if (wraps) {
		while ((uintptr_t)atomic_get(&r->cq_count) >= cq_count) {
			Z_SPIN_DELAY(10);
			k_yield();
		}
	}

	while ((uintptr_t)atomic_get(&r->cq_count) < cq_complete_count) {
		Z_SPIN_DELAY(10);
		k_yield();
	}

	SYS_PORT_TRACING_FUNC_EXIT(rtio, submit, r);
	return res;
}
#endif /* CONFIG_RTIO_SUBMIT_SEM */

/**
 * @brief Pool of RTIO contexts to use with dynamically created threads
 */
struct rtio_pool {
	/** Size of the pool */
	size_t pool_size;

	/** Array containing contexts of the pool */
	struct rtio **contexts;

	/** Atomic bitmap to signal a member is used/unused */
	atomic_t *used;
};

/**
 * @brief Obtain an RTIO context from a pool
 *
 * @param pool RTIO pool to acquire a context from
 *
 * @retval NULL no available contexts
 * @retval r Valid context with permissions granted to the calling thread
 */
__syscall struct rtio *rtio_pool_acquire(struct rtio_pool *pool);

static inline struct rtio *z_impl_rtio_pool_acquire(struct rtio_pool *pool)
{
	struct rtio *r = NULL;

	for (size_t i = 0; i < pool->pool_size; i++) {
		if (atomic_test_and_set_bit(pool->used, i) == 0) {
			r = pool->contexts[i];
			break;
		}
	}

	if (r != NULL) {
		rtio_access_grant(r, k_current_get());
	}

	return r;
}

/**
 * @brief Return an RTIO context to a pool
 *
 * @param pool RTIO pool to return a context to
 * @param r RTIO context to return to the pool
 */
__syscall void rtio_pool_release(struct rtio_pool *pool, struct rtio *r);

static inline void z_impl_rtio_pool_release(struct rtio_pool *pool, struct rtio *r)
{

	if (k_is_user_context()) {
		rtio_access_revoke(r, k_current_get());
	}

	for (size_t i = 0; i < pool->pool_size; i++) {
		if (pool->contexts[i] == r) {
			atomic_clear_bit(pool->used, i);
			break;
		}
	}
}

/* clang-format off */

/** @cond ignore */

#define Z_RTIO_POOL_NAME_N(n, name)                                             \
	name##_##n

#define Z_RTIO_POOL_DEFINE_N(n, name, sq_sz, cq_sz)				\
	RTIO_DEFINE(Z_RTIO_POOL_NAME_N(n, name), sq_sz, cq_sz)

#define Z_RTIO_POOL_REF_N(n, name)                                              \
	&Z_RTIO_POOL_NAME_N(n, name)

/** @endcond */

/**
 * @brief Statically define and initialize a pool of RTIO contexts
 *
 * @param name Name of the RTIO pool
 * @param pool_sz Number of RTIO contexts to allocate in the pool
 * @param sq_sz Size of the submission queue entry pool per context
 * @param cq_sz Size of the completion queue entry pool per context
 */
#define RTIO_POOL_DEFINE(name, pool_sz, sq_sz, cq_sz)				\
	LISTIFY(pool_sz, Z_RTIO_POOL_DEFINE_N, (;), name, sq_sz, cq_sz);        \
	static struct rtio *name##_contexts[] = {                               \
		LISTIFY(pool_sz, Z_RTIO_POOL_REF_N, (,), name)                  \
	};                                                                      \
	ATOMIC_DEFINE(name##_used, pool_sz);                                    \
	STRUCT_SECTION_ITERABLE(rtio_pool, name) = {                            \
		.pool_size = pool_sz,                                           \
		.contexts = name##_contexts,                                    \
		.used = name##_used,                                            \
	}

/* clang-format on */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/rtio.h>

#endif /* ZEPHYR_INCLUDE_RTIO_RTIO_H_ */
