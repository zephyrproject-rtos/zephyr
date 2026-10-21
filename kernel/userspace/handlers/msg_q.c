/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * System call verifiers for kernel/msg_q.c, compiled only with CONFIG_USERSPACE.
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>

int z_vrfy_k_msgq_alloc_init(struct k_msgq *msgq, size_t msg_size,
			    uint32_t max_msgs)
{
	K_OOPS(K_SYSCALL_OBJ_NEVER_INIT(msgq, K_OBJ_MSGQ));

	return z_impl_k_msgq_alloc_init(msgq, msg_size, max_msgs);
}
#include <zephyr/syscalls/k_msgq_alloc_init_mrsh.c>

static inline int z_vrfy_k_msgq_put(struct k_msgq *msgq, const void *data,
				    k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));
	K_OOPS(K_SYSCALL_MEMORY_READ(data, msgq->msg_size));

	return z_impl_k_msgq_put(msgq, data, timeout);
}
#include <zephyr/syscalls/k_msgq_put_mrsh.c>

static inline int z_vrfy_k_msgq_put_front(struct k_msgq *msgq, const void *data)
{
	K_OOPS(K_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));
	K_OOPS(K_SYSCALL_MEMORY_READ(data, msgq->msg_size));

	return z_impl_k_msgq_put_front(msgq, data);
}
#include <zephyr/syscalls/k_msgq_put_front_mrsh.c>

static inline void z_vrfy_k_msgq_get_attrs(struct k_msgq *msgq,
					   struct k_msgq_attrs *attrs)
{
	K_OOPS(K_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(attrs, sizeof(struct k_msgq_attrs)));
	z_impl_k_msgq_get_attrs(msgq, attrs);
}
#include <zephyr/syscalls/k_msgq_get_attrs_mrsh.c>

static inline int z_vrfy_k_msgq_get(struct k_msgq *msgq, void *data,
				    k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(data, msgq->msg_size));

	return z_impl_k_msgq_get(msgq, data, timeout);
}
#include <zephyr/syscalls/k_msgq_get_mrsh.c>

static inline int z_vrfy_k_msgq_peek(struct k_msgq *msgq, void *data)
{
	K_OOPS(K_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(data, msgq->msg_size));

	return z_impl_k_msgq_peek(msgq, data);
}
#include <zephyr/syscalls/k_msgq_peek_mrsh.c>

static inline int z_vrfy_k_msgq_peek_at(struct k_msgq *msgq, void *data, uint32_t idx)
{
	K_OOPS(K_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(data, msgq->msg_size));

	return z_impl_k_msgq_peek_at(msgq, data, idx);
}
#include <zephyr/syscalls/k_msgq_peek_at_mrsh.c>

static inline void z_vrfy_k_msgq_purge(struct k_msgq *msgq)
{
	K_OOPS(K_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));
	z_impl_k_msgq_purge(msgq);
}
#include <zephyr/syscalls/k_msgq_purge_mrsh.c>

static inline uint32_t z_vrfy_k_msgq_num_free_get(struct k_msgq *msgq)
{
	K_OOPS(K_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));
	return z_impl_k_msgq_num_free_get(msgq);
}
#include <zephyr/syscalls/k_msgq_num_free_get_mrsh.c>

static inline uint32_t z_vrfy_k_msgq_num_used_get(struct k_msgq *msgq)
{
	K_OOPS(K_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));
	return z_impl_k_msgq_num_used_get(msgq);
}
#include <zephyr/syscalls/k_msgq_num_used_get_mrsh.c>
