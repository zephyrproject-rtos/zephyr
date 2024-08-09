/*
 * Copyright (c) 2016, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for kernel APIs.
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_H_
#error "Should only be included by zephyr/kernel.h"
#endif

#ifndef ZEPHYR_INCLUDE_KERNEL_INTERNAL_KERNEL_IMPL_H_
#define ZEPHYR_INCLUDE_KERNEL_INTERNAL_KERNEL_IMPL_H_

#if !defined(_ASMLANGUAGE)
#include <zephyr/kernel_includes.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <zephyr/toolchain.h>
#include <zephyr/tracing/tracing_macros.h>
#include <zephyr/sys/mem_stats.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SYS_CLOCK_EXISTS

static inline k_ticks_t z_impl_k_thread_timeout_expires_ticks(
						const struct k_thread *thread)
{
	return z_timeout_expires(&thread->base.timeout);
}

static inline k_ticks_t z_impl_k_thread_timeout_remaining_ticks(
						const struct k_thread *thread)
{
	return z_timeout_remaining(&thread->base.timeout);
}

static inline k_ticks_t z_impl_k_timer_expires_ticks(
				       const struct k_timer *timer)
{
	return z_timeout_expires(&timer->timeout);
}

static inline k_ticks_t z_impl_k_timer_remaining_ticks(
				       const struct k_timer *timer)
{
	return z_timeout_remaining(&timer->timeout);
}

#endif /* CONFIG_SYS_CLOCK_EXISTS */

static inline void z_impl_k_timer_user_data_set(struct k_timer *timer,
					       void *user_data)
{
	timer->user_data = user_data;
}

static inline void *z_impl_k_timer_user_data_get(const struct k_timer *timer)
{
	return timer->user_data;
}

static inline int z_impl_k_queue_is_empty(struct k_queue *queue)
{
	return sys_sflist_is_empty(&queue->data_q) ? 1 : 0;
}

static inline unsigned int z_impl_k_sem_count_get(struct k_sem *sem)
{
	return sem->count;
}

static inline uint32_t z_impl_k_msgq_num_free_get(struct k_msgq *msgq)
{
	return msgq->max_msgs - msgq->used_msgs;
}

static inline uint32_t z_impl_k_msgq_num_used_get(struct k_msgq *msgq)
{
	return msgq->used_msgs;
}

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_KERNEL_INTERNAL_KERNEL_IMPL_H_ */
