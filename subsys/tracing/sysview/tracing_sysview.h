/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_TRACE_SYSVIEW_H
#define ZEPHYR_TRACE_SYSVIEW_H
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <tracing_sysview_ids.h>

#include <SEGGER_SYSVIEW.h>

#ifdef __cplusplus
extern "C" {
#endif

void sys_trace_thread_info(struct k_thread *thread);

#define sys_port_trace_k_thread_foreach_enter() SEGGER_SYSVIEW_RecordVoid(TID_THREAD_FOREACH)

#define sys_port_trace_k_thread_foreach_exit() SEGGER_SYSVIEW_RecordEndCall(TID_THREAD_FOREACH)

#define sys_port_trace_k_thread_foreach_unlocked_enter()                                           \
	SEGGER_SYSVIEW_RecordVoid(TID_THREAD_FOREACH_UNLOCKED)

#define sys_port_trace_k_thread_foreach_unlocked_exit()                                            \
	SEGGER_SYSVIEW_RecordEndCall(TID_THREAD_FOREACH_UNLOCKED)

#define sys_port_trace_k_thread_create(new_thread)                                                 \
	do {                                                                                       \
		SEGGER_SYSVIEW_OnTaskCreate((uint32_t)(uintptr_t)new_thread);                      \
		sys_trace_thread_info(new_thread);                                                 \
	} while (0)

#define sys_port_trace_k_thread_user_mode_enter()                                                  \
	SEGGER_SYSVIEW_RecordVoid(TID_THREAD_USERMODE_ENTER)

#define sys_port_trace_k_thread_heap_assign(thread, heap)
#define sys_port_trace_k_thread_join_enter(thread, timeout)                                        \
	SEGGER_SYSVIEW_RecordU32x2(TID_THREAD_JOIN, (uint32_t)(uintptr_t)thread,                   \
				   (uint32_t)timeout.ticks)
#define sys_port_trace_k_thread_join_blocking(thread, timeout)
#define sys_port_trace_k_thread_join_exit(thread, timeout, ret)                                    \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_THREAD_JOIN, (int32_t)ret)

#define sys_port_trace_k_thread_sleep_enter(timeout)                                               \
	SEGGER_SYSVIEW_RecordU32(TID_SLEEP, (uint32_t)k_ticks_to_ms_floor32(timeout.ticks))

#define sys_port_trace_k_thread_sleep_exit(timeout, ret)                                           \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_SLEEP, (int32_t)ret)

#define sys_port_trace_k_thread_msleep_enter(ms) SEGGER_SYSVIEW_RecordU32(TID_MSLEEP, (uint32_t)ms)

#define sys_port_trace_k_thread_msleep_exit(ms, ret)                                               \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_MSLEEP, (int32_t)ret)

#define sys_port_trace_k_thread_usleep_enter(us) SEGGER_SYSVIEW_RecordU32(TID_USLEEP, (uint32_t)us)

#define sys_port_trace_k_thread_usleep_exit(us, ret)                                               \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_USLEEP, (int32_t)ret)

#define sys_port_trace_k_thread_busy_wait_enter(usec_to_wait)                                      \
	SEGGER_SYSVIEW_RecordU32(TID_BUSYWAIT, (uint32_t)usec_to_wait)

#define sys_port_trace_k_thread_busy_wait_exit(usec_to_wait)                                       \
	SEGGER_SYSVIEW_RecordEndCall(TID_BUSYWAIT)

#define sys_port_trace_k_thread_yield() SEGGER_SYSVIEW_RecordVoid(TID_THREAD_YIELD)

#define sys_port_trace_k_thread_wakeup(thread)                                                     \
	SEGGER_SYSVIEW_RecordU32(TID_THREAD_WAKEUP, (uint32_t)(uintptr_t)thread)

#define sys_port_trace_k_thread_start(thread)                                                      \
	SEGGER_SYSVIEW_RecordU32(TID_THREAD_START, (uint32_t)(uintptr_t)thread)

#define sys_port_trace_k_thread_abort(thread)                                                      \
	SEGGER_SYSVIEW_RecordU32(TID_THREAD_ABORT, (uint32_t)(uintptr_t)thread)

#define sys_port_trace_k_thread_abort_enter(thread)                                                \
	SEGGER_SYSVIEW_RecordU32(TID_THREAD_ABORT, (uint32_t)(uintptr_t)thread)

#define sys_port_trace_k_thread_abort_exit(thread)                                                 \
	SEGGER_SYSVIEW_RecordEndCall(TID_THREAD_ABORT)

#define sys_port_trace_k_thread_suspend_enter(thread)                                              \
	SEGGER_SYSVIEW_RecordU32(TID_THREAD_SUSPEND, (uint32_t)(uintptr_t)thread)

#define sys_port_trace_k_thread_suspend_exit(thread)                                               \
	SEGGER_SYSVIEW_RecordEndCall(TID_THREAD_SUSPEND)

#define sys_port_trace_k_thread_resume_enter(thread)                                               \
	SEGGER_SYSVIEW_RecordU32(TID_THREAD_RESUME, (uint32_t)(uintptr_t)thread)

#define sys_port_trace_k_thread_resume_exit(thread) SEGGER_SYSVIEW_RecordEndCall(TID_THREAD_RESUME)

#define sys_port_trace_k_thread_sched_lock()

#define sys_port_trace_k_thread_sched_unlock()

#define sys_port_trace_k_thread_name_set(thread, ret) do { \
		SEGGER_SYSVIEW_RecordU32(TID_THREAD_NAME_SET, (uint32_t)(uintptr_t)thread); \
		sys_trace_thread_info(thread);	\
	} while (0)

