/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdbool.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/internal/syscall_handler.h>

/**
 * Verify each SQE type operation and its fields ensuring
 * the iodev is a valid accessible k_object (if given) and
 * the buffer pointers are valid accessible memory by the calling
 * thread.
 *
 * Each op code that is acceptable from user mode must also be validated.
 */
static inline bool rtio_vrfy_sqe(struct rtio_sqe *sqe)
{
	if (sqe->iodev != NULL) {
		if (K_SYSCALL_OBJ(sqe->iodev, K_OBJ_RTIO_IODEV)) {
			return false;
		}
	}

	bool valid_sqe = true;

	switch (sqe->op) {
	case RTIO_OP_NOP:
		break;
	case RTIO_OP_TX:
		valid_sqe &= K_SYSCALL_MEMORY(sqe->tx.buf, sqe->tx.buf_len, false);
		break;
	case RTIO_OP_RX:
		if ((sqe->flags & RTIO_SQE_MEMPOOL_BUFFER) == 0) {
			valid_sqe &= K_SYSCALL_MEMORY(sqe->rx.buf, sqe->rx.buf_len, true);
		}
		break;
	case RTIO_OP_TINY_TX:
		break;
	case RTIO_OP_TXRX:
		valid_sqe &= K_SYSCALL_MEMORY(sqe->txrx.tx_buf, sqe->txrx.buf_len, true);
		valid_sqe &= K_SYSCALL_MEMORY(sqe->txrx.rx_buf, sqe->txrx.buf_len, true);
		break;
	default:
		/* RTIO OP must be known and allowable from user mode
		 * otherwise it is invalid
		 */
		valid_sqe = false;
	}

	return valid_sqe;
}

static inline void z_vrfy_rtio_release_buffer(struct rtio *r, void *buff, uint32_t buff_len)
{
	K_OOPS(K_SYSCALL_OBJ(r, K_OBJ_RTIO));
	z_impl_rtio_release_buffer(r, buff, buff_len);
}
#include <zephyr/syscalls/rtio_release_buffer_mrsh.c>

static inline int z_vrfy_rtio_cqe_get_mempool_buffer(const struct rtio *r, struct rtio_cqe *cqe,
						     uint8_t **buff, uint32_t *buff_len)
{
	K_OOPS(K_SYSCALL_OBJ(r, K_OBJ_RTIO));
	K_OOPS(K_SYSCALL_MEMORY_READ(cqe, sizeof(struct rtio_cqe)));
	K_OOPS(K_SYSCALL_MEMORY_READ(buff, sizeof(void *)));
	K_OOPS(K_SYSCALL_MEMORY_READ(buff_len, sizeof(uint32_t)));
	return z_impl_rtio_cqe_get_mempool_buffer(r, cqe, buff, buff_len);
}
#include <zephyr/syscalls/rtio_cqe_get_mempool_buffer_mrsh.c>

static inline int z_vrfy_rtio_sqe_cancel(struct rtio_sqe *sqe)
{
	return z_impl_rtio_sqe_cancel(sqe);
}
#include <zephyr/syscalls/rtio_sqe_cancel_mrsh.c>

static inline int z_vrfy_rtio_sqe_copy_in_get_handles(struct rtio *r, const struct rtio_sqe *sqes,
						      struct rtio_sqe **handle, size_t sqe_count)
{
	K_OOPS(K_SYSCALL_OBJ(r, K_OBJ_RTIO));

	K_OOPS(K_SYSCALL_MEMORY_ARRAY_READ(sqes, sqe_count, sizeof(struct rtio_sqe)));
	struct rtio_sqe *sqe;
	uint32_t acquirable = rtio_sqe_acquirable(r);

	if (acquirable < sqe_count) {
		return -ENOMEM;
	}


	for (int i = 0; i < sqe_count; i++) {
		sqe = rtio_sqe_acquire(r);
		__ASSERT_NO_MSG(sqe != NULL);
		if (handle != NULL && i == 0) {
			*handle = sqe;
		}
		*sqe = sqes[i];

		if (!rtio_vrfy_sqe(sqe)) {
			rtio_sqe_drop_all(r);
			K_OOPS(true);
		}
	}

	/* Already copied *and* verified, no need to redo */
	return z_impl_rtio_sqe_copy_in_get_handles(r, NULL, NULL, 0);
}
#include <zephyr/syscalls/rtio_sqe_copy_in_get_handles_mrsh.c>

static inline int z_vrfy_rtio_cqe_copy_out(struct rtio *r,
					   struct rtio_cqe *cqes,
					   size_t cqe_count,
					   k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_OBJ(r, K_OBJ_RTIO));

	K_OOPS(K_SYSCALL_MEMORY_ARRAY_WRITE(cqes, cqe_count, sizeof(struct rtio_cqe)));

	return z_impl_rtio_cqe_copy_out(r, cqes, cqe_count, timeout);
}
#include <zephyr/syscalls/rtio_cqe_copy_out_mrsh.c>

static inline int z_vrfy_rtio_submit(struct rtio *r, uint32_t wait_count)
{
	K_OOPS(K_SYSCALL_OBJ(r, K_OBJ_RTIO));

#ifdef CONFIG_RTIO_SUBMIT_SEM
	K_OOPS(K_SYSCALL_OBJ(r->submit_sem, K_OBJ_SEM));
#endif

	return z_impl_rtio_submit(r, wait_count);
}
#include <zephyr/syscalls/rtio_submit_mrsh.c>


static inline struct rtio *z_vrfy_rtio_pool_acquire(struct rtio_pool *rpool)
{
	K_OOPS(K_SYSCALL_OBJ(rpool, K_OBJ_RTIO_POOL));

	return z_impl_rtio_pool_acquire(rpool);
}
#include <zephyr/syscalls/rtio_pool_acquire_mrsh.c>


static inline void z_vrfy_rtio_pool_release(struct rtio_pool *rpool, struct rtio *r)
{
	K_OOPS(K_SYSCALL_OBJ(rpool, K_OBJ_RTIO_POOL));
	K_OOPS(K_SYSCALL_OBJ(r, K_OBJ_RTIO));

	z_impl_rtio_pool_release(rpool, r);
}
#include <zephyr/syscalls/rtio_pool_release_mrsh.c>
