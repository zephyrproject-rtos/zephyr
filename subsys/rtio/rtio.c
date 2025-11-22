/**
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/rtio/rtio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/mpsc_lockfree.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/app_memory/app_memdomain.h>

#define __RTIO_MEMPOOL_GET_NUM_BLKS(num_bytes, blk_size)                                           \
	(((num_bytes) + (blk_size) - 1) / (blk_size))

void z_impl_rtio_pool_release(struct rtio_pool *pool, struct rtio *r)
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

struct rtio *z_impl_rtio_pool_acquire(struct rtio_pool *pool)
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

#ifdef CONFIG_RTIO_SUBMIT_SEM
int z_impl_rtio_submit(struct rtio *r, uint32_t wait_count)
{
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

	return res;
}
#else
int z_impl_rtio_submit(struct rtio *r, uint32_t wait_count)
{

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

	return res;
}
#endif /* CONFIG_RTIO_SUBMIT_SEM */

int z_impl_rtio_cqe_copy_out(struct rtio *r, struct rtio_cqe *cqes, size_t cqe_count,
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

int z_impl_rtio_sqe_copy_in_get_handles(struct rtio *r, const struct rtio_sqe *sqes,
					struct rtio_sqe **handle, size_t sqe_count)
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

void z_impl_rtio_sqe_signal(struct rtio_sqe *sqe)
{
	struct rtio_iodev_sqe *iodev_sqe = CONTAINER_OF(sqe, struct rtio_iodev_sqe, sqe);

	if (!atomic_cas(&iodev_sqe->sqe.await.ok, 0, 1)) {
		iodev_sqe->sqe.await.callback(iodev_sqe, iodev_sqe->sqe.await.userdata);
	}
}

int z_impl_rtio_sqe_cancel(struct rtio_sqe *sqe)
{
	struct rtio_iodev_sqe *iodev_sqe = CONTAINER_OF(sqe, struct rtio_iodev_sqe, sqe);

	do {
		iodev_sqe->sqe.flags |= RTIO_SQE_CANCELED;
		iodev_sqe = rtio_iodev_sqe_next(iodev_sqe);
	} while (iodev_sqe != NULL);

	return 0;
}

void z_impl_rtio_release_buffer(struct rtio *r, void *buff, uint32_t buff_len)
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

int rtio_sqe_rx_buf(const struct rtio_iodev_sqe *iodev_sqe, uint32_t min_buf_len,
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

void rtio_cqe_submit(struct rtio *r, int result, void *userdata, uint32_t flags)
{
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
}

int z_impl_rtio_cqe_get_mempool_buffer(const struct rtio *r, struct rtio_cqe *cqe, uint8_t **buff,
				       uint32_t *buff_len)
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
			__ASSERT_NO_MSG(*buff < r->block_pool->buffer +
							blk_size * r->block_pool->info.num_blocks);
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

uint32_t rtio_cqe_compute_flags(struct rtio_iodev_sqe *iodev_sqe)
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

int rtio_flush_completion_queue(struct rtio *r)
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

struct rtio_cqe *rtio_cqe_consume_block(struct rtio *r)
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

struct rtio_cqe *rtio_cqe_consume(struct rtio *r)
{
	struct mpsc_node *node;
	struct rtio_cqe *cqe = NULL;

#ifdef CONFIG_RTIO_CONSUME_SEM
	if (k_sem_take(r->consume_sem, K_NO_WAIT) != 0) {
		return NULL;
	}
#endif

	node = mpsc_pop(&r->cq);
	if (node == NULL) {
		return NULL;
	}
	cqe = CONTAINER_OF(node, struct rtio_cqe, q);

	return cqe;
}

struct rtio_cqe *rtio_cqe_acquire(struct rtio *r)
{
	struct rtio_cqe *cqe = rtio_cqe_pool_alloc(r->cqe_pool);

	if (cqe == NULL) {
		return NULL;
	}

	memset(cqe, 0, sizeof(struct rtio_cqe));

	return cqe;
}

struct rtio_sqe *rtio_sqe_acquire(struct rtio *r)
{
	struct rtio_iodev_sqe *iodev_sqe = rtio_sqe_pool_alloc(r->sqe_pool);

	if (iodev_sqe == NULL) {
		return NULL;
	}

	mpsc_push(&r->sq, &iodev_sqe->q);

	return &iodev_sqe->sqe;
}

void rtio_sqe_drop_all(struct rtio *r)
{
	struct rtio_iodev_sqe *iodev_sqe;
	struct mpsc_node *node = mpsc_pop(&r->sq);

	while (node != NULL) {
		iodev_sqe = CONTAINER_OF(node, struct rtio_iodev_sqe, q);
		rtio_sqe_pool_free(r->sqe_pool, iodev_sqe);
		node = mpsc_pop(&r->sq);
	}
}

void rtio_block_pool_free(struct rtio *r, void *buf, uint32_t buf_len)
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

int rtio_block_pool_alloc(struct rtio *r, size_t min_sz, size_t max_sz, uint8_t **buf,
			  uint32_t *buf_len)
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

struct rtio_iodev_sqe *rtio_sqe_pool_alloc(struct rtio_sqe_pool *pool)
{
	struct mpsc_node *node = mpsc_pop(&pool->free_q);

	if (node == NULL) {
		return NULL;
	}

	struct rtio_iodev_sqe *iodev_sqe = CONTAINER_OF(node, struct rtio_iodev_sqe, q);

	pool->pool_free--;

	return iodev_sqe;
}

void rtio_sqe_pool_free(struct rtio_sqe_pool *pool, struct rtio_iodev_sqe *iodev_sqe)
{
	mpsc_push(&pool->free_q, &iodev_sqe->q);

	pool->pool_free++;
}

struct rtio_cqe *rtio_cqe_pool_alloc(struct rtio_cqe_pool *pool)
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

void rtio_cqe_pool_free(struct rtio_cqe_pool *pool, struct rtio_cqe *cqe)
{
	mpsc_push(&pool->free_q, &cqe->q);

	pool->pool_free++;
}

void rtio_iodev_sqe_await_signal(struct rtio_iodev_sqe *iodev_sqe, rtio_signaled_t callback,
				 void *userdata)
{
	iodev_sqe->sqe.await.callback = callback;
	iodev_sqe->sqe.await.userdata = userdata;

	if (!atomic_cas(&iodev_sqe->sqe.await.ok, 0, 1)) {
		callback(iodev_sqe, userdata);
	}
}

void rtio_access_grant(struct rtio *r, struct k_thread *t)
{
	k_object_access_grant(r, t);

#ifdef CONFIG_RTIO_SUBMIT_SEM
	k_object_access_grant(r->submit_sem, t);
#endif

#ifdef CONFIG_RTIO_CONSUME_SEM
	k_object_access_grant(r->consume_sem, t);
#endif
}

void rtio_access_revoke(struct rtio *r, struct k_thread *t)
{
	k_object_access_revoke(r, t);

#ifdef CONFIG_RTIO_SUBMIT_SEM
	k_object_access_revoke(r->submit_sem, t);
#endif

#ifdef CONFIG_RTIO_CONSUME_SEM
	k_object_access_revoke(r->consume_sem, t);
#endif
}