#define sys_port_trace_k_thread_switched_out() sys_trace_k_thread_switched_out()

#define sys_port_trace_k_thread_switched_in() sys_trace_k_thread_switched_in()

#define sys_port_trace_k_thread_info(thread) sys_trace_k_thread_info(thread)

#define sys_port_trace_k_thread_sched_wakeup(thread)                                               \
	SEGGER_SYSVIEW_RecordU32(TID_THREAD_WAKEUP, (uint32_t)(uintptr_t)thread)

#define sys_port_trace_k_thread_sched_abort(thread)                                                \
	SEGGER_SYSVIEW_RecordU32(TID_THREAD_ABORT, (uint32_t)(uintptr_t)thread)

#define sys_port_trace_k_thread_sched_priority_set(thread, prio)                                   \
	SEGGER_SYSVIEW_RecordU32x2(TID_THREAD_PRIORITY_SET,                                        \
				   SEGGER_SYSVIEW_ShrinkId((uint32_t)thread), prio);

#define sys_port_trace_k_thread_sched_ready(thread)                                                \
	SEGGER_SYSVIEW_OnTaskStartReady((uint32_t)(uintptr_t)thread)

#define sys_port_trace_k_thread_sched_pend(thread)                                                 \
	SEGGER_SYSVIEW_OnTaskStopReady((uint32_t)(uintptr_t)thread, 3 << 3)

#define sys_port_trace_k_thread_sched_resume(thread)

#define sys_port_trace_k_thread_sched_suspend(thread)                                              \
	SEGGER_SYSVIEW_OnTaskStopReady((uint32_t)(uintptr_t)thread, 3 << 3)

#define sys_port_trace_k_work_init(work)                                                           \
	SEGGER_SYSVIEW_RecordU32(TID_WORK_INIT, (uint32_t)(uintptr_t)work)

#define sys_port_trace_k_work_submit_to_queue_enter(queue, work)                                   \
	SEGGER_SYSVIEW_RecordU32x2(TID_WORK_SUBMIT_TO_QUEUE, (uint32_t)(uintptr_t)queue,           \
				   (uint32_t)(uintptr_t)work)

#define sys_port_trace_k_work_submit_to_queue_exit(queue, work, ret)                               \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_WORK_SUBMIT_TO_QUEUE, (uint32_t)ret)

#define sys_port_trace_k_work_submit_enter(work)                                                   \
	SEGGER_SYSVIEW_RecordU32(TID_WORK_SUBMIT, (uint32_t)(uintptr_t)work)

#define sys_port_trace_k_work_submit_exit(work, ret)                                               \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_WORK_SUBMIT, (uint32_t)ret)

#define sys_port_trace_k_work_flush_enter(work)                                                    \
	SEGGER_SYSVIEW_RecordU32(TID_WORK_FLUSH, (uint32_t)(uintptr_t)work)

#define sys_port_trace_k_work_flush_blocking(work, timeout)

#define sys_port_trace_k_work_flush_exit(work, ret)                                                \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_WORK_FLUSH, (uint32_t)ret)

#define sys_port_trace_k_work_cancel_enter(work)                                                   \
	SEGGER_SYSVIEW_RecordU32(TID_WORK_CANCEL, (uint32_t)(uintptr_t)work)

#define sys_port_trace_k_work_cancel_exit(work, ret)                                               \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_WORK_CANCEL, (uint32_t)ret)

#define sys_port_trace_k_work_cancel_sync_enter(work, sync)                                        \
	SEGGER_SYSVIEW_RecordU32x2(TID_WORK_CANCEL_SYNC, (uint32_t)(uintptr_t)work,                \
				   (uint32_t)(uintptr_t)sync)

#define sys_port_trace_k_work_cancel_sync_blocking(work, sync)

#define sys_port_trace_k_work_cancel_sync_exit(work, sync, ret)                                    \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_WORK_CANCEL_SYNC, (uint32_t)ret)

#define sys_port_trace_k_work_queue_init(queue)             \
	SEGGER_SYSVIEW_RecordU32(TID_WORK_QUEUE_INIT,       \
				 (uint32_t)(uintptr_t)queue)

#define sys_port_trace_k_work_queue_start_enter(queue)                                             \
	SEGGER_SYSVIEW_RecordU32(TID_WORK_QUEUE_START, (uint32_t)(uintptr_t)queue)

#define sys_port_trace_k_work_queue_start_exit(queue)                                              \
	SEGGER_SYSVIEW_RecordEndCall(TID_WORK_QUEUE_START)

#define sys_port_trace_k_work_queue_drain_enter(queue)                                             \
	SEGGER_SYSVIEW_RecordU32(TID_WORK_QUEUE_DRAIN, (uint32_t)(uintptr_t)queue)

#define sys_port_trace_k_work_queue_drain_exit(queue, ret)                                         \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_WORK_QUEUE_DRAIN, (uint32_t)ret)

#define sys_port_trace_k_work_queue_unplug_enter(queue)                                            \
	SEGGER_SYSVIEW_RecordU32(TID_WORK_QUEUE_UNPLUG, (uint32_t)(uintptr_t)queue)

#define sys_port_trace_k_work_queue_unplug_exit(queue, ret)                                        \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_WORK_QUEUE_UNPLUG, (uint32_t)ret)

