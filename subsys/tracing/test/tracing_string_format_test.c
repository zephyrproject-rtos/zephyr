/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DISABLE_SYSCALL_TRACING

#include <zephyr/kernel.h>
#include <tracing_test.h>
#include <tracing_test_syscall.h>
#include <zephyr/tracing/tracing_format.h>

void sys_trace_k_thread_switched_out(void)
{
	struct k_thread *thread;

	thread = k_current_get();
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_switched_in(void)
{
	struct k_thread *thread;

	thread = k_current_get();
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_priority_set(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_create(struct k_thread *thread, size_t stack_size,
			       int prio)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_start(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_abort(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_suspend(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_resume(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_resume_exit(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_ready(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_sched_ready(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_sched_pend(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_sched_abort(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_sched_resume(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_sched_suspend(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_sleep_enter(k_timeout_t timeout)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_k_thread_sleep_exit(k_timeout_t timeout, int ret)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_k_thread_usleep_enter(int32_t us)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_k_thread_usleep_exit(int32_t us, int ret)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_k_thread_busy_wait_enter(uint32_t usec_to_wait)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_k_thread_busy_wait_exit(uint32_t usec_to_wait)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_k_thread_abort_enter(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_abort_exit(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_yield(void)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_thread_yield(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_wakeup(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_pend(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_info(struct k_thread *thread)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_name_set(struct k_thread *thread, int ret)
{
	TRACING_STRING("%s: %p\n", __func__, thread);
}

void sys_trace_k_thread_sched_lock(void)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_port_trace_k_thread_sched_unlock(void)
{
	TRACING_STRING("%s\n", __func__);
}


void sys_trace_k_thread_join_blocking(struct k_thread *thread, k_timeout_t timeout)
{
	TRACING_STRING("%s %p, timeout: %u\n", __func__, thread, (uint32_t)timeout.ticks);
}

void sys_trace_k_thread_join_exit(struct k_thread *thread, k_timeout_t timeout, int ret)
{
	TRACING_STRING("%s %p, timeout: %u\n", __func__, thread, (uint32_t)timeout.ticks);
}

void sys_trace_isr_enter(void)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_isr_exit(void)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_isr_exit_to_scheduler(void)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_idle(void)
{
	TRACING_STRING("%s\n", __func__);
}

void sys_trace_k_condvar_broadcast_enter(struct k_condvar *condvar)
{
	TRACING_STRING("%s: %p\n", __func__, condvar);
}

void sys_trace_k_condvar_broadcast_exit(struct k_condvar *condvar, int ret)
{
	TRACING_STRING("%s: %p\n", __func__, condvar);
}

void sys_trace_k_condvar_init(struct k_condvar *condvar, int ret)
{
	TRACING_STRING("%s: %p\n", __func__, condvar);
}

void sys_trace_k_condvar_signal_enter(struct k_condvar *condvar)
{
	TRACING_STRING("%s: %p\n", __func__, condvar);
}

void sys_trace_k_condvar_signal_blocking(struct k_condvar *condvar)
{
	TRACING_STRING("%s: %p\n", __func__, condvar);
}

void sys_trace_k_condvar_signal_exit(struct k_condvar *condvar, int ret)
{
	TRACING_STRING("%s: %p\n", __func__, condvar);
}

void sys_trace_k_condvar_wait_enter(struct k_condvar *condvar, struct k_mutex *mutex,
				    k_timeout_t timeout)
{
	TRACING_STRING("%s: %p\n", __func__, condvar);
}

void sys_trace_k_condvar_wait_exit(struct k_condvar *condvar, struct k_mutex *mutex,
				   k_timeout_t timeout, int ret)
{
	TRACING_STRING("%s: %p\n", __func__, condvar);
}


void sys_trace_k_sem_init(struct k_sem *sem, int ret)
{
	TRACING_STRING("%s: %p\n", __func__, sem);
}
void sys_trace_k_sem_give_enter(struct k_sem *sem)
{
	TRACING_STRING("%s: %p\n", __func__, sem);
}

void sys_trace_k_sem_take_enter(struct k_sem *sem, k_timeout_t timeout)
{
	TRACING_STRING("%s: %p, timeout: %u\n", __func__, sem, (uint32_t)timeout.ticks);
}

void sys_trace_k_sem_take_exit(struct k_sem *sem, k_timeout_t timeout, int ret)
{
	TRACING_STRING("%s: %p, timeout: %u\n", __func__, sem, (uint32_t)timeout.ticks);
}

void sys_trace_k_sem_take_blocking(struct k_sem *sem, k_timeout_t timeout)
{
	TRACING_STRING("%s: %p, timeout: %u\n", __func__, sem, (uint32_t)timeout.ticks);
}

void sys_trace_k_sem_reset(struct k_sem *sem)
{
	TRACING_STRING("%s: %p\n", __func__, sem);
}

void sys_trace_k_mutex_init(struct k_mutex *mutex, int ret)
{
	TRACING_STRING("%s: %p, returns %d\n", __func__, mutex, ret);
}

void sys_trace_k_mutex_lock_enter(struct k_mutex *mutex, k_timeout_t timeout)
{
	TRACING_STRING("%s: %p, timeout: %u\n", __func__, mutex, (uint32_t)timeout.ticks);
}

void sys_trace_k_mutex_lock_exit(struct k_mutex *mutex, k_timeout_t timeout, int ret)
{
	TRACING_STRING("%s: %p, timeout: %u, returns: %d\n", __func__, mutex,
		       (uint32_t)timeout.ticks, ret);
}

void sys_trace_k_mutex_lock_blocking(struct k_mutex *mutex, k_timeout_t timeout)
{
	TRACING_STRING("%s: %p, timeout: %u\n", __func__, mutex, (uint32_t)timeout.ticks);
}

void sys_trace_k_mutex_unlock_enter(struct k_mutex *mutex)
{
	TRACING_STRING("%s: %p\n", __func__, mutex);
}


void sys_trace_k_mutex_unlock_exit(struct k_mutex *mutex, int ret)
{
	TRACING_STRING("%s: %p, return: %d\n", __func__, mutex, ret);
}

void sys_trace_k_thread_sched_set_priority(struct k_thread *thread, int prio)
{
	TRACING_STRING("%s: %p, priority: %d\n", __func__, thread, prio);
}

void sys_trace_k_timer_start(struct k_timer *timer, k_timeout_t duration,
			     k_timeout_t period)
{
	TRACING_STRING("%s: %p, duration: %d, period: %d\n", __func__, timer,
		(uint32_t)duration.ticks, (uint32_t)period.ticks);
}

void sys_trace_k_timer_init(struct k_timer *timer, k_timer_expiry_t expiry_fn,
			    k_timer_expiry_t stop_fn)
{
	TRACING_STRING("%s: %p\n", __func__, timer);
}

void sys_trace_k_timer_stop(struct k_timer *timer)
{
	TRACING_STRING("%s: %p\n", __func__, timer);
}
void sys_trace_k_timer_status_sync_blocking(struct k_timer *timer)
{
	TRACING_STRING("%s: %p\n", __func__, timer);
}

void sys_trace_k_timer_status_sync_exit(struct k_timer *timer, uint32_t result)
{
	TRACING_STRING("%s: %p\n", __func__, timer);
}


void sys_trace_k_heap_init(struct k_heap *h, void *mem, size_t bytes)
{
	TRACING_STRING("%s: %p\n", __func__, h);
}

void sys_trace_k_heap_aligned_alloc_enter(struct k_heap *h, size_t bytes, k_timeout_t timeout)
{
	TRACING_STRING("%s: %p\n", __func__, h);
}

void sys_trace_k_heap_alloc_enter(struct k_heap *h, size_t bytes, k_timeout_t timeout)
{
	TRACING_STRING("%s: %p\n", __func__, h);
}

void sys_trace_k_heap_free(struct k_heap *h, void *mem)
{
	TRACING_STRING("%s: %p\n", __func__, h);
}

void sys_trace_k_heap_aligned_alloc_blocking(struct k_heap *h, size_t bytes, k_timeout_t timeout)
{
	TRACING_STRING("%s: %p\n", __func__, h);
}

void sys_trace_k_heap_alloc_exit(struct k_heap *h, size_t bytes, k_timeout_t timeout, void *ret)
{
	TRACING_STRING("%s: %p\n", __func__, h);
}

void sys_trace_k_heap_aligned_alloc_exit(struct k_heap *h, size_t bytes,
					 k_timeout_t timeout, void *ret)
{
	TRACING_STRING("%s: %p\n", __func__, h);
}

void sys_trace_k_heap_sys_k_free_enter(struct k_heap *h, struct k_heap **hr)
{
	TRACING_STRING("%s: %p\n", __func__, h);
}

void sys_trace_k_heap_sys_k_free_exit(struct k_heap *h, struct k_heap **hr)
{
	TRACING_STRING("%s: %p\n", __func__, h);
}

void sys_trace_k_queue_init(struct k_queue *queue)
{
	TRACING_STRING("%s: %p\n", __func__, queue);
}

void sys_trace_k_queue_cancel_wait(struct k_queue *queue)
{
	TRACING_STRING("%s: %p\n", __func__, queue);
}

void sys_trace_k_queue_append_enter(struct k_queue *queue, void *data)
{
	TRACING_STRING("%s: %p\n", __func__, queue);
}

void sys_trace_k_queue_append_exit(struct k_queue *queue, void *data)
{
	TRACING_STRING("%s: %p\n", __func__, queue);
}

void sys_trace_k_queue_queue_insert_enter(struct k_queue *queue, bool alloc, void *data)
{
	TRACING_STRING("%s: %p\n", __func__, queue);
}

void sys_trace_k_queue_queue_insert_exit(struct k_queue *queue, bool alloc, void *data, int ret)
{
	TRACING_STRING("%s: %p\n", __func__, queue);
}

void sys_trace_k_queue_get_blocking(struct k_queue *queue, k_timeout_t timeout)
{
	TRACING_STRING("%s: %p\n", __func__, queue);
}

void sys_trace_k_queue_get_exit(struct k_queue *queue, k_timeout_t timeout, void *ret)
{
	TRACING_STRING("%s: %p\n", __func__, queue);
}

void sys_trace_k_queue_peek_head(struct k_queue *queue, void *ret)
{
	TRACING_STRING("%s: %p\n", __func__, queue);
}

void sys_trace_k_queue_peek_tail(struct k_queue *queue, void *ret)
{
	TRACING_STRING("%s: %p\n", __func__, queue);
}

void sys_trace_k_queue_alloc_append_enter(struct k_queue *queue, void *data)
{
	TRACING_STRING("%s: %p\n", __func__, queue);
}

void sys_trace_k_queue_alloc_append_exit(struct k_queue *queue, void *data, int ret)
{
	TRACING_STRING("%s: %p\n", __func__, queue);
}

void sys_trace_k_queue_alloc_prepend_enter(struct k_queue *queue, void *data)
{
	TRACING_STRING("%s: %p\n", __func__, queue);
}

void sys_trace_k_queue_alloc_prepend_exit(struct k_queue *queue, void *data, int ret)
{
	TRACING_STRING("%s: %p\n", __func__, queue);
}


void sys_trace_k_mem_slab_alloc_enter(struct k_mem_slab *slab, void **mem, k_timeout_t timeout)
{
	TRACING_STRING("%s: %p\n", __func__, slab);
}

void sys_trace_k_mem_slab_alloc_blocking(struct k_mem_slab *slab, void **mem, k_timeout_t timeout)
{
	TRACING_STRING("%s: %p\n", __func__, slab);
}

void sys_trace_k_mem_slab_alloc_exit(struct k_mem_slab *slab, void **mem, k_timeout_t timeout,
				     int ret)
{
	TRACING_STRING("%s: %p\n", __func__, slab);
}

void sys_trace_k_mem_slab_free_enter(struct k_mem_slab *slab, void *mem)
{
	TRACING_STRING("%s: %p\n", __func__, slab);
}

void sys_trace_k_mem_slab_free_exit(struct k_mem_slab *slab, void *mem)
{
	TRACING_STRING("%s: %p\n", __func__, slab);
}

void sys_trace_k_fifo_put_enter(struct k_fifo *fifo, void *data)
{
	TRACING_STRING("%s: %p\n", __func__, fifo);
}

void sys_trace_k_fifo_put_exit(struct k_fifo *fifo, void *data)
{
	TRACING_STRING("%s: %p\n", __func__, fifo);
}

void sys_trace_k_fifo_get_enter(struct k_fifo *fifo, k_timeout_t timeout)
{
	TRACING_STRING("%s: %p\n", __func__, fifo);
}

void sys_trace_k_fifo_get_exit(struct k_fifo *fifo, k_timeout_t timeout, void *ret)
{
	TRACING_STRING("%s: %p\n", __func__, fifo);
}

void sys_trace_syscall_enter(uint32_t syscall_id, const char *syscall_name)
{
	TRACING_STRING("%s: %s (%u) enter\n", __func__, syscall_name, syscall_id);
}

void sys_trace_syscall_exit(uint32_t syscall_id, const char *syscall_name)
{
	TRACING_STRING("%s: %s (%u) exit\n", __func__, syscall_name, syscall_id);
}

void sys_trace_k_thread_foreach_unlocked_enter(k_thread_user_cb_t user_cb, void *data)
{
	TRACING_STRING("%s: %p (%p) enter\n", __func__, user_cb, data);
}

void sys_trace_k_thread_foreach_unlocked_exit(k_thread_user_cb_t user_cb, void *data)
{
	TRACING_STRING("%s: %p (%p) exit\n", __func__, user_cb, data);
}
