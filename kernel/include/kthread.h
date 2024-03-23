/*
 * Copyright (c) 2016-2017 Wind River Systems, Inc.
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_THREAD_H_
#define ZEPHYR_KERNEL_INCLUDE_THREAD_H_

#include <zephyr/kernel.h>
#include <timeout_q.h>

#ifdef CONFIG_THREAD_MONITOR
/* This lock protects the linked list of active threads; i.e. the
 * initial _kernel.threads pointer and the linked list made up of
 * thread->next_thread (until NULL)
 */
extern struct k_spinlock z_thread_monitor_lock;
#endif /* CONFIG_THREAD_MONITOR */

/* clean up when a thread is aborted */

#if defined(CONFIG_THREAD_MONITOR)
void z_thread_monitor_exit(struct k_thread *thread);
#else
#define z_thread_monitor_exit(thread) \
	do {/* nothing */    \
	} while (false)
#endif /* CONFIG_THREAD_MONITOR */


#ifdef CONFIG_MULTITHREADING
static inline void thread_schedule_new(struct k_thread *thread, k_timeout_t delay)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS
	if (K_TIMEOUT_EQ(delay, K_NO_WAIT)) {
		k_thread_start(thread);
	} else {
		z_add_thread_timeout(thread, delay);
	}
#else
	ARG_UNUSED(delay);
	k_thread_start(thread);
#endif /* CONFIG_SYS_CLOCK_EXISTS */
}
#endif /* CONFIG_MULTITHREADING */

#endif /* ZEPHYR_KERNEL_INCLUDE_THREAD_H_ */