#define sys_port_trace_k_work_delayable_init(dwork)                                                \
	SEGGER_SYSVIEW_RecordU32(TID_WORK_DELAYABLE_INIT, (uint32_t)(uintptr_t)dwork)

#define sys_port_trace_k_work_schedule_for_queue_enter(queue, dwork, delay)                        \
	SEGGER_SYSVIEW_RecordU32x3(TID_WORK_SCHEDULE_FOR_QUEUE, (uint32_t)(uintptr_t)queue,        \
				   (uint32_t)(uintptr_t)dwork, (uint32_t)delay.ticks)

#define sys_port_trace_k_work_schedule_for_queue_exit(queue, dwork, delay, ret)                    \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_WORK_SCHEDULE_FOR_QUEUE, (uint32_t)ret)

#define sys_port_trace_k_work_schedule_enter(dwork, delay)                                         \
	SEGGER_SYSVIEW_RecordU32x2(TID_WORK_SCHEDULE, (uint32_t)(uintptr_t)dwork,                  \
				   (uint32_t)delay.ticks)

#define sys_port_trace_k_work_schedule_exit(dwork, delay, ret)                                     \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_WORK_SCHEDULE, (uint32_t)ret)

#define sys_port_trace_k_work_reschedule_for_queue_enter(queue, dwork, delay)                      \
	SEGGER_SYSVIEW_RecordU32x3(TID_WORK_RESCHEDULE_FOR_QUEUE, (uint32_t)(uintptr_t)queue,      \
				   (uint32_t)(uintptr_t)dwork, (uint32_t)delay.ticks)

#define sys_port_trace_k_work_reschedule_for_queue_exit(queue, dwork, delay, ret)                  \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_WORK_RESCHEDULE_FOR_QUEUE, (uint32_t)ret)

#define sys_port_trace_k_work_reschedule_enter(dwork, delay)                                       \
	SEGGER_SYSVIEW_RecordU32x2(TID_WORK_RESCHEDULE, (uint32_t)(uintptr_t)dwork,                \
				   (uint32_t)delay.ticks)

#define sys_port_trace_k_work_reschedule_exit(dwork, delay, ret)                                   \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_WORK_RESCHEDULE, (uint32_t)ret)

#define sys_port_trace_k_work_flush_delayable_enter(dwork, sync)                                   \
	SEGGER_SYSVIEW_RecordU32x2(TID_WORK_FLUSH_DELAYABLE, (uint32_t)(uintptr_t)dwork,           \
				   (uint32_t)(uintptr_t)sync)

#define sys_port_trace_k_work_flush_delayable_exit(dwork, sync, ret)                               \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_WORK_FLUSH_DELAYABLE, (uint32_t)ret)

#define sys_port_trace_k_work_cancel_delayable_enter(dwork)                                        \
	SEGGER_SYSVIEW_RecordU32(TID_WORK_CANCEL_DELAYABLE, (uint32_t)(uintptr_t)dwork)

#define sys_port_trace_k_work_cancel_delayable_exit(dwork, ret)                                    \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_WORK_CANCEL_DELAYABLE, (uint32_t)ret)

#define sys_port_trace_k_work_cancel_delayable_sync_enter(dwork, sync)                             \
	SEGGER_SYSVIEW_RecordU32x2(TID_WORK_CANCEL_DELAYABLE_SYNC, (uint32_t)(uintptr_t)dwork,     \
				   (uint32_t)(uintptr_t)sync)

#define sys_port_trace_k_work_cancel_delayable_sync_exit(dwork, sync, ret)                         \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_WORK_CANCEL_DELAYABLE_SYNC, (uint32_t)ret)

#define sys_port_trace_k_work_poll_init_enter(work)                                                \
	SEGGER_SYSVIEW_RecordU32(TID_WORK_POLL_INIT, (uint32_t)(uintptr_t)work)

#define sys_port_trace_k_work_poll_init_exit(work) SEGGER_SYSVIEW_RecordEndCall(TID_WORK_POLL_INIT)

#define sys_port_trace_k_work_poll_submit_to_queue_enter(work_q, work, timeout)                    \
	SEGGER_SYSVIEW_RecordU32x3(TID_WORK_POLL_SUBMIT_TO_QUEUE, (uint32_t)(uintptr_t)work_q,     \
				   (uint32_t)(uintptr_t)work, (uint32_t)timeout.ticks)

#define sys_port_trace_k_work_poll_submit_to_queue_blocking(work_q, work, timeout)

#define sys_port_trace_k_work_poll_submit_to_queue_exit(work_q, work, timeout, ret)                \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_WORK_POLL_SUBMIT_TO_QUEUE, (uint32_t)ret)

#define sys_port_trace_k_work_poll_submit_enter(work, timeout)                                     \
	SEGGER_SYSVIEW_RecordU32x2(TID_WORK_POLL_SUBMIT, (uint32_t)(uintptr_t)work,                \
				   (uint32_t)timeout.ticks)

#define sys_port_trace_k_work_poll_submit_exit(work, timeout, ret)                                 \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_WORK_POLL_SUBMIT, (uint32_t)ret)

#define sys_port_trace_k_work_poll_cancel_enter(work)                                              \
	SEGGER_SYSVIEW_RecordU32(TID_WORK_POLL_CANCEL, (uint32_t)(uintptr_t)work)

