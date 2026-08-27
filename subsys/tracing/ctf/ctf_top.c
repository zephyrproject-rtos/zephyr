/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <ctf_top.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket_poll.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/sys/cpu_load.h>
#include <zephyr/pm/state.h>

struct rtio;
struct rtio_sqe;
struct rtio_cqe;
struct rtio_iodev_sqe;

static void _get_thread_name(struct k_thread *thread, ctf_bounded_string_t *name)
{
	const char *tname = k_thread_name_get(thread);

	if (tname != NULL && tname[0] != '\0') {
		strncpy(name->buf, tname, sizeof(name->buf));
		/* strncpy may not always null-terminate */
		name->buf[sizeof(name->buf) - 1] = 0;
	}
}

void sys_trace_k_thread_switched_out(void)
{
	ctf_bounded_string_t name = {"unknown"};
	struct k_thread *thread;

	thread = k_sched_current_thread_query();
	_get_thread_name(thread, &name);

	ctf_top_thread_switched_out((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_user_mode_enter(void)
{
	struct k_thread *thread;
	ctf_bounded_string_t name = {"unknown"};

	thread = k_sched_current_thread_query();
	_get_thread_name(thread, &name);
	ctf_top_thread_user_mode_enter((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_wakeup(struct k_thread *thread)
{
	ctf_bounded_string_t name = {"unknown"};

	_get_thread_name(thread, &name);
	ctf_top_thread_wakeup((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_switched_in(void)
{
	struct k_thread *thread;
	ctf_bounded_string_t name = {"unknown"};

	thread = k_sched_current_thread_query();
	_get_thread_name(thread, &name);

	ctf_top_thread_switched_in((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_priority_set(struct k_thread *thread)
{
	ctf_bounded_string_t name = {"unknown"};

	_get_thread_name(thread, &name);
	ctf_top_thread_priority_set((uint32_t)(uintptr_t)thread, thread->base.prio, name);
}

void sys_trace_k_thread_sleep_ticks_enter(k_timeout_t timeout)
{
	ctf_top_thread_sleep_ticks_enter(k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_thread_sleep_ticks_exit(k_timeout_t timeout, int ret)
{
	ctf_top_thread_sleep_ticks_exit(k_ticks_to_us_floor32((uint32_t)timeout.ticks),
					(uint32_t)ret);
}

void sys_trace_k_thread_create(struct k_thread *thread, size_t stack_size, int prio)
{
	ctf_bounded_string_t name = {"unknown"};

	_get_thread_name(thread, &name);
	ctf_top_thread_create((uint32_t)(uintptr_t)thread, thread->base.prio, name);

#if defined(CONFIG_THREAD_STACK_INFO)
	ctf_top_thread_info((uint32_t)(uintptr_t)thread, name, thread->stack_info.start,
			    thread->stack_info.size);
#endif
}

void sys_trace_k_thread_abort(struct k_thread *thread)
{
	ctf_bounded_string_t name = {"unknown"};

	_get_thread_name(thread, &name);
	ctf_top_thread_abort((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_suspend(struct k_thread *thread)
{
	ctf_bounded_string_t name = {"unknown"};

	_get_thread_name(thread, &name);
	ctf_top_thread_suspend((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_resume(struct k_thread *thread)
{
	ctf_bounded_string_t name = {"unknown"};

	_get_thread_name(thread, &name);

	ctf_top_thread_resume((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_ready(struct k_thread *thread)
{
	ctf_bounded_string_t name = {"unknown"};

	_get_thread_name(thread, &name);

	ctf_top_thread_ready((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_start(struct k_thread *thread)
{
}

void sys_trace_k_thread_pend(struct k_thread *thread)
{
	ctf_bounded_string_t name = {"unknown"};

	_get_thread_name(thread, &name);
	ctf_top_thread_pend((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_info(struct k_thread *thread)
{
#if defined(CONFIG_THREAD_STACK_INFO)
	ctf_bounded_string_t name = {"unknown"};

	_get_thread_name(thread, &name);
	ctf_top_thread_info((uint32_t)(uintptr_t)thread, name, thread->stack_info.start,
			    thread->stack_info.size);
#endif
}

void sys_trace_k_thread_name_set(struct k_thread *thread, int ret)
{
	ctf_bounded_string_t name = {"unknown"};

	_get_thread_name(thread, &name);
	ctf_top_thread_name_set((uint32_t)(uintptr_t)thread, name);
}

/* Thread Extended Functions */
void sys_trace_k_thread_foreach_enter(void)
{
	ctf_top_thread_foreach_enter();
}

void sys_trace_k_thread_foreach_exit(void)
{
	ctf_top_thread_foreach_exit();
}

void sys_trace_k_thread_foreach_unlocked_enter(void)
{
	ctf_top_thread_foreach_unlocked_enter();
}

void sys_trace_k_thread_foreach_unlocked_exit(void)
{
	ctf_top_thread_foreach_unlocked_exit();
}

void sys_trace_k_thread_heap_assign(struct k_thread *thread, struct k_heap *heap)
{
	ctf_top_thread_heap_assign((uint32_t)(uintptr_t)thread, (uint32_t)(uintptr_t)heap);
}

void sys_trace_k_thread_join_enter(struct k_thread *thread, k_timeout_t timeout)
{
	ctf_top_thread_join_enter((uint32_t)(uintptr_t)thread, (uint32_t)timeout.ticks);
}

void sys_trace_k_thread_join_blocking(struct k_thread *thread, k_timeout_t timeout)
{
	ctf_top_thread_join_blocking((uint32_t)(uintptr_t)thread, (uint32_t)timeout.ticks);
}

void sys_trace_k_thread_join_exit(struct k_thread *thread, k_timeout_t timeout, int ret)
{
	ctf_top_thread_join_exit((uint32_t)(uintptr_t)thread, (uint32_t)timeout.ticks,
				 (int32_t)ret);
}

void sys_trace_k_thread_busy_wait_enter(uint32_t usec_to_wait)
{
	ctf_top_thread_busy_wait_enter(usec_to_wait);
}

void sys_trace_k_thread_busy_wait_exit(uint32_t usec_to_wait)
{
	ctf_top_thread_busy_wait_exit(usec_to_wait);
}

void sys_trace_k_thread_yield(void)
{
	ctf_top_thread_yield();
}

void sys_trace_k_thread_suspend_exit(struct k_thread *thread)
{
	ctf_bounded_string_t name = {"unknown"};

	_get_thread_name(thread, &name);
	ctf_top_thread_suspend_exit((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_sched_lock(void)
{
	ctf_top_thread_sched_lock();
}

void sys_trace_k_thread_sched_unlock(void)
{
	ctf_top_thread_sched_unlock();
}

void sys_trace_k_thread_sched_wakeup(struct k_thread *thread)
{
	ctf_bounded_string_t name = {"unknown"};

	_get_thread_name(thread, &name);
	ctf_top_thread_sched_wakeup((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_sched_abort(struct k_thread *thread)
{
	ctf_bounded_string_t name = {"unknown"};

	_get_thread_name(thread, &name);
	ctf_top_thread_sched_abort((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_sched_priority_set(struct k_thread *thread, int prio)
{
	ctf_bounded_string_t name = {"unknown"};

	_get_thread_name(thread, &name);
	ctf_top_thread_sched_priority_set((uint32_t)(uintptr_t)thread, (int8_t)prio, name);
}

void sys_trace_k_thread_sched_ready(struct k_thread *thread)
{
	ctf_bounded_string_t name = {"unknown"};

	_get_thread_name(thread, &name);
	ctf_top_thread_sched_ready((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_sched_pend(struct k_thread *thread)
{
	ctf_bounded_string_t name = {"unknown"};

	_get_thread_name(thread, &name);
	ctf_top_thread_sched_pend((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_sched_resume(struct k_thread *thread)
{
	ctf_bounded_string_t name = {"unknown"};

	_get_thread_name(thread, &name);
	ctf_top_thread_sched_resume((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_sched_suspend(struct k_thread *thread)
{
	ctf_bounded_string_t name = {"unknown"};

	_get_thread_name(thread, &name);
	ctf_top_thread_sched_suspend((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_isr_enter(void)
{
	if (IS_ENABLED(CONFIG_CPU_LOAD_BACKEND_IDLE_HOOK)) {
		cpu_load_on_exit_idle();
	}

	ctf_top_isr_enter();
}

void sys_trace_isr_exit(void)
{
	ctf_top_isr_exit();
}

void sys_trace_isr_exit_to_scheduler(void)
{
	ctf_top_isr_exit_to_scheduler();
}

void sys_trace_idle(void)
{
#ifdef CONFIG_TRACING_IDLE
	ctf_top_idle();
#endif
	if (IS_ENABLED(CONFIG_CPU_LOAD_BACKEND_IDLE_HOOK)) {
		cpu_load_on_enter_idle();
	}
}

void sys_trace_idle_exit(void)
{
	if (IS_ENABLED(CONFIG_CPU_LOAD_BACKEND_IDLE_HOOK)) {
		cpu_load_on_exit_idle();
	}
}

/* Memory Slabs */

void sys_trace_k_mem_slab_init(struct k_mem_slab *slab, int ret)
{
	ctf_top_mem_slab_init((uint32_t)(uintptr_t)slab, (int32_t)ret);
}

void sys_trace_k_mem_slab_alloc_enter(struct k_mem_slab *slab, k_timeout_t timeout)
{
	ctf_top_mem_slab_alloc_enter((uint32_t)(uintptr_t)slab,
				     k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_mem_slab_alloc_blocking(struct k_mem_slab *slab, k_timeout_t timeout)
{
	ctf_top_mem_slab_alloc_blocking((uint32_t)(uintptr_t)slab,
					k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_mem_slab_alloc_exit(struct k_mem_slab *slab, k_timeout_t timeout, int ret)
{
	ctf_top_mem_slab_alloc_exit((uint32_t)(uintptr_t)slab,
				    k_ticks_to_us_floor32((uint32_t)timeout.ticks), (int32_t)ret);
}

void sys_trace_k_mem_slab_free_enter(struct k_mem_slab *slab)
{
	ctf_top_mem_slab_free_enter((uint32_t)(uintptr_t)slab);
}

void sys_trace_k_mem_slab_free_exit(struct k_mem_slab *slab)
{
	ctf_top_mem_slab_free_exit((uint32_t)(uintptr_t)slab);
}

/* Message Queues */
void sys_trace_k_msgq_init(struct k_msgq *msgq)
{
	ctf_top_msgq_init((uint32_t)(uintptr_t)msgq);
}

void sys_trace_k_msgq_alloc_init_enter(struct k_msgq *msgq)
{
	ctf_top_msgq_alloc_init_enter((uint32_t)(uintptr_t)msgq);
}

void sys_trace_k_msgq_alloc_init_exit(struct k_msgq *msgq, int ret)
{
	ctf_top_msgq_alloc_init_exit((uint32_t)(uintptr_t)msgq, (int32_t)ret);
}

void sys_trace_k_msgq_put_enter(struct k_msgq *msgq, k_timeout_t timeout)
{
	ctf_top_msgq_put_enter((uint32_t)(uintptr_t)msgq,
			       k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_msgq_get_enter(struct k_msgq *msgq, k_timeout_t timeout)
{
	ctf_top_msgq_get_enter((uint32_t)(uintptr_t)msgq,
			       k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_msgq_get_blocking(struct k_msgq *msgq, k_timeout_t timeout)
{
	ctf_top_msgq_get_blocking((uint32_t)(uintptr_t)msgq,
				  k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_msgq_get_exit(struct k_msgq *msgq, k_timeout_t timeout, int ret)
{
	ctf_top_msgq_get_exit((uint32_t)(uintptr_t)msgq,
			      k_ticks_to_us_floor32((uint32_t)timeout.ticks), (int32_t)ret);
}

void sys_trace_k_msgq_put_blocking(struct k_msgq *msgq, k_timeout_t timeout)
{
	ctf_top_msgq_put_blocking((uint32_t)(uintptr_t)msgq,
				  k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_msgq_put_exit(struct k_msgq *msgq, k_timeout_t timeout, int ret)
{
	ctf_top_msgq_put_exit((uint32_t)(uintptr_t)msgq,
			      k_ticks_to_us_floor32((uint32_t)timeout.ticks), (int32_t)ret);
}

void sys_trace_k_msgq_peek(struct k_msgq *msgq, int ret)
{
	ctf_top_msgq_peek((uint32_t)(uintptr_t)msgq, (int32_t)ret);
}

void sys_trace_k_msgq_purge(struct k_msgq *msgq)
{
	ctf_top_msgq_purge((uint32_t)(uintptr_t)msgq);
}

void sys_trace_k_msgq_put_front_enter(struct k_msgq *msgq, k_timeout_t timeout)
{
	ctf_top_msgq_put_front_enter((uint32_t)(uintptr_t)msgq,
				     k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_msgq_put_front_blocking(struct k_msgq *msgq, k_timeout_t timeout)
{
	ctf_top_msgq_put_front_blocking((uint32_t)(uintptr_t)msgq,
					k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_msgq_put_front_exit(struct k_msgq *msgq, k_timeout_t timeout, int ret)
{
	ctf_top_msgq_put_front_exit((uint32_t)(uintptr_t)msgq,
				    k_ticks_to_us_floor32((uint32_t)timeout.ticks), (int32_t)ret);
}

void sys_trace_k_msgq_cleanup_enter(struct k_msgq *msgq)
{
	ctf_top_msgq_cleanup_enter((uint32_t)(uintptr_t)msgq);
}

void sys_trace_k_msgq_cleanup_exit(struct k_msgq *msgq, int ret)
{
	ctf_top_msgq_cleanup_exit((uint32_t)(uintptr_t)msgq, (int32_t)ret);
}

/* Condition Variables */
void sys_trace_k_condvar_init(struct k_condvar *condvar, int ret)
{
	ctf_top_condvar_init((uint32_t)(uintptr_t)condvar, (int32_t)ret);
}

void sys_trace_k_condvar_wait_enter(struct k_condvar *condvar, k_timeout_t timeout)
{
	ctf_top_condvar_wait_enter((uint32_t)(uintptr_t)condvar,
				   k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_condvar_wait_exit(struct k_condvar *condvar, k_timeout_t timeout, int ret)
{
	ctf_top_condvar_wait_exit((uint32_t)(uintptr_t)condvar,
				  k_ticks_to_us_floor32((uint32_t)timeout.ticks), (int32_t)ret);
}

void sys_trace_k_condvar_signal_enter(struct k_condvar *condvar)
{
	ctf_top_condvar_signal_enter((uint32_t)(uintptr_t)condvar);
}

void sys_trace_k_condvar_signal_blocking(struct k_condvar *condvar, k_timeout_t timeout)
{
	ctf_top_condvar_signal_blocking((uint32_t)(uintptr_t)condvar,
					k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_condvar_signal_exit(struct k_condvar *condvar, int ret)
{
	ctf_top_condvar_signal_exit((uint32_t)(uintptr_t)condvar, (int32_t)ret);
}
void sys_trace_k_condvar_broadcast_enter(struct k_condvar *condvar)
{
	ctf_top_condvar_broadcast_enter((uint32_t)(uintptr_t)condvar);
}
void sys_trace_k_condvar_broadcast_exit(struct k_condvar *condvar, int ret)
{
	ctf_top_condvar_broadcast_exit((uint32_t)(uintptr_t)condvar, (int32_t)ret);
}

/* Work Queue */
void sys_trace_k_work_init(struct k_work *work)
{
	ctf_top_work_init((uint32_t)(uintptr_t)work);
}

void sys_trace_k_work_submit_to_queue_enter(struct k_work_q *queue, struct k_work *work)
{
	ctf_top_work_submit_to_queue_enter((uint32_t)(uintptr_t)queue, (uint32_t)(uintptr_t)work);
}

void sys_trace_k_work_submit_to_queue_exit(struct k_work_q *queue, struct k_work *work, int ret)
{
	ctf_top_work_submit_to_queue_exit((uint32_t)(uintptr_t)queue, (uint32_t)(uintptr_t)work,
					  (int32_t)ret);
}

void sys_trace_k_work_submit_enter(struct k_work *work)
{
	ctf_top_work_submit_enter((uint32_t)(uintptr_t)work);
}

void sys_trace_k_work_submit_exit(struct k_work *work, int ret)
{
	ctf_top_work_submit_exit((uint32_t)(uintptr_t)work, (int32_t)ret);
}

void sys_trace_k_work_flush_enter(struct k_work *work)
{
	ctf_top_work_flush_enter((uint32_t)(uintptr_t)work);
}

void sys_trace_k_work_flush_blocking(struct k_work *work, k_timeout_t timeout)
{
	ctf_top_work_flush_blocking((uint32_t)(uintptr_t)work,
				    k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_work_flush_exit(struct k_work *work, int ret)
{
	ctf_top_work_flush_exit((uint32_t)(uintptr_t)work, (int32_t)ret);
}

void sys_trace_k_work_cancel_enter(struct k_work *work)
{
	ctf_top_work_cancel_enter((uint32_t)(uintptr_t)work);
}

void sys_trace_k_work_cancel_exit(struct k_work *work, int ret)
{
	ctf_top_work_cancel_exit((uint32_t)(uintptr_t)work, (int32_t)ret);
}

void sys_trace_k_work_cancel_sync_enter(struct k_work *work, struct k_work_sync *sync)
{
	ctf_top_work_cancel_sync_enter((uint32_t)(uintptr_t)work, (uint32_t)(uintptr_t)sync);
}

void sys_trace_k_work_cancel_sync_blocking(struct k_work *work, struct k_work_sync *sync)
{
	ctf_top_work_cancel_sync_blocking((uint32_t)(uintptr_t)work, (uint32_t)(uintptr_t)sync);
}

void sys_trace_k_work_cancel_sync_exit(struct k_work *work, struct k_work_sync *sync, int ret)
{
	ctf_top_work_cancel_sync_exit((uint32_t)(uintptr_t)work, (uint32_t)(uintptr_t)sync,
				      (int32_t)ret);
}

/* Work Queue Management */
void sys_trace_k_work_queue_init(struct k_work_q *queue)
{
	ctf_top_work_queue_init((uint32_t)(uintptr_t)queue);
}

void sys_trace_k_work_queue_start_enter(struct k_work_q *queue)
{
	ctf_top_work_queue_start_enter((uint32_t)(uintptr_t)queue);
}

void sys_trace_k_work_queue_start_exit(struct k_work_q *queue)
{
	ctf_top_work_queue_start_exit((uint32_t)(uintptr_t)queue);
}

void sys_trace_k_work_queue_stop_enter(struct k_work_q *queue, k_timeout_t timeout)
{
	ctf_top_work_queue_stop_enter((uint32_t)(uintptr_t)queue,
				      k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_work_queue_stop_blocking(struct k_work_q *queue, k_timeout_t timeout)
{
	ctf_top_work_queue_stop_blocking((uint32_t)(uintptr_t)queue,
					 k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_work_queue_stop_exit(struct k_work_q *queue, k_timeout_t timeout, int ret)
{
	ctf_top_work_queue_stop_exit((uint32_t)(uintptr_t)queue,
				     k_ticks_to_us_floor32((uint32_t)timeout.ticks), (int32_t)ret);
}

void sys_trace_k_work_queue_drain_enter(struct k_work_q *queue)
{
	ctf_top_work_queue_drain_enter((uint32_t)(uintptr_t)queue);
}

void sys_trace_k_work_queue_drain_exit(struct k_work_q *queue, int ret)
{
	ctf_top_work_queue_drain_exit((uint32_t)(uintptr_t)queue, (int32_t)ret);
}

void sys_trace_k_work_queue_unplug_enter(struct k_work_q *queue)
{
	ctf_top_work_queue_unplug_enter((uint32_t)(uintptr_t)queue);
}

void sys_trace_k_work_queue_unplug_exit(struct k_work_q *queue, int ret)
{
	ctf_top_work_queue_unplug_exit((uint32_t)(uintptr_t)queue, (int32_t)ret);
}

/* Delayable Work */
void sys_trace_k_work_delayable_init(struct k_work_delayable *dwork)
{
	ctf_top_work_delayable_init((uint32_t)(uintptr_t)dwork);
}

void sys_trace_k_work_schedule_for_queue_enter(struct k_work_q *queue,
					       struct k_work_delayable *dwork, k_timeout_t delay)
{
	ctf_top_work_schedule_for_queue_enter((uint32_t)(uintptr_t)queue,
					      (uint32_t)(uintptr_t)dwork,
					      k_ticks_to_us_floor32((uint32_t)delay.ticks));
}

void sys_trace_k_work_schedule_for_queue_exit(struct k_work_q *queue,
					      struct k_work_delayable *dwork, k_timeout_t delay,
					      int ret)
{
	ctf_top_work_schedule_for_queue_exit((uint32_t)(uintptr_t)queue, (uint32_t)(uintptr_t)dwork,
					     k_ticks_to_us_floor32((uint32_t)delay.ticks),
					     (int32_t)ret);
}

void sys_trace_k_work_schedule_enter(struct k_work_delayable *dwork, k_timeout_t delay)
{
	ctf_top_work_schedule_enter((uint32_t)(uintptr_t)dwork,
				    k_ticks_to_us_floor32((uint32_t)delay.ticks));
}

void sys_trace_k_work_schedule_exit(struct k_work_delayable *dwork, k_timeout_t delay, int ret)
{
	ctf_top_work_schedule_exit((uint32_t)(uintptr_t)dwork,
				   k_ticks_to_us_floor32((uint32_t)delay.ticks), (int32_t)ret);
}

void sys_trace_k_work_reschedule_for_queue_enter(struct k_work_q *queue,
						 struct k_work_delayable *dwork, k_timeout_t delay)
{
	ctf_top_work_reschedule_for_queue_enter((uint32_t)(uintptr_t)queue,
						(uint32_t)(uintptr_t)dwork,
						k_ticks_to_us_floor32((uint32_t)delay.ticks));
}

void sys_trace_k_work_reschedule_for_queue_exit(struct k_work_q *queue,
						struct k_work_delayable *dwork, k_timeout_t delay,
						int ret)
{
	ctf_top_work_reschedule_for_queue_exit(
		(uint32_t)(uintptr_t)queue, (uint32_t)(uintptr_t)dwork,
		k_ticks_to_us_floor32((uint32_t)delay.ticks), (int32_t)ret);
}

void sys_trace_k_work_reschedule_enter(struct k_work_delayable *dwork, k_timeout_t delay)
{
	ctf_top_work_reschedule_enter((uint32_t)(uintptr_t)dwork,
				      k_ticks_to_us_floor32((uint32_t)delay.ticks));
}

void sys_trace_k_work_reschedule_exit(struct k_work_delayable *dwork, k_timeout_t delay, int ret)
{
	ctf_top_work_reschedule_exit((uint32_t)(uintptr_t)dwork,
				     k_ticks_to_us_floor32((uint32_t)delay.ticks), (int32_t)ret);
}

void sys_trace_k_work_flush_delayable_enter(struct k_work_delayable *dwork,
					    struct k_work_sync *sync)
{
	ctf_top_work_flush_delayable_enter((uint32_t)(uintptr_t)dwork, (uint32_t)(uintptr_t)sync);
}

void sys_trace_k_work_flush_delayable_exit(struct k_work_delayable *dwork, struct k_work_sync *sync,
					   int ret)
{
	ctf_top_work_flush_delayable_exit((uint32_t)(uintptr_t)dwork, (uint32_t)(uintptr_t)sync,
					  (int32_t)ret);
}

void sys_trace_k_work_cancel_delayable_enter(struct k_work_delayable *dwork)
{
	ctf_top_work_cancel_delayable_enter((uint32_t)(uintptr_t)dwork);
}

void sys_trace_k_work_cancel_delayable_exit(struct k_work_delayable *dwork, int ret)
{
	ctf_top_work_cancel_delayable_exit((uint32_t)(uintptr_t)dwork, (int32_t)ret);
}

void sys_trace_k_work_cancel_delayable_sync_enter(struct k_work_delayable *dwork,
						  struct k_work_sync *sync)
{
	ctf_top_work_cancel_delayable_sync_enter((uint32_t)(uintptr_t)dwork,
						 (uint32_t)(uintptr_t)sync);
}

void sys_trace_k_work_cancel_delayable_sync_exit(struct k_work_delayable *dwork,
						 struct k_work_sync *sync, int ret)
{
	ctf_top_work_cancel_delayable_sync_exit((uint32_t)(uintptr_t)dwork,
						(uint32_t)(uintptr_t)sync, (int32_t)ret);
}

/* Poll Work */
void sys_trace_k_work_poll_init_enter(struct k_work_poll *work)
{
	ctf_top_work_poll_init_enter((uint32_t)(uintptr_t)work);
}

void sys_trace_k_work_poll_init_exit(struct k_work_poll *work)
{
	ctf_top_work_poll_init_exit((uint32_t)(uintptr_t)work);
}

void sys_trace_k_work_poll_submit_to_queue_enter(struct k_work_q *work_q, struct k_work_poll *work,
						 k_timeout_t timeout)
{
	ctf_top_work_poll_submit_to_queue_enter((uint32_t)(uintptr_t)work_q,
						(uint32_t)(uintptr_t)work,
						k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_work_poll_submit_to_queue_blocking(struct k_work_q *work_q,
						    struct k_work_poll *work, k_timeout_t timeout)
{
	ctf_top_work_poll_submit_to_queue_blocking((uint32_t)(uintptr_t)work_q,
						   (uint32_t)(uintptr_t)work,
						   k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_work_poll_submit_to_queue_exit(struct k_work_q *work_q, struct k_work_poll *work,
						k_timeout_t timeout, int ret)
{
	ctf_top_work_poll_submit_to_queue_exit(
		(uint32_t)(uintptr_t)work_q, (uint32_t)(uintptr_t)work,
		k_ticks_to_us_floor32((uint32_t)timeout.ticks), (int32_t)ret);
}

void sys_trace_k_work_poll_submit_enter(struct k_work_poll *work, k_timeout_t timeout)
{
	ctf_top_work_poll_submit_enter((uint32_t)(uintptr_t)work,
				       k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_work_poll_submit_exit(struct k_work_poll *work, k_timeout_t timeout, int ret)
{
	ctf_top_work_poll_submit_exit((uint32_t)(uintptr_t)work,
				      k_ticks_to_us_floor32((uint32_t)timeout.ticks), (int32_t)ret);
}

void sys_trace_k_work_poll_cancel_enter(struct k_work_poll *work)
{
	ctf_top_work_poll_cancel_enter((uint32_t)(uintptr_t)work);
}

void sys_trace_k_work_poll_cancel_exit(struct k_work_poll *work, int ret)
{
	ctf_top_work_poll_cancel_exit((uint32_t)(uintptr_t)work, (int32_t)ret);
}

/* Poll API */
void sys_trace_k_poll_api_event_init(struct k_poll_event *event)
{
	ctf_top_poll_event_init((uint32_t)(uintptr_t)event);
}

void sys_trace_k_poll_api_poll_enter(struct k_poll_event *events)
{
	ctf_top_poll_enter((uint32_t)(uintptr_t)events);
}

void sys_trace_k_poll_api_poll_exit(struct k_poll_event *events, int ret)
{
	ctf_top_poll_exit((uint32_t)(uintptr_t)events, (int32_t)ret);
}

void sys_trace_k_poll_api_signal_init(struct k_poll_signal *sig)
{
	ctf_top_poll_signal_init((uint32_t)(uintptr_t)sig);
}

void sys_trace_k_poll_api_signal_reset(struct k_poll_signal *sig)
{
	ctf_top_poll_signal_reset((uint32_t)(uintptr_t)sig);
}

void sys_trace_k_poll_api_signal_check(struct k_poll_signal *sig)
{
	ctf_top_poll_signal_check((uint32_t)(uintptr_t)sig);
}

void sys_trace_k_poll_api_signal_raise(struct k_poll_signal *sig, int ret)
{
	ctf_top_poll_signal_raise(
		(uint32_t)(uintptr_t)sig,
		(int32_t)ret
		);
}

/* Semaphore */
void sys_trace_k_sem_init(struct k_sem *sem, int ret)
{
	ctf_top_semaphore_init((uint32_t)(uintptr_t)sem, (int32_t)ret);
}

void sys_trace_k_sem_take_enter(struct k_sem *sem, k_timeout_t timeout)
{
	ctf_top_semaphore_take_enter((uint32_t)(uintptr_t)sem,
				     k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_sem_take_blocking(struct k_sem *sem, k_timeout_t timeout)
{
	ctf_top_semaphore_take_blocking((uint32_t)(uintptr_t)sem,
					k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_sem_take_exit(struct k_sem *sem, k_timeout_t timeout, int ret)
{
	ctf_top_semaphore_take_exit((uint32_t)(uintptr_t)sem,
				    k_ticks_to_us_floor32((uint32_t)timeout.ticks), (uint32_t)ret);
}

void sys_trace_k_sem_reset(struct k_sem *sem)
{
	ctf_top_semaphore_reset((uint32_t)(uintptr_t)sem);
}

void sys_trace_k_sem_give_enter(struct k_sem *sem)
{
	ctf_top_semaphore_give_enter((uint32_t)(uintptr_t)sem);
}

void sys_trace_k_sem_give_exit(struct k_sem *sem)
{
	ctf_top_semaphore_give_exit((uint32_t)(uintptr_t)sem);
}

/* Mutex */
void sys_trace_k_mutex_init(struct k_mutex *mutex, int ret)
{
	ctf_top_mutex_init((uint32_t)(uintptr_t)mutex, (int32_t)ret);
}

void sys_trace_k_mutex_lock_enter(struct k_mutex *mutex, k_timeout_t timeout)
{
	ctf_top_mutex_lock_enter((uint32_t)(uintptr_t)mutex,
				 k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_mutex_lock_blocking(struct k_mutex *mutex, k_timeout_t timeout)
{
	ctf_top_mutex_lock_blocking((uint32_t)(uintptr_t)mutex,
				    k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_mutex_lock_exit(struct k_mutex *mutex, k_timeout_t timeout, int ret)
{
	ctf_top_mutex_lock_exit((uint32_t)(uintptr_t)mutex,
				k_ticks_to_us_floor32((uint32_t)timeout.ticks), (int32_t)ret);
}

void sys_trace_k_mutex_unlock_enter(struct k_mutex *mutex)
{
	ctf_top_mutex_unlock_enter((uint32_t)(uintptr_t)mutex);
}

void sys_trace_k_mutex_unlock_exit(struct k_mutex *mutex, int ret)
{
	ctf_top_mutex_unlock_exit((uint32_t)(uintptr_t)mutex, (int32_t)ret);
}

/* Timer */
void sys_trace_k_timer_init(struct k_timer *timer)
{
	ctf_top_timer_init((uint32_t)(uintptr_t)timer);
}

void sys_trace_k_timer_start(struct k_timer *timer, k_timeout_t duration, k_timeout_t period)
{
	ctf_top_timer_start((uint32_t)(uintptr_t)timer,
			    k_ticks_to_us_floor32((uint32_t)duration.ticks),
			    k_ticks_to_us_floor32((uint32_t)period.ticks));
}

void sys_trace_k_timer_stop(struct k_timer *timer)
{
	ctf_top_timer_stop((uint32_t)(uintptr_t)timer);
}

void sys_trace_k_timer_status_sync_enter(struct k_timer *timer)
{
	ctf_top_timer_status_sync_enter((uint32_t)(uintptr_t)timer);
}

void sys_trace_k_timer_status_sync_blocking(struct k_timer *timer, k_timeout_t timeout)
{
	ctf_top_timer_status_sync_blocking((uint32_t)(uintptr_t)timer,
					   k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_timer_status_sync_exit(struct k_timer *timer, uint32_t result)
{
	ctf_top_timer_status_sync_exit((uint32_t)(uintptr_t)timer, result);
}

void sys_trace_k_timer_expiry_enter(struct k_timer *timer)
{
	ctf_top_timer_expiry_enter((uint32_t)(uintptr_t)timer);
}

void sys_trace_k_timer_expiry_exit(struct k_timer *timer)
{
	ctf_top_timer_expiry_exit((uint32_t)(uintptr_t)timer);
}

void sys_trace_k_timer_stop_fn_expiry_enter(struct k_timer *timer)
{
	ctf_top_timer_stop_fn_expiry_enter((uint32_t)(uintptr_t)timer);
}

void sys_trace_k_timer_stop_fn_expiry_exit(struct k_timer *timer)
{
	ctf_top_timer_stop_fn_expiry_exit((uint32_t)(uintptr_t)timer);
}

/* Network socket */
void sys_trace_socket_init(int sock, int family, int type, int proto)
{
	ctf_top_socket_init(sock, family, type, proto);
}

void sys_trace_socket_close_enter(int sock)
{
	ctf_top_socket_close_enter(sock);
}

void sys_trace_socket_close_exit(int sock, int ret)
{
	ctf_top_socket_close_exit(sock, ret);
}

void sys_trace_socket_shutdown_enter(int sock, int how)
{
	ctf_top_socket_shutdown_enter(sock, how);
}

void sys_trace_socket_shutdown_exit(int sock, int ret)
{
	ctf_top_socket_shutdown_exit(sock, ret);
}

void sys_trace_socket_bind_enter(int sock, const struct net_sockaddr *addr, size_t addrlen)
{
	ctf_net_bounded_string_t addr_str = {"unknown"};

	(void)net_addr_ntop(addr->sa_family, &net_sin(addr)->sin_addr, addr_str.buf,
			    sizeof(addr_str.buf));

	ctf_top_socket_bind_enter(sock, addr_str, addrlen, net_ntohs(net_sin(addr)->sin_port));
}

void sys_trace_socket_bind_exit(int sock, int ret)
{
	ctf_top_socket_bind_exit(sock, ret);
}

void sys_trace_socket_connect_enter(int sock, const struct net_sockaddr *addr, size_t addrlen)
{
	ctf_net_bounded_string_t addr_str = {"unknown"};

	(void)net_addr_ntop(addr->sa_family, &net_sin(addr)->sin_addr, addr_str.buf,
			    sizeof(addr_str.buf));

	ctf_top_socket_connect_enter(sock, addr_str, addrlen);
}

void sys_trace_socket_connect_exit(int sock, int ret)
{
	ctf_top_socket_connect_exit(sock, ret);
}

void sys_trace_socket_listen_enter(int sock, int backlog)
{
	ctf_top_socket_listen_enter(sock, backlog);
}

void sys_trace_socket_listen_exit(int sock, int ret)
{
	ctf_top_socket_listen_exit(sock, ret);
}

void sys_trace_socket_accept_enter(int sock)
{
	ctf_top_socket_accept_enter(sock);
}

void sys_trace_socket_accept_exit(int sock, const struct net_sockaddr *addr,
				  const uint32_t *addrlen, int ret)
{
	ctf_net_bounded_string_t addr_str = {"unknown"};
	uint32_t addr_len = 0U;
	uint16_t port = 0U;

	if (addr != NULL) {
		(void)net_addr_ntop(addr->sa_family, &net_sin(addr)->sin_addr, addr_str.buf,
				    sizeof(addr_str.buf));
		port = net_sin(addr)->sin_port;
	}

	if (addrlen != NULL) {
		addr_len = *addrlen;
	}

	ctf_top_socket_accept_exit(sock, addr_str, addr_len, port, ret);
}

void sys_trace_socket_sendto_enter(int sock, int len, int flags,
				   const struct net_sockaddr *dest_addr, uint32_t addrlen)
{
	ctf_net_bounded_string_t addr_str = {"unknown"};

	if (dest_addr != NULL) {
		(void)net_addr_ntop(dest_addr->sa_family, &net_sin(dest_addr)->sin_addr,
				    addr_str.buf, sizeof(addr_str.buf));
	}

	ctf_top_socket_sendto_enter(sock, len, flags, addr_str, addrlen);
}

void sys_trace_socket_sendto_exit(int sock, int ret)
{
	ctf_top_socket_sendto_exit(sock, ret);
}

void sys_trace_socket_sendmsg_enter(int sock, const struct net_msghdr *msg, int flags)
{
	ctf_net_bounded_string_t addr = {"unknown"};
	uint32_t len = 0;

	for (int i = 0; msg->msg_iov != NULL && i < msg->msg_iovlen; i++) {
		len += msg->msg_iov[i].iov_len;
	}

	if (msg->msg_name != NULL) {
		(void)net_addr_ntop(((struct net_sockaddr *)msg->msg_name)->sa_family,
				    &net_sin((struct net_sockaddr *)msg->msg_name)->sin_addr,
				    addr.buf,
				    sizeof(addr.buf));
	}

	ctf_top_socket_sendmsg_enter(sock, flags, (uint32_t)(uintptr_t)msg, addr, len);
}

void sys_trace_socket_sendmsg_exit(int sock, int ret)
{
	ctf_top_socket_sendmsg_exit(sock, ret);
}

void sys_trace_socket_recvfrom_enter(int sock, int max_len, int flags,
				     struct net_sockaddr *addr, uint32_t *addrlen)
{
	ctf_top_socket_recvfrom_enter(sock, max_len, flags, (uint32_t)(uintptr_t)addr,
				      (uint32_t)(uintptr_t)addrlen);
}

void sys_trace_socket_recvfrom_exit(int sock, const struct net_sockaddr *src_addr,
				    const uint32_t *addrlen, int ret)
{
	ctf_net_bounded_string_t addr_str = {"unknown"};
	int len = 0;

	if (src_addr != NULL) {
		(void)net_addr_ntop(src_addr->sa_family, &net_sin(src_addr)->sin_addr,
				    addr_str.buf,
				    sizeof(addr_str.buf));
	}

	if (addrlen != NULL) {
		len = *addrlen;
	}

	ctf_top_socket_recvfrom_exit(sock, addr_str, len, ret);
}

void sys_trace_socket_recvmsg_enter(int sock, const struct net_msghdr *msg, int flags)
{
	uint32_t max_len = 0;

	for (int i = 0; msg->msg_iov != NULL && i < msg->msg_iovlen; i++) {
		max_len += msg->msg_iov[i].iov_len;
	}

	ctf_top_socket_recvmsg_enter(sock, (uint32_t)(uintptr_t)msg, max_len, flags);
}

void sys_trace_socket_recvmsg_exit(int sock, const struct net_msghdr *msg, int ret)
{
	uint32_t len = 0;
	ctf_net_bounded_string_t addr = {"unknown"};

	for (int i = 0; msg->msg_iov != NULL && i < msg->msg_iovlen; i++) {
		len += msg->msg_iov[i].iov_len;
	}

	if (msg->msg_name != NULL) {
		(void)net_addr_ntop(((struct net_sockaddr *)msg->msg_name)->sa_family,
				    &net_sin((struct net_sockaddr *)msg->msg_name)->sin_addr,
				    addr.buf,
				    sizeof(addr.buf));
	}

	ctf_top_socket_recvmsg_exit(sock, len, addr, ret);
}

void sys_trace_socket_fcntl_enter(int sock, int cmd, int flags)
{
	ctf_top_socket_fcntl_enter(sock, cmd, flags);
}

void sys_trace_socket_fcntl_exit(int sock, int ret)
{
	ctf_top_socket_fcntl_exit(sock, ret);
}

void sys_trace_socket_ioctl_enter(int sock, int req)
{
	ctf_top_socket_ioctl_enter(sock, req);
}

void sys_trace_socket_ioctl_exit(int sock, int ret)
{
	ctf_top_socket_ioctl_exit(sock, ret);
}

void sys_trace_socket_poll_value(int fd, int events)
{
	ctf_top_socket_poll_value(fd, events);
}

void sys_trace_socket_poll_enter(const struct zsock_pollfd *fds, int nfds, int timeout)
{
	ctf_top_socket_poll_enter((uint32_t)(uintptr_t)fds, nfds, timeout);

	for (int i = 0; i < nfds; i++) {
		sys_trace_socket_poll_value(fds[i].fd, fds[i].events);
	}
}

void sys_trace_socket_poll_exit(const struct zsock_pollfd *fds, int nfds, int ret)
{
	ctf_top_socket_poll_exit((uint32_t)(uintptr_t)fds, nfds, ret);

	for (int i = 0; i < nfds; i++) {
		sys_trace_socket_poll_value(fds[i].fd, fds[i].revents);
	}
}

void sys_trace_socket_getsockopt_enter(int sock, int level, int optname)
{
	ctf_top_socket_getsockopt_enter(sock, level, optname);
}

void sys_trace_socket_getsockopt_exit(int sock, int level, int optname, void *optval, size_t optlen,
				      int ret)
{
	ctf_top_socket_getsockopt_exit(sock, level, optname, (uint32_t)(uintptr_t)optval, optlen,
				       ret);
}

void sys_trace_socket_setsockopt_enter(int sock, int level, int optname, const void *optval,
				       size_t optlen)
{
	ctf_top_socket_setsockopt_enter(sock, level, optname, (uint32_t)(uintptr_t)optval, optlen);
}

void sys_trace_socket_setsockopt_exit(int sock, int ret)
{
	ctf_top_socket_setsockopt_exit(sock, ret);
}

void sys_trace_socket_getpeername_enter(int sock)
{
	ctf_top_socket_getpeername_enter(sock);
}

void sys_trace_socket_getpeername_exit(int sock,  struct net_sockaddr *addr,
				       const uint32_t *addrlen, int ret)
{
	ctf_net_bounded_string_t addr_str = {"unknown"};

	(void)net_addr_ntop(addr->sa_family, &net_sin(addr)->sin_addr, addr_str.buf,
			    sizeof(addr_str.buf));

	ctf_top_socket_getpeername_exit(sock, addr_str, *addrlen, ret);
}

void sys_trace_socket_getsockname_enter(int sock)
{
	ctf_top_socket_getsockname_enter(sock);
}

void sys_trace_socket_getsockname_exit(int sock, const struct net_sockaddr *addr,
				       const uint32_t *addrlen, int ret)
{
	ctf_net_bounded_string_t addr_str = {"unknown"};

	(void)net_addr_ntop(addr->sa_family, &net_sin(addr)->sin_addr, addr_str.buf,
			    sizeof(addr_str.buf));

	ctf_top_socket_getsockname_exit(sock, addr_str, *addrlen, ret);
}

void sys_trace_socket_socketpair_enter(int family, int type, int proto, int *sv)
{
	ctf_top_socket_socketpair_enter(family, type, proto, (uint32_t)(uintptr_t)sv);
}

void sys_trace_socket_socketpair_exit(int sock_A, int sock_B, int ret)
{
	ctf_top_socket_socketpair_exit(sock_A, sock_B, ret);
}

void sys_trace_net_recv_data_enter(struct net_if *iface, struct net_pkt *pkt)
{
	ctf_top_net_recv_data_enter((int32_t)net_if_get_by_iface(iface), (uint32_t)(uintptr_t)iface,
				    (uint32_t)(uintptr_t)pkt, (uint32_t)net_pkt_get_len(pkt));
}

void sys_trace_net_recv_data_exit(struct net_if *iface, struct net_pkt *pkt, int ret)
{
	ctf_top_net_recv_data_exit((int32_t)net_if_get_by_iface(iface), (uint32_t)(uintptr_t)iface,
				   (uint32_t)(uintptr_t)pkt, (int32_t)ret);
}

void sys_trace_net_send_data_enter(struct net_pkt *pkt)
{
	struct net_if *iface;
	int ifindex;

	iface = net_pkt_iface(pkt);
	if (iface == NULL) {
		ifindex = -1;
	} else {
		ifindex = net_if_get_by_iface(iface);
	}

	ctf_top_net_send_data_enter((int32_t)ifindex, (uint32_t)(uintptr_t)iface,
				    (uint32_t)(uintptr_t)pkt, (uint32_t)net_pkt_get_len(pkt));
}

void sys_trace_net_send_data_exit(struct net_pkt *pkt, int ret)
{
	struct net_if *iface;
	int ifindex;

	iface = net_pkt_iface(pkt);
	if (iface == NULL) {
		ifindex = -1;
	} else {
		ifindex = net_if_get_by_iface(iface);
	}

	ctf_top_net_send_data_exit((int32_t)ifindex, (uint32_t)(uintptr_t)iface,
				   (uint32_t)(uintptr_t)pkt, (int32_t)ret);
}

void sys_trace_net_rx_time(struct net_pkt *pkt, uint32_t end_time)
{
	struct net_if *iface;
	int ifindex;
	uint32_t diff;
	int tc;
	uint32_t duration_us;

	iface = net_pkt_iface(pkt);
	if (iface == NULL) {
		ifindex = -1;
		tc = 0;
		duration_us = 0;
	} else {
		ifindex = net_if_get_by_iface(iface);
		diff = end_time - net_pkt_create_time(pkt);
		tc = net_rx_priority2tc(net_pkt_priority(pkt));
		duration_us = k_cyc_to_ns_floor64(diff) / 1000U;
	}

	ctf_top_net_rx_time((int32_t)ifindex, (uint32_t)(uintptr_t)iface, (uint32_t)(uintptr_t)pkt,
			    (uint32_t)net_pkt_priority(pkt), (uint32_t)tc, (uint32_t)duration_us);
}

void sys_trace_net_tx_time(struct net_pkt *pkt, uint32_t end_time)
{
	struct net_if *iface;
	int ifindex;
	uint32_t diff;
	int tc;
	uint32_t duration_us;

	iface = net_pkt_iface(pkt);
	if (iface == NULL) {
		ifindex = -1;
		tc = 0;
		duration_us = 0;
	} else {
		ifindex = net_if_get_by_iface(iface);
		diff = end_time - net_pkt_create_time(pkt);
		tc = net_rx_priority2tc(net_pkt_priority(pkt));
		duration_us = k_cyc_to_ns_floor64(diff) / 1000U;
	}

	ctf_top_net_tx_time((int32_t)ifindex, (uint32_t)(uintptr_t)iface, (uint32_t)(uintptr_t)pkt,
			    (uint32_t)net_pkt_priority(pkt), (uint32_t)tc, (uint32_t)duration_us);
}

void sys_trace_named_event(const char *name, uint32_t arg0, uint32_t arg1)
{
	ctf_bounded_string_t ctf_name = {""};

	strncpy(ctf_name.buf, name, CTF_MAX_STRING_LEN);
	/* Make sure buffer is NULL terminated */
	ctf_name.buf[CTF_MAX_STRING_LEN - 1] = '\0';

	ctf_named_event(ctf_name, arg0, arg1);
}

static void _get_init_name(const struct init_entry *entry, ctf_bounded_string_t *name)
{
	const struct device *dev = entry->dev;

	if (dev != NULL && dev->name != NULL && dev->name[0] != '\0') {
		strncpy(name->buf, dev->name, sizeof(name->buf));
		name->buf[sizeof(name->buf) - 1] = '\0';
	}
}

void sys_trace_sys_init_enter(const struct init_entry *entry, int level)
{
	ctf_bounded_string_t name = {""};

	_get_init_name(entry, &name);
	ctf_sys_init_enter(name, (uint32_t)(uintptr_t)entry->init_fn, (uint8_t)level);
}

void sys_trace_sys_init_exit(const struct init_entry *entry, int level, int result)
{
	ctf_bounded_string_t name = {""};

	_get_init_name(entry, &name);
	ctf_sys_init_exit(name, (uint32_t)(uintptr_t)entry->init_fn, (uint8_t)level,
			  (int32_t)result);
}

/* GPIO */
void sys_port_trace_gpio_pin_interrupt_configure_enter(const struct device *port, gpio_pin_t pin,
						       gpio_flags_t flags)
{
	ctf_top_gpio_pin_interrupt_configure_enter((uint32_t)(uintptr_t)port, (uint32_t)pin,
						   (uint32_t)flags);
}

void sys_port_trace_gpio_pin_interrupt_configure_exit(const struct device *port, gpio_pin_t pin,
						      int ret)
{
	ctf_top_gpio_pin_interrupt_configure_exit((uint32_t)(uintptr_t)port, (uint32_t)pin,
						  (int32_t)ret);
}

void sys_port_trace_gpio_pin_configure_enter(const struct device *port, gpio_pin_t pin,
					     gpio_flags_t flags)
{
	ctf_top_gpio_pin_configure_enter((uint32_t)(uintptr_t)port, (uint32_t)pin, (uint32_t)flags);
}

void sys_port_trace_gpio_pin_configure_exit(const struct device *port, gpio_pin_t pin, int ret)
{
	ctf_top_gpio_pin_configure_exit((uint32_t)(uintptr_t)port, (uint32_t)pin, (int32_t)ret);
}

void sys_port_trace_gpio_port_get_direction_enter(const struct device *port, gpio_port_pins_t map,
						  gpio_port_pins_t *inputs,
						  gpio_port_pins_t *outputs)
{
	ctf_top_gpio_port_get_direction_enter((uint32_t)(uintptr_t)port, (uint32_t)map,
					      (uint32_t)(uintptr_t)inputs,
					      (uint32_t)(uintptr_t)outputs);
}

void sys_port_trace_gpio_port_get_direction_exit(const struct device *port, int ret)
{
	ctf_top_gpio_port_get_direction_exit((uint32_t)(uintptr_t)port, (int32_t)ret);
}

void sys_port_trace_gpio_pin_get_config_enter(const struct device *port, gpio_pin_t pin,
					      gpio_flags_t flags)
{
	ctf_top_gpio_pin_get_config_enter((uint32_t)(uintptr_t)port, (uint32_t)pin,
					  (uint32_t)flags);
}

void sys_port_trace_gpio_pin_get_config_exit(const struct device *port, gpio_pin_t pin, int ret)
{
	ctf_top_gpio_pin_get_config_exit((uint32_t)(uintptr_t)port, (uint32_t)pin, (int32_t)ret);
}

void sys_port_trace_gpio_port_get_raw_enter(const struct device *port, gpio_port_value_t *value)
{
	ctf_top_gpio_port_get_raw_enter((uint32_t)(uintptr_t)port, (uint32_t)(uintptr_t)value);
}

void sys_port_trace_gpio_port_get_raw_exit(const struct device *port, int ret)
{
	ctf_top_gpio_port_get_raw_exit((uint32_t)(uintptr_t)port, (int32_t)ret);
}

void sys_port_trace_gpio_port_set_masked_raw_enter(const struct device *port, gpio_port_pins_t mask,
						   gpio_port_value_t value)
{
	ctf_top_gpio_port_set_masked_raw_enter((uint32_t)(uintptr_t)port, (uint32_t)mask,
					       (uint32_t)value);
}

void sys_port_trace_gpio_port_set_masked_raw_exit(const struct device *port, int ret)
{
	ctf_top_gpio_port_set_masked_raw_exit((uint32_t)(uintptr_t)port, (int32_t)ret);
}

void sys_port_trace_gpio_port_set_bits_raw_enter(const struct device *port, gpio_port_pins_t pins)
{
	ctf_top_gpio_port_set_bits_raw_enter((uint32_t)(uintptr_t)port, (uint32_t)pins);
}

void sys_port_trace_gpio_port_set_bits_raw_exit(const struct device *port, int ret)
{
	ctf_top_gpio_port_set_bits_raw_exit((uint32_t)(uintptr_t)port, (int32_t)ret);
}

void sys_port_trace_gpio_port_clear_bits_raw_enter(const struct device *port, gpio_port_pins_t pins)
{
	ctf_top_gpio_port_clear_bits_raw_enter((uint32_t)(uintptr_t)port, (uint32_t)pins);
}

void sys_port_trace_gpio_port_clear_bits_raw_exit(const struct device *port, int ret)
{
	ctf_top_gpio_port_clear_bits_raw_exit((uint32_t)(uintptr_t)port, (int32_t)ret);
}

void sys_port_trace_gpio_port_toggle_bits_enter(const struct device *port, gpio_port_pins_t pins)
{
	ctf_top_gpio_port_toggle_bits_enter((uint32_t)(uintptr_t)port, (uint32_t)pins);
}

void sys_port_trace_gpio_port_toggle_bits_exit(const struct device *port, int ret)
{
	ctf_top_gpio_port_toggle_bits_exit((uint32_t)(uintptr_t)port, (int32_t)ret);
}

void sys_port_trace_gpio_init_callback_enter(struct gpio_callback *callback,
					     gpio_callback_handler_t handler,
					     gpio_port_pins_t pin_mask)
{
	ctf_top_gpio_init_callback_enter((uint32_t)(uintptr_t)callback,
					 (uint32_t)(uintptr_t)handler, (uint32_t)pin_mask);
}

void sys_port_trace_gpio_init_callback_exit(struct gpio_callback *callback)
{
	ctf_top_gpio_init_callback_exit((uint32_t)(uintptr_t)callback);
}

void sys_port_trace_gpio_add_callback_enter(const struct device *port,
					    struct gpio_callback *callback)
{
	ctf_top_gpio_add_callback_enter((uint32_t)(uintptr_t)port, (uint32_t)(uintptr_t)callback);
}

void sys_port_trace_gpio_add_callback_exit(const struct device *port, int ret)
{
	ctf_top_gpio_add_callback_exit((uint32_t)(uintptr_t)port, (int32_t)ret);
}

void sys_port_trace_gpio_remove_callback_enter(const struct device *port,
					       struct gpio_callback *callback)
{
	ctf_top_gpio_remove_callback_enter((uint32_t)(uintptr_t)port,
					   (uint32_t)(uintptr_t)callback);
}

void sys_port_trace_gpio_remove_callback_exit(const struct device *port, int ret)
{
	ctf_top_gpio_remove_callback_exit((uint32_t)(uintptr_t)port, (int32_t)ret);
}

void sys_port_trace_gpio_get_pending_int_enter(const struct device *dev)
{
	ctf_top_gpio_get_pending_int_enter((uint32_t)(uintptr_t)dev);
}

void sys_port_trace_gpio_get_pending_int_exit(const struct device *dev, int ret)
{
	ctf_top_gpio_get_pending_int_exit((uint32_t)(uintptr_t)dev, (int32_t)ret);
}

void sys_port_trace_gpio_fire_callbacks_enter(sys_slist_t *list, const struct device *port,
					      gpio_port_pins_t pins)
{
	ctf_top_gpio_fire_callbacks_enter((uint32_t)(uintptr_t)list, (uint32_t)(uintptr_t)port,
					  (uint32_t)pins);
}

void sys_port_trace_gpio_fire_callback(const struct device *port, struct gpio_callback *cb)
{
	ctf_top_gpio_fire_callback((uint32_t)(uintptr_t)port, (uint32_t)(uintptr_t)cb);
}

/* Mailbox */
void sys_trace_k_mbox_init(struct k_mbox *mbox)
{
	ctf_top_mbox_init((uint32_t)(uintptr_t)mbox);
}

void sys_trace_k_mbox_message_put_enter(struct k_mbox *mbox, k_timeout_t timeout)
{
	ctf_top_mbox_message_put_enter((uint32_t)(uintptr_t)mbox, (uint32_t)timeout.ticks);
}

void sys_trace_k_mbox_message_put_blocking(struct k_mbox *mbox, k_timeout_t timeout)
{
	ctf_top_mbox_message_put_blocking((uint32_t)(uintptr_t)mbox, (uint32_t)timeout.ticks);
}

void sys_trace_k_mbox_message_put_exit(struct k_mbox *mbox, k_timeout_t timeout, int ret)
{
	ctf_top_mbox_message_put_exit((uint32_t)(uintptr_t)mbox, (uint32_t)timeout.ticks,
				      (int32_t)ret);
}

void sys_trace_k_mbox_put_enter(struct k_mbox *mbox, k_timeout_t timeout)
{
	ctf_top_mbox_put_enter((uint32_t)(uintptr_t)mbox, (uint32_t)timeout.ticks);
}

void sys_trace_k_mbox_put_exit(struct k_mbox *mbox, k_timeout_t timeout, int ret)
{
	ctf_top_mbox_put_exit((uint32_t)(uintptr_t)mbox, (uint32_t)timeout.ticks, (int32_t)ret);
}

void sys_trace_k_mbox_async_put_enter(struct k_mbox *mbox, struct k_sem *sem)
{
	ctf_top_mbox_async_put_enter((uint32_t)(uintptr_t)mbox, (uint32_t)(uintptr_t)sem);
}

void sys_trace_k_mbox_async_put_exit(struct k_mbox *mbox, struct k_sem *sem)
{
	ctf_top_mbox_async_put_exit((uint32_t)(uintptr_t)mbox, (uint32_t)(uintptr_t)sem);
}

void sys_trace_k_mbox_get_enter(struct k_mbox *mbox, k_timeout_t timeout)
{
	ctf_top_mbox_get_enter((uint32_t)(uintptr_t)mbox, (uint32_t)timeout.ticks);
}

void sys_trace_k_mbox_get_blocking(struct k_mbox *mbox, k_timeout_t timeout)
{
	ctf_top_mbox_get_blocking((uint32_t)(uintptr_t)mbox, (uint32_t)timeout.ticks);
}

void sys_trace_k_mbox_get_exit(struct k_mbox *mbox, k_timeout_t timeout, int ret)
{
	ctf_top_mbox_get_exit((uint32_t)(uintptr_t)mbox, (uint32_t)timeout.ticks, (int32_t)ret);
}

void sys_trace_k_mbox_data_get(struct k_mbox_msg *rx_msg)
{
	ctf_top_mbox_data_get((uint32_t)(uintptr_t)rx_msg);
}

/* Event */
void sys_trace_k_event_init(struct k_event *event)
{
	ctf_top_event_init((uint32_t)(uintptr_t)event);
}

void sys_trace_k_event_post_enter(struct k_event *event, uint32_t events, uint32_t events_mask)
{
	ctf_top_event_post_enter((uint32_t)(uintptr_t)event, events, events_mask);
}

void sys_trace_k_event_post_exit(struct k_event *event, uint32_t events, uint32_t events_mask)
{
	ctf_top_event_post_exit((uint32_t)(uintptr_t)event, events, events_mask);
}

void sys_trace_k_event_wait_enter(struct k_event *event, uint32_t events, uint32_t options,
				  k_timeout_t timeout)
{
	ctf_top_event_wait_enter((uint32_t)(uintptr_t)event, events, options,
				 (uint32_t)timeout.ticks);
}

void sys_trace_k_event_wait_blocking(struct k_event *event, uint32_t events, uint32_t options,
				     k_timeout_t timeout)
{
	ctf_top_event_wait_blocking((uint32_t)(uintptr_t)event, events, options,
				    (uint32_t)timeout.ticks);
}

void sys_trace_k_event_wait_exit(struct k_event *event, uint32_t events, int ret)
{
	ctf_top_event_wait_exit((uint32_t)(uintptr_t)event, events, (int32_t)ret);
}

/* Queue */

void sys_trace_k_queue_init(struct k_queue *queue)
{
	ctf_top_queue_init((uint32_t)(uintptr_t)queue);
}

void sys_trace_k_queue_cancel_wait(struct k_queue *queue)
{
	ctf_top_queue_cancel_wait((uint32_t)(uintptr_t)queue);
}

void sys_trace_k_queue_queue_insert_enter(struct k_queue *queue, bool alloc)
{
	ctf_top_queue_queue_insert_enter((uint32_t)(uintptr_t)queue, (uint8_t)alloc);
}

void sys_trace_k_queue_queue_insert_blocking(struct k_queue *queue, bool alloc, k_timeout_t timeout)
{
	ctf_top_queue_queue_insert_blocking((uint32_t)(uintptr_t)queue, (uint8_t)alloc,
					    k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_queue_queue_insert_exit(struct k_queue *queue, bool alloc, int32_t ret)
{
	ctf_top_queue_queue_insert_exit((uint32_t)(uintptr_t)queue, (uint8_t)alloc, ret);
}

void sys_trace_k_queue_append_enter(struct k_queue *queue)
{
	ctf_top_queue_append_enter((uint32_t)(uintptr_t)queue);
}

void sys_trace_k_queue_append_exit(struct k_queue *queue)
{
	ctf_top_queue_append_exit((uint32_t)(uintptr_t)queue);
}

void sys_trace_k_queue_alloc_append_enter(struct k_queue *queue)
{
	ctf_top_queue_alloc_append_enter((uint32_t)(uintptr_t)queue);
}

void sys_trace_k_queue_alloc_append_exit(struct k_queue *queue, int32_t ret)
{
	ctf_top_queue_alloc_append_exit((uint32_t)(uintptr_t)queue, ret);
}

void sys_trace_k_queue_prepend_enter(struct k_queue *queue)
{
	ctf_top_queue_prepend_enter((uint32_t)(uintptr_t)queue);
}

void sys_trace_k_queue_prepend_exit(struct k_queue *queue)
{
	ctf_top_queue_prepend_exit((uint32_t)(uintptr_t)queue);
}

void sys_trace_k_queue_alloc_prepend_enter(struct k_queue *queue)
{
	ctf_top_queue_alloc_prepend_enter((uint32_t)(uintptr_t)queue);
}

void sys_trace_k_queue_alloc_prepend_exit(struct k_queue *queue, int32_t ret)
{
	ctf_top_queue_alloc_prepend_exit((uint32_t)(uintptr_t)queue, ret);
}

void sys_trace_k_queue_insert_enter(struct k_queue *queue)
{
	ctf_top_queue_insert_enter((uint32_t)(uintptr_t)queue);
}

void sys_trace_k_queue_insert_exit(struct k_queue *queue)
{
	ctf_top_queue_insert_exit((uint32_t)(uintptr_t)queue);
}

void sys_trace_k_queue_append_list_enter(struct k_queue *queue)
{
	ctf_top_queue_append_list_enter((uint32_t)(uintptr_t)queue);
}

void sys_trace_k_queue_append_list_exit(struct k_queue *queue, int ret)
{
	ctf_top_queue_append_list_exit((uint32_t)(uintptr_t)queue, ret);
}

void sys_trace_k_queue_merge_slist_enter(struct k_queue *queue)
{
	ctf_top_queue_merge_slist_enter((uint32_t)(uintptr_t)queue);
}

void sys_trace_k_queue_merge_slist_exit(struct k_queue *queue, int ret)
{
	ctf_top_queue_merge_slist_exit((uint32_t)(uintptr_t)queue, ret);
}

void sys_trace_k_queue_get_enter(struct k_queue *queue, k_timeout_t timeout)
{
	ctf_top_queue_get_enter((uint32_t)(uintptr_t)queue,
				k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_queue_get_blocking(struct k_queue *queue, k_timeout_t timeout)
{
	ctf_top_queue_get_blocking((uint32_t)(uintptr_t)queue,
				   k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_queue_get_exit(struct k_queue *queue, k_timeout_t timeout, void *ret)
{
	ctf_top_queue_get_exit((uint32_t)(uintptr_t)queue,
			       k_ticks_to_us_floor32((uint32_t)timeout.ticks),
			       (uint32_t)(uintptr_t)ret);
}

void sys_trace_k_queue_remove_enter(struct k_queue *queue)
{
	ctf_top_queue_remove_enter((uint32_t)(uintptr_t)queue);
}

void sys_trace_k_queue_remove_exit(struct k_queue *queue, bool ret)
{
	ctf_top_queue_remove_exit((uint32_t)(uintptr_t)queue, (uint8_t)ret);
}

void sys_trace_k_queue_unique_append_enter(struct k_queue *queue)
{
	ctf_top_queue_unique_append_enter((uint32_t)(uintptr_t)queue);
}

void sys_trace_k_queue_unique_append_exit(struct k_queue *queue, bool ret)
{
	ctf_top_queue_unique_append_exit((uint32_t)(uintptr_t)queue, (uint8_t)ret);
}

void sys_trace_k_queue_peek_head(struct k_queue *queue, void *ret)
{
	ctf_top_queue_peek_head((uint32_t)(uintptr_t)queue, (uint32_t)(uintptr_t)ret);
}

void sys_trace_k_queue_peek_tail(struct k_queue *queue, void *ret)
{
	ctf_top_queue_peek_tail((uint32_t)(uintptr_t)queue, (uint32_t)(uintptr_t)ret);
}

/* FIFO */

void sys_trace_k_fifo_init_enter(struct k_fifo *fifo)
{
	ctf_top_fifo_init_enter((uint32_t)(uintptr_t)fifo);
}

void sys_trace_k_fifo_init_exit(struct k_fifo *fifo)
{
	ctf_top_fifo_init_exit((uint32_t)(uintptr_t)fifo);
}

void sys_trace_k_fifo_cancel_wait_enter(struct k_fifo *fifo)
{
	ctf_top_fifo_cancel_wait_enter((uint32_t)(uintptr_t)fifo);
}

void sys_trace_k_fifo_cancel_wait_exit(struct k_fifo *fifo)
{
	ctf_top_fifo_cancel_wait_exit((uint32_t)(uintptr_t)fifo);
}

void sys_trace_k_fifo_put_enter(struct k_fifo *fifo, void *data)
{
	ctf_top_fifo_put_enter((uint32_t)(uintptr_t)fifo, (uint32_t)(uintptr_t)data);
}

void sys_trace_k_fifo_put_exit(struct k_fifo *fifo, void *data)
{
	ctf_top_fifo_put_exit((uint32_t)(uintptr_t)fifo, (uint32_t)(uintptr_t)data);
}

void sys_trace_k_fifo_alloc_put_enter(struct k_fifo *fifo, void *data)
{
	ctf_top_fifo_alloc_put_enter((uint32_t)(uintptr_t)fifo, (uint32_t)(uintptr_t)data);
}

void sys_trace_k_fifo_alloc_put_exit(struct k_fifo *fifo, void *data, int ret)
{
	ctf_top_fifo_alloc_put_exit((uint32_t)(uintptr_t)fifo, (uint32_t)(uintptr_t)data, ret);
}

void sys_trace_k_fifo_put_list_enter(struct k_fifo *fifo, void *head, void *tail)
{
	ctf_top_fifo_put_list_enter((uint32_t)(uintptr_t)fifo, (uint32_t)(uintptr_t)head,
				    (uint32_t)(uintptr_t)tail);
}

void sys_trace_k_fifo_put_list_exit(struct k_fifo *fifo, void *head, void *tail)
{
	ctf_top_fifo_put_list_exit((uint32_t)(uintptr_t)fifo, (uint32_t)(uintptr_t)head,
				   (uint32_t)(uintptr_t)tail);
}

void sys_trace_k_fifo_put_slist_enter(struct k_fifo *fifo, sys_slist_t *list)
{
	ctf_top_fifo_put_slist_enter((uint32_t)(uintptr_t)fifo, (uint32_t)(uintptr_t)list);
}

void sys_trace_k_fifo_put_slist_exit(struct k_fifo *fifo, sys_slist_t *list)
{
	ctf_top_fifo_put_slist_exit((uint32_t)(uintptr_t)fifo, (uint32_t)(uintptr_t)list);
}

void sys_trace_k_fifo_get_enter(struct k_fifo *fifo, k_timeout_t timeout)
{
	ctf_top_fifo_get_enter((uint32_t)(uintptr_t)fifo,
			       k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_fifo_get_exit(struct k_fifo *fifo, k_timeout_t timeout, void *ret)
{
	ctf_top_fifo_get_exit((uint32_t)(uintptr_t)fifo,
			      k_ticks_to_us_floor32((uint32_t)timeout.ticks),
			      (uint32_t)(uintptr_t)ret);
}

void sys_trace_k_fifo_peek_head_enter(struct k_fifo *fifo)
{
	ctf_top_fifo_peek_head_enter((uint32_t)(uintptr_t)fifo);
}

void sys_trace_k_fifo_peek_head_exit(struct k_fifo *fifo, void *ret)
{
	ctf_top_fifo_peek_head_exit((uint32_t)(uintptr_t)fifo, (uint32_t)(uintptr_t)ret);
}

void sys_trace_k_fifo_peek_tail_enter(struct k_fifo *fifo)
{
	ctf_top_fifo_peek_tail_enter((uint32_t)(uintptr_t)fifo);
}

void sys_trace_k_fifo_peek_tail_exit(struct k_fifo *fifo, void *ret)
{
	ctf_top_fifo_peek_tail_exit((uint32_t)(uintptr_t)fifo, (uint32_t)(uintptr_t)ret);
}

/* LIFO */

void sys_trace_k_lifo_init_enter(struct k_lifo *lifo)
{
	ctf_top_lifo_init_enter((uint32_t)(uintptr_t)lifo);
}

void sys_trace_k_lifo_init_exit(struct k_lifo *lifo)
{
	ctf_top_lifo_init_exit((uint32_t)(uintptr_t)lifo);
}

void sys_trace_k_lifo_put_enter(struct k_lifo *lifo, void *data)
{
	ctf_top_lifo_put_enter((uint32_t)(uintptr_t)lifo, (uint32_t)(uintptr_t)data);
}

void sys_trace_k_lifo_put_exit(struct k_lifo *lifo, void *data)
{
	ctf_top_lifo_put_exit((uint32_t)(uintptr_t)lifo, (uint32_t)(uintptr_t)data);
}

void sys_trace_k_lifo_alloc_put_enter(struct k_lifo *lifo, void *data)
{
	ctf_top_lifo_alloc_put_enter((uint32_t)(uintptr_t)lifo, (uint32_t)(uintptr_t)data);
}

void sys_trace_k_lifo_alloc_put_exit(struct k_lifo *lifo, void *data, int ret)
{
	ctf_top_lifo_alloc_put_exit((uint32_t)(uintptr_t)lifo, (uint32_t)(uintptr_t)data, ret);
}

void sys_trace_k_lifo_get_enter(struct k_lifo *lifo, k_timeout_t timeout)
{
	ctf_top_lifo_get_enter((uint32_t)(uintptr_t)lifo,
			       k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_lifo_get_exit(struct k_lifo *lifo, k_timeout_t timeout, void *ret)
{
	ctf_top_lifo_get_exit((uint32_t)(uintptr_t)lifo,
			      k_ticks_to_us_floor32((uint32_t)timeout.ticks),
			      (uint32_t)(uintptr_t)ret);
}

/* Stack */

void sys_trace_k_stack_init(struct k_stack *stack)
{
	ctf_top_stack_init((uint32_t)(uintptr_t)stack);
}

void sys_trace_k_stack_alloc_init_enter(struct k_stack *stack)
{
	ctf_top_stack_alloc_init_enter((uint32_t)(uintptr_t)stack);
}

void sys_trace_k_stack_alloc_init_exit(struct k_stack *stack, int32_t ret)
{
	ctf_top_stack_alloc_init_exit((uint32_t)(uintptr_t)stack, ret);
}

void sys_trace_k_stack_cleanup_enter(struct k_stack *stack)
{
	ctf_top_stack_cleanup_enter((uint32_t)(uintptr_t)stack);
}

void sys_trace_k_stack_cleanup_exit(struct k_stack *stack, int ret)
{
	ctf_top_stack_cleanup_exit((uint32_t)(uintptr_t)stack, ret);
}

void sys_trace_k_stack_push_enter(struct k_stack *stack)
{
	ctf_top_stack_push_enter((uint32_t)(uintptr_t)stack);
}

void sys_trace_k_stack_push_exit(struct k_stack *stack, int ret)
{
	ctf_top_stack_push_exit((uint32_t)(uintptr_t)stack, ret);
}

void sys_trace_k_stack_pop_enter(struct k_stack *stack, k_timeout_t timeout)
{
	ctf_top_stack_pop_enter((uint32_t)(uintptr_t)stack,
				k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_stack_pop_blocking(struct k_stack *stack, k_timeout_t timeout)
{
	ctf_top_stack_pop_blocking((uint32_t)(uintptr_t)stack,
				   k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_stack_pop_exit(struct k_stack *stack, k_timeout_t timeout, int ret)
{
	ctf_top_stack_pop_exit((uint32_t)(uintptr_t)stack,
			       k_ticks_to_us_floor32((uint32_t)timeout.ticks), ret);
}

/* Heap */

void sys_trace_k_heap_init(struct k_heap *h)
{
	ctf_top_heap_init((uint32_t)(uintptr_t)h);
}

void sys_trace_k_heap_aligned_alloc_enter(struct k_heap *h, k_timeout_t timeout)
{
	ctf_top_heap_aligned_alloc_enter((uint32_t)(uintptr_t)h,
					 k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_heap_alloc_helper_blocking(struct k_heap *h, k_timeout_t timeout)
{
	ctf_top_heap_alloc_helper_blocking((uint32_t)(uintptr_t)h,
					   k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_heap_aligned_alloc_exit(struct k_heap *h, k_timeout_t timeout, void *ret)
{
	ctf_top_heap_aligned_alloc_exit((uint32_t)(uintptr_t)h,
					k_ticks_to_us_floor32((uint32_t)timeout.ticks),
					(uint32_t)(uintptr_t)ret);
}

void sys_trace_k_heap_alloc_enter(struct k_heap *h, k_timeout_t timeout)
{
	ctf_top_heap_alloc_enter((uint32_t)(uintptr_t)h,
				 k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_heap_alloc_exit(struct k_heap *h, k_timeout_t timeout, void *ret)
{
	ctf_top_heap_alloc_exit((uint32_t)(uintptr_t)h,
				k_ticks_to_us_floor32((uint32_t)timeout.ticks),
				(uint32_t)(uintptr_t)ret);
}

void sys_trace_k_heap_calloc_enter(struct k_heap *h, k_timeout_t timeout)
{
	ctf_top_heap_calloc_enter((uint32_t)(uintptr_t)h,
				  k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_heap_calloc_exit(struct k_heap *h, k_timeout_t timeout, void *ret)
{
	ctf_top_heap_calloc_exit((uint32_t)(uintptr_t)h,
				 k_ticks_to_us_floor32((uint32_t)timeout.ticks),
				 (uint32_t)(uintptr_t)ret);
}

void sys_trace_k_heap_free(struct k_heap *h)
{
	ctf_top_heap_free((uint32_t)(uintptr_t)h);
}

void sys_trace_k_heap_realloc_enter(struct k_heap *h, void *ptr, size_t bytes, k_timeout_t timeout)
{
	ctf_top_heap_realloc_enter((uint32_t)(uintptr_t)h, (uint32_t)(uintptr_t)ptr,
				   (uint32_t)bytes, k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_heap_realloc_exit(struct k_heap *h, void *ptr, size_t bytes, k_timeout_t timeout,
				   void *ret)
{
	ctf_top_heap_realloc_exit((uint32_t)(uintptr_t)h, (uint32_t)(uintptr_t)ptr, (uint32_t)bytes,
				  k_ticks_to_us_floor32((uint32_t)timeout.ticks),
				  (uint32_t)(uintptr_t)ret);
}

/* System heap */

void sys_trace_k_heap_sys_k_aligned_alloc_enter(struct k_heap *heap)
{
	ctf_top_heap_sys_k_aligned_alloc_enter((uint32_t)(uintptr_t)heap);
}

void sys_trace_k_heap_sys_k_aligned_alloc_exit(struct k_heap *heap, void *ret)
{
	ctf_top_heap_sys_k_aligned_alloc_exit((uint32_t)(uintptr_t)heap, (uint32_t)(uintptr_t)ret);
}

void sys_trace_k_heap_sys_k_malloc_enter(struct k_heap *heap)
{
	ctf_top_heap_sys_k_malloc_enter((uint32_t)(uintptr_t)heap);
}

void sys_trace_k_heap_sys_k_malloc_exit(struct k_heap *heap, void *ret)
{
	ctf_top_heap_sys_k_malloc_exit((uint32_t)(uintptr_t)heap, (uint32_t)(uintptr_t)ret);
}

void sys_trace_k_heap_sys_k_calloc_enter(struct k_heap *heap)
{
	ctf_top_heap_sys_k_calloc_enter((uint32_t)(uintptr_t)heap);
}

void sys_trace_k_heap_sys_k_calloc_exit(struct k_heap *heap, void *ret)
{
	ctf_top_heap_sys_k_calloc_exit((uint32_t)(uintptr_t)heap, (uint32_t)(uintptr_t)ret);
}

void sys_trace_k_heap_sys_k_free_enter(struct k_heap *heap, struct k_heap **heap_ref)
{
	ctf_top_heap_sys_k_free_enter((uint32_t)(uintptr_t)heap, (uint32_t)(uintptr_t)heap_ref);
}

void sys_trace_k_heap_sys_k_free_exit(struct k_heap *heap, void *heap_ref)
{
	ctf_top_heap_sys_k_free_exit((uint32_t)(uintptr_t)heap, (uint32_t)(uintptr_t)heap_ref);
}

void sys_trace_k_heap_sys_k_realloc_enter(struct k_heap *heap, void *ptr)
{
	ctf_top_heap_sys_k_realloc_enter((uint32_t)(uintptr_t)heap, (uint32_t)(uintptr_t)ptr);
}

void sys_trace_k_heap_sys_k_realloc_exit(struct k_heap *heap, void *ptr, void *ret)
{
	ctf_top_heap_sys_k_realloc_exit((uint32_t)(uintptr_t)heap, (uint32_t)(uintptr_t)ptr,
					(uint32_t)(uintptr_t)ret);
}

/* Pipe */

void sys_trace_k_pipe_init(struct k_pipe *pipe, uint8_t *buffer, size_t size)
{
	ctf_top_pipe_init((uint32_t)(uintptr_t)pipe, (uint32_t)(uintptr_t)buffer, (uint32_t)size);
}

void sys_trace_k_pipe_reset_enter(struct k_pipe *pipe)
{
	ctf_top_pipe_reset_enter((uint32_t)(uintptr_t)pipe);
}

void sys_trace_k_pipe_reset_exit(struct k_pipe *pipe)
{
	ctf_top_pipe_reset_exit((uint32_t)(uintptr_t)pipe);
}

void sys_trace_k_pipe_close_enter(struct k_pipe *pipe)
{
	ctf_top_pipe_close_enter((uint32_t)(uintptr_t)pipe);
}

void sys_trace_k_pipe_close_exit(struct k_pipe *pipe)
{
	ctf_top_pipe_close_exit((uint32_t)(uintptr_t)pipe);
}

void sys_trace_k_pipe_write_enter(struct k_pipe *pipe, const uint8_t *data, size_t len,
				  k_timeout_t timeout)
{
	ctf_top_pipe_write_enter((uint32_t)(uintptr_t)pipe, (uint32_t)(uintptr_t)data,
				 (uint32_t)len, k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_pipe_write_blocking(struct k_pipe *pipe, k_timeout_t timeout)
{
	ctf_top_pipe_write_blocking((uint32_t)(uintptr_t)pipe,
				    k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_pipe_write_exit(struct k_pipe *pipe, int ret)
{
	ctf_top_pipe_write_exit((uint32_t)(uintptr_t)pipe, ret);
}

void sys_trace_k_pipe_read_enter(struct k_pipe *pipe, uint8_t *data, size_t len,
				 k_timeout_t timeout)
{
	ctf_top_pipe_read_enter((uint32_t)(uintptr_t)pipe, (uint32_t)(uintptr_t)data, (uint32_t)len,
				k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_pipe_read_blocking(struct k_pipe *pipe, k_timeout_t timeout)
{
	ctf_top_pipe_read_blocking((uint32_t)(uintptr_t)pipe,
				   k_ticks_to_us_floor32((uint32_t)timeout.ticks));
}

void sys_trace_k_pipe_read_exit(struct k_pipe *pipe, int ret)
{
	ctf_top_pipe_read_exit((uint32_t)(uintptr_t)pipe, ret);
}

/* RTIO */

void sys_trace_rtio_submit_enter(const struct rtio *r, uint32_t wait_count)
{
	ctf_top_rtio_submit_enter((uint32_t)(uintptr_t)r, (uint32_t)wait_count);
}

void sys_trace_rtio_submit_exit(const struct rtio *r)
{
	ctf_top_rtio_submit_exit((uint32_t)(uintptr_t)r);
}

void sys_trace_rtio_sqe_acquire_enter(const struct rtio *r)
{
	ctf_top_rtio_sqe_acquire_enter((uint32_t)(uintptr_t)r);
}

void sys_trace_rtio_sqe_acquire_exit(const struct rtio *r, const struct rtio_sqe *sqe)
{
	ctf_top_rtio_sqe_acquire_exit((uint32_t)(uintptr_t)r, (uint32_t)(uintptr_t)sqe);
}

void sys_trace_rtio_sqe_cancel(const struct rtio_sqe *sqe)
{
	ctf_top_rtio_sqe_cancel((uint32_t)(uintptr_t)sqe);
}

void sys_trace_rtio_cqe_submit_enter(const struct rtio *r, int result, uint32_t flags)
{
	ctf_top_rtio_cqe_submit_enter((uint32_t)(uintptr_t)r, result, (uint32_t)flags);
}

void sys_trace_rtio_cqe_submit_exit(const struct rtio *r)
{
	ctf_top_rtio_cqe_submit_exit((uint32_t)(uintptr_t)r);
}

void sys_trace_rtio_cqe_acquire_enter(const struct rtio *r)
{
	ctf_top_rtio_cqe_acquire_enter((uint32_t)(uintptr_t)r);
}

void sys_trace_rtio_cqe_acquire_exit(const struct rtio *r, const struct rtio_cqe *cqe)
{
	ctf_top_rtio_cqe_acquire_exit((uint32_t)(uintptr_t)r, (uint32_t)(uintptr_t)cqe);
}

void sys_trace_rtio_cqe_release(const struct rtio *r, const struct rtio_cqe *cqe)
{
	ctf_top_rtio_cqe_release((uint32_t)(uintptr_t)r, (uint32_t)(uintptr_t)cqe);
}

void sys_trace_rtio_cqe_consume_enter(const struct rtio *r)
{
	ctf_top_rtio_cqe_consume_enter((uint32_t)(uintptr_t)r);
}

void sys_trace_rtio_cqe_consume_exit(const struct rtio *r, const struct rtio_cqe *cqe)
{
	ctf_top_rtio_cqe_consume_exit((uint32_t)(uintptr_t)r, (uint32_t)(uintptr_t)cqe);
}

void sys_trace_rtio_txn_next_enter(const struct rtio *r, const struct rtio_iodev_sqe *iodev_sqe)
{
	ctf_top_rtio_txn_next_enter((uint32_t)(uintptr_t)r, (uint32_t)(uintptr_t)iodev_sqe);
}

void sys_trace_rtio_txn_next_exit(const struct rtio *r, const struct rtio_iodev_sqe *iodev_sqe)
{
	ctf_top_rtio_txn_next_exit((uint32_t)(uintptr_t)r, (uint32_t)(uintptr_t)iodev_sqe);
}

void sys_trace_rtio_chain_next_enter(const struct rtio *r, const struct rtio_iodev_sqe *iodev_sqe)
{
	ctf_top_rtio_chain_next_enter((uint32_t)(uintptr_t)r, (uint32_t)(uintptr_t)iodev_sqe);
}

void sys_trace_rtio_chain_next_exit(const struct rtio *r, const struct rtio_iodev_sqe *iodev_sqe)
{
	ctf_top_rtio_chain_next_exit((uint32_t)(uintptr_t)r, (uint32_t)(uintptr_t)iodev_sqe);
}

/* PM device runtime */

void sys_trace_pm_device_runtime_get_enter(const struct device *dev)
{
	ctf_top_pm_device_runtime_get_enter((uint32_t)(uintptr_t)dev);
}

void sys_trace_pm_device_runtime_get_exit(const struct device *dev, int ret)
{
	ctf_top_pm_device_runtime_get_exit((uint32_t)(uintptr_t)dev, ret);
}

void sys_trace_pm_device_runtime_put_enter(const struct device *dev)
{
	ctf_top_pm_device_runtime_put_enter((uint32_t)(uintptr_t)dev);
}

void sys_trace_pm_device_runtime_put_exit(const struct device *dev, int ret)
{
	ctf_top_pm_device_runtime_put_exit((uint32_t)(uintptr_t)dev, ret);
}

void sys_trace_pm_device_runtime_put_async_enter(const struct device *dev, k_timeout_t delay)
{
	ctf_top_pm_device_runtime_put_async_enter((uint32_t)(uintptr_t)dev,
						  k_ticks_to_us_floor32((uint32_t)delay.ticks));
}

void sys_trace_pm_device_runtime_put_async_exit(const struct device *dev, k_timeout_t delay,
						int ret)
{
	ctf_top_pm_device_runtime_put_async_exit((uint32_t)(uintptr_t)dev,
						 k_ticks_to_us_floor32((uint32_t)delay.ticks), ret);
}

void sys_trace_pm_device_runtime_enable_enter(const struct device *dev)
{
	ctf_top_pm_device_runtime_enable_enter((uint32_t)(uintptr_t)dev);
}

void sys_trace_pm_device_runtime_enable_exit(const struct device *dev, int ret)
{
	ctf_top_pm_device_runtime_enable_exit((uint32_t)(uintptr_t)dev, ret);
}

void sys_trace_pm_device_runtime_disable_enter(const struct device *dev)
{
	ctf_top_pm_device_runtime_disable_enter((uint32_t)(uintptr_t)dev);
}

void sys_trace_pm_device_runtime_disable_exit(const struct device *dev, int ret)
{
	ctf_top_pm_device_runtime_disable_exit((uint32_t)(uintptr_t)dev, ret);
}

/* PM system */

void sys_trace_pm_system_suspend_enter(int32_t ticks)
{
	ctf_top_pm_system_suspend_enter(ticks);
}

void sys_trace_pm_system_suspend_exit(int32_t ticks, enum pm_state state)
{
	ctf_top_pm_system_suspend_exit(ticks, (uint8_t)state);
}

/* Syscall */

void sys_trace_syscall_enter(uint32_t id, const char *name)
{
	ctf_bounded_string_t ctf_name = {""};

	strncpy(ctf_name.buf, name, CTF_MAX_STRING_LEN);
	ctf_name.buf[CTF_MAX_STRING_LEN - 1] = '\0';

	ctf_top_syscall_enter(id, ctf_name);
}

void sys_trace_syscall_exit(uint32_t id)
{
	ctf_top_syscall_exit(id);
}
