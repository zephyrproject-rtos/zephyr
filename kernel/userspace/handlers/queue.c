/*
 * Copyright (c) 2010-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * System call verifiers for kernel/queue.c, compiled only with CONFIG_USERSPACE.
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>

static inline void z_vrfy_k_queue_init(struct k_queue *queue)
{
	K_OOPS(K_SYSCALL_OBJ_NEVER_INIT(queue, K_OBJ_QUEUE));
	z_impl_k_queue_init(queue);
}
#include <zephyr/syscalls/k_queue_init_mrsh.c>

static inline void z_vrfy_k_queue_cancel_wait(struct k_queue *queue)
{
	K_OOPS(K_SYSCALL_OBJ(queue, K_OBJ_QUEUE));
	z_impl_k_queue_cancel_wait(queue);
}
#include <zephyr/syscalls/k_queue_cancel_wait_mrsh.c>

static inline int32_t z_vrfy_k_queue_alloc_append(struct k_queue *queue,
						  void *data)
{
	K_OOPS(K_SYSCALL_OBJ(queue, K_OBJ_QUEUE));
	return z_impl_k_queue_alloc_append(queue, data);
}
#include <zephyr/syscalls/k_queue_alloc_append_mrsh.c>

static inline int32_t z_vrfy_k_queue_alloc_prepend(struct k_queue *queue,
						   void *data)
{
	K_OOPS(K_SYSCALL_OBJ(queue, K_OBJ_QUEUE));
	return z_impl_k_queue_alloc_prepend(queue, data);
}
#include <zephyr/syscalls/k_queue_alloc_prepend_mrsh.c>

static inline void *z_vrfy_k_queue_get(struct k_queue *queue,
				       k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_OBJ(queue, K_OBJ_QUEUE));
	return z_impl_k_queue_get(queue, timeout);
}
#include <zephyr/syscalls/k_queue_get_mrsh.c>

static inline int z_vrfy_k_queue_is_empty(struct k_queue *queue)
{
	K_OOPS(K_SYSCALL_OBJ(queue, K_OBJ_QUEUE));
	return z_impl_k_queue_is_empty(queue);
}
#include <zephyr/syscalls/k_queue_is_empty_mrsh.c>

static inline void *z_vrfy_k_queue_peek_head(struct k_queue *queue)
{
	K_OOPS(K_SYSCALL_OBJ(queue, K_OBJ_QUEUE));
	return z_impl_k_queue_peek_head(queue);
}
#include <zephyr/syscalls/k_queue_peek_head_mrsh.c>

static inline void *z_vrfy_k_queue_peek_tail(struct k_queue *queue)
{
	K_OOPS(K_SYSCALL_OBJ(queue, K_OBJ_QUEUE));
	return z_impl_k_queue_peek_tail(queue);
}
#include <zephyr/syscalls/k_queue_peek_tail_mrsh.c>