#define sys_port_trace_k_work_poll_cancel_exit(work, ret)                                          \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_WORK_POLL_CANCEL, (uint32_t)ret)

#define sys_port_trace_k_poll_api_event_init(event)
#define sys_port_trace_k_poll_api_poll_enter(events)
#define sys_port_trace_k_poll_api_poll_exit(events, ret)
#define sys_port_trace_k_poll_api_signal_init(signal)
#define sys_port_trace_k_poll_api_signal_reset(signal)
#define sys_port_trace_k_poll_api_signal_check(signal)
#define sys_port_trace_k_poll_api_signal_raise(signal, ret)

#define sys_port_trace_k_sem_init(sem, ret)                                                        \
	SEGGER_SYSVIEW_RecordU32x2(TID_SEMA_INIT, (uint32_t)(uintptr_t)sem, (int32_t)ret)

#define sys_port_trace_k_sem_give_enter(sem)                                                       \
	SEGGER_SYSVIEW_RecordU32(TID_SEMA_GIVE, (uint32_t)(uintptr_t)sem)

#define sys_port_trace_k_sem_give_exit(sem) SEGGER_SYSVIEW_RecordEndCall(TID_SEMA_GIVE)

#define sys_port_trace_k_sem_take_enter(sem, timeout)                                              \
	SEGGER_SYSVIEW_RecordU32x2(TID_SEMA_TAKE, (uint32_t)(uintptr_t)sem, (uint32_t)timeout.ticks)

#define sys_port_trace_k_sem_take_blocking(sem, timeout)

#define sys_port_trace_k_sem_take_exit(sem, timeout, ret)                                          \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_SEMA_TAKE, (int32_t)ret)

#define sys_port_trace_k_sem_reset(sem)                                                            \
	SEGGER_SYSVIEW_RecordU32(TID_SEMA_RESET, (uint32_t)(uintptr_t)sem)

#define sys_port_trace_k_mutex_init(mutex, ret)                                                    \
	SEGGER_SYSVIEW_RecordU32x2(TID_MUTEX_INIT, (uint32_t)(uintptr_t)mutex, (int32_t)ret)

#define sys_port_trace_k_mutex_lock_enter(mutex, timeout)                                          \
	SEGGER_SYSVIEW_RecordU32x2(TID_MUTEX_LOCK, (uint32_t)(uintptr_t)mutex,                     \
				   (uint32_t)timeout.ticks)

#define sys_port_trace_k_mutex_lock_blocking(mutex, timeout)

#define sys_port_trace_k_mutex_lock_exit(mutex, timeout, ret)                                      \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_MUTEX_LOCK, (int32_t)ret)

#define sys_port_trace_k_mutex_unlock_enter(mutex)                                                 \
	SEGGER_SYSVIEW_RecordU32(TID_MUTEX_UNLOCK, (uint32_t)(uintptr_t)mutex)

#define sys_port_trace_k_mutex_unlock_exit(mutex, ret)                                             \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_MUTEX_UNLOCK, (uint32_t)ret)

#define sys_port_trace_k_condvar_init(condvar, ret)                                                \
	SEGGER_SYSVIEW_RecordU32(TID_CONDVAR_INIT, (uint32_t)(uintptr_t)condvar)

#define sys_port_trace_k_condvar_signal_enter(condvar)                                             \
	SEGGER_SYSVIEW_RecordU32(TID_CONDVAR_SIGNAL, (uint32_t)(uintptr_t)condvar)

#define sys_port_trace_k_condvar_signal_blocking(condvar, timeout)

#define sys_port_trace_k_condvar_signal_exit(condvar, ret)                                         \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_CONDVAR_SIGNAL, (uint32_t)ret)

#define sys_port_trace_k_condvar_broadcast_enter(condvar)                                          \
	SEGGER_SYSVIEW_RecordU32(TID_CONDVAR_BROADCAST, (uint32_t)(uintptr_t)condvar)

#define sys_port_trace_k_condvar_broadcast_exit(condvar, ret)                                      \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_CONDVAR_BROADCAST, (uint32_t)ret)

#define sys_port_trace_k_condvar_wait_enter(condvar)                                               \
	SEGGER_SYSVIEW_RecordU32(TID_CONDVAR_WAIT, (uint32_t)(uintptr_t)condvar)

#define sys_port_trace_k_condvar_wait_exit(condvar, ret)                                           \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_CONDVAR_WAIT, (uint32_t)ret)

#define sys_port_trace_k_queue_init(queue)                                                         \
	SEGGER_SYSVIEW_RecordU32(TID_QUEUE_INIT, (uint32_t)(uintptr_t)queue)

#define sys_port_trace_k_queue_cancel_wait(queue)                                                  \
	SEGGER_SYSVIEW_RecordU32(TID_QUEUE_CANCEL_WAIT, (uint32_t)(uintptr_t)queue)

#define sys_port_trace_k_queue_queue_insert_enter(queue, alloc)
#define sys_port_trace_k_queue_queue_insert_blocking(queue, alloc, timeout)
#define sys_port_trace_k_queue_queue_insert_exit(queue, alloc, ret)

#define sys_port_trace_k_queue_append_enter(queue)                                                 \
	SEGGER_SYSVIEW_RecordU32(TID_QUEUE_APPEND, (uint32_t)(uintptr_t)queue)

#define sys_port_trace_k_queue_append_exit(queue) SEGGER_SYSVIEW_RecordEndCall(TID_QUEUE_APPEND)

