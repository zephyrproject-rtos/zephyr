/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Inline syscall implementation for RTIO APIs
 */

#ifndef ZEPHYR_INCLUDE_RTIO_RTIO_H_
#error "Should only be included by zephyr/drivers/rtio/rtio.h"
#endif

#ifndef ZEPHYR_INCLUDE_RTIO_INTERNAL_RTIO_IMPL_H_
#define ZEPHYR_INCLUDE_RTIO_INTERNAL_RTIO_IMPL_H_

#include <string.h>

#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/mem_blocks.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/mpsc_lockfree.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_rtio_cqe_get_mempool_buffer(const struct rtio *r, struct rtio_cqe *cqe,
						     uint8_t **buff, uint32_t *buff_len)
{
#ifdef CONFIG_RTIO_SYS_MEM_BLOCKS
	if (RTIO_CQE_FLAG_GET(cqe->flags) == RTIO_CQE_FLAG_MEMPOOL_BUFFER) {
		int blk_idx = RTIO_CQE_FLAG_MEMPOOL_GET_BLK_IDX(cqe->flags);
		int blk_count = RTIO_CQE_FLAG_MEMPOOL_GET_BLK_CNT(cqe->flags);
		uint32_t blk_size = rtio_mempool_block_size(r);

		*buff = r->block_pool->buffer + blk_idx * blk_size;
		*buff_len = blk_count * blk_size;
		__ASSERT_NO_MSG(*buff >= r->block_pool->buffer);
		__ASSERT_NO_MSG(*buff <
				r->block_pool->buffer + blk_size * r->block_pool->info.num_blocks);
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

static inline int z_impl_rtio_sqe_cancel(struct rtio_sqe *sqe)
{
	struct rtio_iodev_sqe *iodev_sqe = CONTAINER_OF(sqe, struct rtio_iodev_sqe, sqe);

	do {
		iodev_sqe->sqe.flags |= RTIO_SQE_CANCELED;
		iodev_sqe = rtio_iodev_sqe_next(iodev_sqe);
	} while (iodev_sqe != NULL);

	return 0;
}

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
#ifdef CONFIG_BOARD_NATIVE_POSIX
			/* Native posix fakes the clock and only moves it forward when sleeping. */
			k_sleep(K_TICKS(1));
#else
			Z_SPIN_DELAY(1);
#endif
			continue;
		}
		cqes[copied++] = *cqe;
		rtio_cqe_release(r, cqe);
	} while (copied < cqe_count && !sys_timepoint_expired(end));

	return copied;
}

static inline int z_impl_rtio_submit(struct rtio *r, uint32_t wait_count)
{
	int res = 0;

#ifdef CONFIG_RTIO_SUBMIT_SEM
	/* TODO undefined behavior if another thread calls submit of course
	 */
	if (wait_count > 0) {
		__ASSERT(!k_is_in_isr(),
			 "expected rtio submit with wait count to be called from a thread");

		k_sem_reset(r->submit_sem);
		r->submit_count = wait_count;
	}
#else
	uintptr_t cq_count = (uintptr_t)atomic_get(&r->cq_count) + wait_count;
#endif

	/* Submit the queue to the executor which consumes submissions
	 * and produces completions through ISR chains or other means.
	 */
	rtio_executor_submit(r);


	/* TODO could be nicer if we could suspend the thread and not
	 * wake up on each completion here.
	 */
#ifdef CONFIG_RTIO_SUBMIT_SEM

	if (wait_count > 0) {
		res = k_sem_take(r->submit_sem, K_FOREVER);
		__ASSERT(res == 0,
			 "semaphore was reset or timed out while waiting on completions!");
	}
#else
	while ((uintptr_t)atomic_get(&r->cq_count) < cq_count) {
		Z_SPIN_DELAY(10);
		k_yield();
	}
#endif

	return res;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RTIO_INTERNAL_RTIO_IMPL_H_ */