#define sys_port_trace_k_queue_alloc_append_enter(queue)                                           \
	SEGGER_SYSVIEW_RecordU32(TID_QUEUE_ALLOC_APPEND, (uint32_t)(uintptr_t)queue)

#define sys_port_trace_k_queue_alloc_append_exit(queue, ret)                                       \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_QUEUE_ALLOC_APPEND, (uint32_t)ret)

#define sys_port_trace_k_queue_prepend_enter(queue)                                                \
	SEGGER_SYSVIEW_RecordU32(TID_QUEUE_PREPEND, (uint32_t)(uintptr_t)queue)

#define sys_port_trace_k_queue_prepend_exit(queue) SEGGER_SYSVIEW_RecordEndCall(TID_QUEUE_PREPEND)

#define sys_port_trace_k_queue_alloc_prepend_enter(queue)                                          \
	SEGGER_SYSVIEW_RecordU32(TID_QUEUE_ALLOC_PREPEND, (uint32_t)(uintptr_t)queue)

#define sys_port_trace_k_queue_alloc_prepend_exit(queue, ret)                                      \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_QUEUE_ALLOC_PREPEND, (uint32_t)ret)

#define sys_port_trace_k_queue_insert_enter(queue)                                                 \
	SEGGER_SYSVIEW_RecordU32(TID_QUEUE_INSERT, (uint32_t)(uintptr_t)queue)

#define sys_port_trace_k_queue_insert_blocking(queue, timeout)

#define sys_port_trace_k_queue_insert_exit(queue) SEGGER_SYSVIEW_RecordEndCall(TID_QUEUE_INSERT)

#define sys_port_trace_k_queue_append_list_enter(queue)                                            \
	SEGGER_SYSVIEW_RecordU32(TID_QUEUE_APPEND_LIST, (uint32_t)(uintptr_t)queue)

#define sys_port_trace_k_queue_append_list_exit(queue, ret)                                        \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_QUEUE_APPEND_LIST, (uint32_t)ret)

#define sys_port_trace_k_queue_merge_slist_enter(queue)
#define sys_port_trace_k_queue_merge_slist_exit(queue, ret)

#define sys_port_trace_k_queue_get_enter(queue, timeout)                                           \
	SEGGER_SYSVIEW_RecordU32x2(TID_QUEUE_GET, (uint32_t)(uintptr_t)queue,                      \
				   (uint32_t)timeout.ticks)

#define sys_port_trace_k_queue_get_blocking(queue, timeout)

#define sys_port_trace_k_queue_get_exit(queue, timeout, data)                                      \
	SEGGER_SYSVIEW_RecordEndCall(TID_QUEUE_GET)

#define sys_port_trace_k_queue_remove_enter(queue)                                                 \
	SEGGER_SYSVIEW_RecordU32(TID_QUEUE_REMOVE, (uint32_t)(uintptr_t)queue)

#define sys_port_trace_k_queue_remove_exit(queue, ret)                                             \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_QUEUE_REMOVE, (uint32_t)ret)

#define sys_port_trace_k_queue_unique_append_enter(queue)
#define sys_port_trace_k_queue_unique_append_exit(queue, ret)

#define sys_port_trace_k_queue_peek_head(queue, ret)                                               \
	SEGGER_SYSVIEW_RecordU32x2(TID_QUEUE_PEAK_HEAD, (uint32_t)(uintptr_t)queue, (int32_t)ret)

#define sys_port_trace_k_queue_peek_tail(queue, ret)                                               \
	SEGGER_SYSVIEW_RecordU32x2(TID_QUEUE_PEAK_TAIL, (uint32_t)(uintptr_t)queue, (int32_t)ret)

#define sys_port_trace_k_fifo_init_enter(fifo)                                                     \
	SEGGER_SYSVIEW_RecordU32(TID_FIFO_INIT, (uint32_t)(uintptr_t)fifo)
#define sys_port_trace_k_fifo_init_exit(fifo) SEGGER_SYSVIEW_RecordEndCall(TID_FIFO_INIT)

#define sys_port_trace_k_fifo_cancel_wait_enter(fifo)                                              \
	SEGGER_SYSVIEW_RecordU32(TID_FIFO_CANCEL_WAIT, (uint32_t)(uintptr_t)fifo)
#define sys_port_trace_k_fifo_cancel_wait_exit(fifo)                                               \
	SEGGER_SYSVIEW_RecordEndCall(TID_FIFO_CANCEL_WAIT)

#define sys_port_trace_k_fifo_put_enter(fifo, data)                                                \
	SEGGER_SYSVIEW_RecordU32x2(TID_FIFO_PUT, (uint32_t)(uintptr_t)fifo,                        \
				   (uint32_t)(uintptr_t)data)

#define sys_port_trace_k_fifo_put_exit(fifo, data) SEGGER_SYSVIEW_RecordEndCall(TID_FIFO_PUT)

#define sys_port_trace_k_fifo_alloc_put_enter(fifo, data)                                          \
	SEGGER_SYSVIEW_RecordU32x2(TID_FIFO_ALLOC_PUT, (uint32_t)(uintptr_t)fifo,                  \
				   (uint32_t)(uintptr_t)data)
#define sys_port_trace_k_fifo_alloc_put_exit(fifo, data, ret)                                      \
	SEGGER_SYSVIEW_RecordEndCall(TID_FIFO_ALLOC_PUT)

#define sys_port_trace_k_fifo_put_list_enter(fifo, head, tail)                                     \
	SEGGER_SYSVIEW_RecordU32x3(TID_FIFO_PUT_LIST, (uint32_t)(uintptr_t)fifo,                   \
				   (uint32_t)(uintptr_t)head, (uint32_t)(uintptr_t)tail)

#define sys_port_trace_k_fifo_put_list_exit(fifo, head, tail)                                      \
	SEGGER_SYSVIEW_RecordEndCall(TID_FIFO_PUT_LIST)

#define sys_port_trace_k_fifo_put_slist_enter(fifo, list)                                          \
	SEGGER_SYSVIEW_RecordU32x2(TID_FIFO_PUT_SLIST, (uint32_t)(uintptr_t)fifo,                  \
				   (uint32_t)(uintptr_t)list)
#define sys_port_trace_k_fifo_put_slist_exit(fifo, list)                                           \
	SEGGER_SYSVIEW_RecordEndCall(TID_FIFO_PUT_SLIST)

#define sys_port_trace_k_fifo_get_enter(fifo, timeout)                                             \
	SEGGER_SYSVIEW_RecordU32x2(TID_FIFO_GET, (uint32_t)(uintptr_t)fifo, (uint32_t)timeout.ticks)

#define sys_port_trace_k_fifo_get_exit(fifo, timeout, ret)                                         \
	SEGGER_SYSVIEW_RecordEndCall(TID_FIFO_GET)

#define sys_port_trace_k_fifo_peek_head_enter(fifo)                                                \
	SEGGER_SYSVIEW_RecordU32(TID_FIFO_PEAK_HEAD, (uint32_t)(uintptr_t)fifo)

#define sys_port_trace_k_fifo_peek_head_exit(fifo, ret)                                            \
	SEGGER_SYSVIEW_RecordEndCall(TID_FIFO_PEAK_HEAD)

#define sys_port_trace_k_fifo_peek_tail_enter(fifo)                                                \
	SEGGER_SYSVIEW_RecordU32(TID_FIFO_PEAK_TAIL, (uint32_t)(uintptr_t)fifo)
#define sys_port_trace_k_fifo_peek_tail_exit(fifo, ret)                                            \
	SEGGER_SYSVIEW_RecordEndCall(TID_FIFO_PEAK_TAIL)

#define sys_port_trace_k_lifo_init_enter(lifo)                                                     \
	SEGGER_SYSVIEW_RecordU32(TID_LIFO_INIT, (uint32_t)(uintptr_t)lifo)

#define sys_port_trace_k_lifo_init_exit(lifo) SEGGER_SYSVIEW_RecordEndCall(TID_LIFO_INIT)

#define sys_port_trace_k_lifo_put_enter(lifo, data)                                                \
	SEGGER_SYSVIEW_RecordU32x2(TID_LIFO_PUT, (uint32_t)(uintptr_t)lifo,                        \
				   (uint32_t)(uintptr_t)data)

#define sys_port_trace_k_lifo_put_exit(lifo, data) SEGGER_SYSVIEW_RecordEndCall(TID_LIFO_PUT)

#define sys_port_trace_k_lifo_alloc_put_enter(lifo, data)                                          \
	SEGGER_SYSVIEW_RecordU32x2(TID_LIFO_ALLOC_PUT, (uint32_t)(uintptr_t)lifo,                  \
				   (uint32_t)(uintptr_t)data)
#define sys_port_trace_k_lifo_alloc_put_exit(lifo, data, ret)                                      \
	SEGGER_SYSVIEW_RecordEndCall(TID_LIFO_ALLOC_PUT)

#define sys_port_trace_k_lifo_get_enter(lifo, timeout)                                             \
	SEGGER_SYSVIEW_RecordU32x2(TID_LIFO_GET, (uint32_t)(uintptr_t)lifo, (uint32_t)timeout.ticks)
#define sys_port_trace_k_lifo_get_exit(lifo, timeout, ret)                                         \
	SEGGER_SYSVIEW_RecordEndCall(TID_LIFO_GET)

#define sys_port_trace_k_stack_init(stack)
#define sys_port_trace_k_stack_alloc_init_enter(stack)
#define sys_port_trace_k_stack_alloc_init_exit(stack, ret)
#define sys_port_trace_k_stack_cleanup_enter(stack)
#define sys_port_trace_k_stack_cleanup_exit(stack, ret)
#define sys_port_trace_k_stack_push_enter(stack)
#define sys_port_trace_k_stack_push_exit(stack, ret)
#define sys_port_trace_k_stack_pop_enter(stack, timeout)
#define sys_port_trace_k_stack_pop_blocking(stack, timeout)
#define sys_port_trace_k_stack_pop_exit(stack, timeout, ret)

#define sys_port_trace_k_msgq_init(msgq)
#define sys_port_trace_k_msgq_alloc_init_enter(msgq)
#define sys_port_trace_k_msgq_alloc_init_exit(msgq, ret)
#define sys_port_trace_k_msgq_cleanup_enter(msgq)
#define sys_port_trace_k_msgq_cleanup_exit(msgq, ret)
#define sys_port_trace_k_msgq_put_enter(msgq, timeout)
#define sys_port_trace_k_msgq_put_blocking(msgq, timeout)
#define sys_port_trace_k_msgq_put_exit(msgq, timeout, ret)
#define sys_port_trace_k_msgq_get_enter(msgq, timeout)
#define sys_port_trace_k_msgq_get_blocking(msgq, timeout)
#define sys_port_trace_k_msgq_get_exit(msgq, timeout, ret)
#define sys_port_trace_k_msgq_peek(msgq, ret)
#define sys_port_trace_k_msgq_purge(msgq)

#define sys_port_trace_k_mbox_init(mbox)
#define sys_port_trace_k_mbox_message_put_enter(mbox, timeout)
#define sys_port_trace_k_mbox_message_put_blocking(mbox, timeout)
#define sys_port_trace_k_mbox_message_put_exit(mbox, timeout, ret)
#define sys_port_trace_k_mbox_put_enter(mbox, timeout)
#define sys_port_trace_k_mbox_put_exit(mbox, timeout, ret)
#define sys_port_trace_k_mbox_async_put_enter(mbox, sem)
#define sys_port_trace_k_mbox_async_put_exit(mbox, sem)
#define sys_port_trace_k_mbox_get_enter(mbox, timeout)
#define sys_port_trace_k_mbox_get_blocking(mbox, timeout)
#define sys_port_trace_k_mbox_get_exit(mbox, timeout, ret)
#define sys_port_trace_k_mbox_data_get(rx_msg)

#define sys_port_trace_k_pipe_init(pipe)
#define sys_port_trace_k_pipe_cleanup_enter(pipe)
#define sys_port_trace_k_pipe_cleanup_exit(pipe, ret)
#define sys_port_trace_k_pipe_alloc_init_enter(pipe)
#define sys_port_trace_k_pipe_alloc_init_exit(pipe, ret)
#define sys_port_trace_k_pipe_flush_enter(pipe)
#define sys_port_trace_k_pipe_flush_exit(pipe)
#define sys_port_trace_k_pipe_buffer_flush_enter(pipe)
#define sys_port_trace_k_pipe_buffer_flush_exit(pipe)
#define sys_port_trace_k_pipe_put_enter(pipe, timeout)
#define sys_port_trace_k_pipe_put_blocking(pipe, timeout)
#define sys_port_trace_k_pipe_put_exit(pipe, timeout, ret)
#define sys_port_trace_k_pipe_get_enter(pipe, timeout)
#define sys_port_trace_k_pipe_get_blocking(pipe, timeout)
#define sys_port_trace_k_pipe_get_exit(pipe, timeout, ret)

#define sys_port_trace_k_event_init(event)
#define sys_port_trace_k_event_post_enter(event, events, events_mask)
#define sys_port_trace_k_event_post_exit(event, events, events_mask)
#define sys_port_trace_k_event_wait_enter(event, events, options, timeout)
#define sys_port_trace_k_event_wait_blocking(event, events, options, timeout)
#define sys_port_trace_k_event_wait_exit(event, events, ret)

#define sys_port_trace_k_heap_init(heap)                                                           \
	SEGGER_SYSVIEW_RecordU32(TID_HEAP_INIT, (uint32_t)(uintptr_t)heap)

#define sys_port_trace_k_heap_aligned_alloc_enter(heap, timeout)                                   \
	SEGGER_SYSVIEW_RecordU32x2(TID_HEAP_ALIGNED_ALLOC, (uint32_t)(uintptr_t)heap,              \
				   (uint32_t)timeout.ticks)

#define sys_port_trace_k_heap_aligned_alloc_blocking(heap, timeout)

#define sys_port_trace_k_heap_aligned_alloc_exit(heap, timeout, ret)                               \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_HEAP_ALIGNED_ALLOC, (uint32_t)ret)

#define sys_port_trace_k_heap_alloc_enter(heap, timeout)                                           \
	SEGGER_SYSVIEW_RecordU32x2(TID_HEAP_ALLOC, (uint32_t)(uintptr_t)heap,                      \
				   (uint32_t)timeout.ticks)

#define sys_port_trace_k_heap_alloc_exit(heap, timeout, ret)                                       \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_HEAP_ALLOC, (uint32_t)ret)

#define sys_port_trace_k_heap_free(heap)                                                           \
	SEGGER_SYSVIEW_RecordU32(TID_HEAP_FREE, (uint32_t)(uintptr_t)heap)

#define sys_port_trace_k_heap_sys_k_aligned_alloc_enter(heap)
#define sys_port_trace_k_heap_sys_k_aligned_alloc_exit(heap, ret)
#define sys_port_trace_k_heap_sys_k_malloc_enter(heap)
#define sys_port_trace_k_heap_sys_k_malloc_exit(heap, ret)
#define sys_port_trace_k_heap_sys_k_free_enter(heap, heap_ref)
#define sys_port_trace_k_heap_sys_k_free_exit(heap, heap_ref)
#define sys_port_trace_k_heap_sys_k_calloc_enter(heap)
#define sys_port_trace_k_heap_sys_k_calloc_exit(heap, ret)

#define sys_port_trace_k_mem_slab_init(slab, rc)                                                   \
	SEGGER_SYSVIEW_RecordU32(TID_MSLAB_INIT, (uint32_t)(uintptr_t)slab)

#define sys_port_trace_k_mem_slab_alloc_enter(slab, timeout)                                       \
	SEGGER_SYSVIEW_RecordU32x2(TID_MSLAB_ALLOC, (uint32_t)(uintptr_t)slab,                     \
				   (uint32_t)timeout.ticks)

#define sys_port_trace_k_mem_slab_alloc_blocking(slab, timeout)
#define sys_port_trace_k_mem_slab_alloc_exit(slab, timeout, ret)                                   \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_MSLAB_ALLOC, (uint32_t)ret)

#define sys_port_trace_k_mem_slab_free_enter(slab)                                                 \
	SEGGER_SYSVIEW_RecordU32(TID_MSLAB_FREE, (uint32_t)(uintptr_t)slab)

#define sys_port_trace_k_mem_slab_free_exit(slab) SEGGER_SYSVIEW_RecordEndCall(TID_MSLAB_ALLOC)

#define sys_port_trace_k_timer_init(timer)                                                         \
	SEGGER_SYSVIEW_RecordU32(TID_TIMER_INIT, (uint32_t)(uintptr_t)timer)

#define sys_port_trace_k_timer_start(timer, duration, period)					   \
	SEGGER_SYSVIEW_RecordU32x3(TID_TIMER_START, (uint32_t)(uintptr_t)timer,			   \
			(uint32_t)duration.ticks, (uint32_t)period.ticks)

#define sys_port_trace_k_timer_stop(timer)                                                         \
	SEGGER_SYSVIEW_RecordU32(TID_TIMER_STOP, (uint32_t)(uintptr_t)timer)

#define sys_port_trace_k_timer_status_sync_enter(timer)                                            \
	SEGGER_SYSVIEW_RecordU32(TID_TIMER_STATUS_SYNC, (uint32_t)(uintptr_t)timer)

#define sys_port_trace_k_timer_status_sync_blocking(timer, timeout)

#define sys_port_trace_k_timer_status_sync_exit(timer, result)                                     \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_TIMER_STATUS_SYNC, (uint32_t)result)

#define sys_port_trace_syscall_enter(id, name, ...)	\
	SEGGER_SYSVIEW_RecordString(TID_SYSCALL, (const char *)#name)

#define sys_port_trace_syscall_exit(id, name, ...)	\
	SEGGER_SYSVIEW_RecordEndCall(TID_SYSCALL)

void sys_trace_idle(void);

void sys_trace_k_thread_create(struct k_thread *new_thread, size_t stack_size, int prio);
void sys_trace_k_thread_user_mode_enter(k_thread_entry_t entry, void *p1, void *p2, void *p3);

void sys_trace_k_thread_join_blocking(struct k_thread *thread, k_timeout_t timeout);
void sys_trace_k_thread_join_exit(struct k_thread *thread, k_timeout_t timeout, int ret);

void sys_trace_k_thread_switched_out(void);
void sys_trace_k_thread_switched_in(void);
void sys_trace_k_thread_ready(struct k_thread *thread);
void sys_trace_k_thread_pend(struct k_thread *thread);
void sys_trace_k_thread_info(struct k_thread *thread);

#define sys_port_trace_pm_system_suspend_enter(ticks)			       \
	SEGGER_SYSVIEW_RecordU32(TID_PM_SYSTEM_SUSPEND, (uint32_t)ticks)
#define sys_port_trace_pm_system_suspend_exit(ticks, state)		       \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_PM_SYSTEM_SUSPEND, (uint32_t)state)

#define sys_port_trace_pm_device_runtime_get_enter(dev)			       \
	SEGGER_SYSVIEW_RecordU32(TID_PM_DEVICE_RUNTIME_GET,		       \
				 (uint32_t)(uintptr_t)dev)
#define sys_port_trace_pm_device_runtime_get_exit(dev, ret)		       \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_PM_DEVICE_RUNTIME_GET,	       \
					(uint32_t)ret)
#define sys_port_trace_pm_device_runtime_put_enter(dev)			       \
	SEGGER_SYSVIEW_RecordU32(TID_PM_DEVICE_RUNTIME_PUT,		       \
				 (uint32_t)(uintptr_t)dev)
#define sys_port_trace_pm_device_runtime_put_exit(dev, ret)		       \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_PM_DEVICE_RUNTIME_PUT,	       \
					(uint32_t)ret)
#define sys_port_trace_pm_device_runtime_put_async_enter(dev, delay)	       \
	SEGGER_SYSVIEW_RecordU32x2(TID_PM_DEVICE_RUNTIME_PUT_ASYNC,	       \
			 (uint32_t)(uintptr_t)dev, (uint32_t)delay.ticks)
#define sys_port_trace_pm_device_runtime_put_async_exit(dev, delay, ret)       \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_PM_DEVICE_RUNTIME_PUT_ASYNC,       \
					(uint32_t)ret)
#define sys_port_trace_pm_device_runtime_enable_enter(dev)		       \
	SEGGER_SYSVIEW_RecordU32(TID_PM_DEVICE_RUNTIME_ENABLE,		       \
				 (uint32_t)(uintptr_t)dev)
#define sys_port_trace_pm_device_runtime_enable_exit(dev, ret)		       \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_PM_DEVICE_RUNTIME_ENABLE,	       \
				     (uint32_t)ret)
#define sys_port_trace_pm_device_runtime_disable_enter(dev)		       \
	SEGGER_SYSVIEW_RecordU32(TID_PM_DEVICE_RUNTIME_DISABLE,		       \
				 (uint32_t)(uintptr_t)dev)
#define sys_port_trace_pm_device_runtime_disable_exit(dev, ret)		       \
	SEGGER_SYSVIEW_RecordEndCallU32(TID_PM_DEVICE_RUNTIME_DISABLE,	       \
					(uint32_t)ret)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TRACE_SYSVIEW_H */
