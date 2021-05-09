/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_TRACE_SYSVIEW_H
#define ZEPHYR_TRACE_SYSVIEW_H
#include <string.h>
#include <kernel.h>
#include <init.h>

#include <SEGGER_SYSVIEW.h>


#ifdef __cplusplus
extern "C" {
#endif

#define SYS_TRACE_ID_OFFSET (32u)

#define SYS_TRACE_ID_MUTEX_INIT (1u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_MUTEX_UNLOCK (2u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_MUTEX_LOCK (3u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_SEMA_INIT (4u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_SEMA_GIVE (5u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_SEMA_TAKE (6u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_SLEEP (7u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_SEMA_RESET (8u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_PRIORITY_SET (9u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_THREAD_WAKEUP (10u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_THREAD_ABORT (11u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_THREAD_START (12u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_THREAD_SUSPEND (13u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_THREAD_RESUME (14u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_SEMA_BLOCKING (15u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_THREAD_JOIN (16u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_MSLEEP (17u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_USLEEP (18u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_BUSYWAIT (19u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_THREAD_FOREACH (20u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_THREAD_FOREACH_UNLOCKED (21u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_THREAD_YIELD (22u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_THREAD_USERMODE_ENTER (23u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_CONDVAR_INIT (24u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_CONDVAR_SIGNAL (25u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_CONDVAR_BROADCAST (26u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_CONDVAR_WAIT (27u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_CANCEL (28u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_CANCEL_DELAYABLE (29u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_CANCEL_DELAYABLE_SYNC (30u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_CANCEL_SYNC (31u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_DELAYABLE_INIT (32u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_QUEUE_DRAIN (33u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_FLUSH (34u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_FLUSH_DELAYABLE (35u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_INIT (36u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_POLL_CANCEL (37u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_POLL_INIT (38u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_POLL_SUBMIT (39u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_POLL_SUBMIT_TO_QUEUE (40u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_QUEUE_START (41u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_RESCHEDULE (42u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_RESCHEDULE_FOR_QUEUE (43u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_SCHEDULE (44u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_SCHEDULE_FOR_QUEUE (45u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_SUBMIT (46u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_SUBMIT_TO_QUEUE (47u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_WORK_QUEUE_UNPLUG (48u + SYS_TRACE_ID_OFFSET)

void sys_trace_thread_info(struct k_thread *thread);

#define sys_port_trace_k_thread_foreach_enter()                                                    \
	SEGGER_SYSVIEW_RecordVoid(SYS_TRACE_ID_THREAD_FOREACH)

#define sys_port_trace_k_thread_foreach_exit()                                                     \
	SEGGER_SYSVIEW_RecordEndCall(SYS_TRACE_ID_THREAD_FOREACH)

#define sys_port_trace_k_thread_foreach_unlocked_enter()                                           \
	SEGGER_SYSVIEW_RecordVoid(SYS_TRACE_ID_THREAD_FOREACH_UNLOCKED)

#define sys_port_trace_k_thread_foreach_unlocked_exit()                                            \
	SEGGER_SYSVIEW_RecordEndCall(SYS_TRACE_ID_THREAD_FOREACH_UNLOCKED)

#define sys_port_trace_k_thread_create(new_thread)                                                 \
	do {                                                                                       \
		SEGGER_SYSVIEW_OnTaskCreate((uint32_t)(uintptr_t)new_thread);                      \
		sys_trace_thread_info(new_thread);                                                 \
	} while (0)

#define sys_port_trace_k_thread_user_mode_enter()                                                  \
	SEGGER_SYSVIEW_RecordVoid(SYS_TRACE_ID_THREAD_USERMODE_ENTER)

#define sys_port_trace_k_thread_heap_assign(thread, heap)
#define sys_port_trace_k_thread_join_enter(thread, timeout)                                        \
	SEGGER_SYSVIEW_RecordU32x2(SYS_TRACE_ID_THREAD_JOIN, (uint32_t)(uintptr_t)thread,          \
				   k_ticks_to_ms_floor32((uint32_t)timeout.ticks))
#define sys_port_trace_k_thread_join_blocking(thread, timeout)
#define sys_port_trace_k_thread_join_exit(thread, timeout, ret)                                    \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_THREAD_JOIN, (int32_t)ret)

#define sys_port_trace_k_thread_sleep_enter(timeout)                                               \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_SLEEP, k_ticks_to_ms_floor32((uint32_t)timeout.ticks))

#define sys_port_trace_k_thread_sleep_exit(timeout, ret)                                           \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_SLEEP, (int32_t)ret)

#define sys_port_trace_k_thread_msleep_enter(ms)                                                   \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_MSLEEP, (uint32_t)ms)

#define sys_port_trace_k_thread_msleep_exit(ms, ret)                                               \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_MSLEEP, (int32_t)ret)

#define sys_port_trace_k_thread_usleep_enter(us)                                                   \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_USLEEP, (uint32_t)us)

#define sys_port_trace_k_thread_usleep_exit(us, ret)                                               \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_USLEEP, (int32_t)ret)

#define sys_port_trace_k_thread_busy_wait_enter(usec_to_wait)                                      \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_BUSYWAIT, (uint32_t)usec_to_wait)

#define sys_port_trace_k_thread_busy_wait_exit(usec_to_wait)                                       \
	SEGGER_SYSVIEW_RecordEndCall(SYS_TRACE_ID_BUSYWAIT)

#define sys_port_trace_k_thread_yield() SEGGER_SYSVIEW_RecordVoid(SYS_TRACE_ID_THREAD_YIELD)

#define sys_port_trace_k_thread_wakeup(thread)                                                     \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_THREAD_WAKEUP, (uint32_t)(uintptr_t)thread)

#define sys_port_trace_k_thread_start(thread)                                                      \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_THREAD_START, (uint32_t)(uintptr_t)thread)

#define sys_port_trace_k_thread_abort(thread)                                                      \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_THREAD_ABORT, (uint32_t)(uintptr_t)thread)

#define sys_port_trace_k_thread_suspend_enter(thread)                                              \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_THREAD_SUSPEND, (uint32_t)(uintptr_t)thread)

#define sys_port_trace_k_thread_suspend_exit(thread)                                               \
	SEGGER_SYSVIEW_RecordEndCall(SYS_TRACE_ID_THREAD_SUSPEND)

#define sys_port_trace_k_thread_resume_enter(thread)                                               \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_THREAD_RESUME, (uint32_t)(uintptr_t)thread)

#define sys_port_trace_k_thread_sched_lock()

#define sys_port_trace_k_thread_sched_unlock()

#define sys_port_trace_k_thread_name_set(thread, ret)

#define sys_port_trace_k_thread_switched_out() sys_trace_k_thread_switched_out()

#define sys_port_trace_k_thread_switched_in() sys_trace_k_thread_switched_in()

#define sys_port_trace_k_thread_info(thread) sys_trace_k_thread_info(thread)

#define sys_port_trace_k_thread_sched_wakeup(thread)                                               \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_THREAD_WAKEUP, (uint32_t)(uintptr_t)thread)

#define sys_port_trace_k_thread_sched_abort(thread)                                                \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_THREAD_ABORT, (uint32_t)(uintptr_t)thread)

#define sys_port_trace_k_thread_sched_priority_set(thread, prio)                                   \
	SEGGER_SYSVIEW_RecordU32x2(SYS_TRACE_ID_PRIORITY_SET,                                      \
				   SEGGER_SYSVIEW_ShrinkId((uint32_t)thread), prio);

#define sys_port_trace_k_thread_sched_ready(thread)                                                \
	SEGGER_SYSVIEW_OnTaskStartReady((uint32_t)(uintptr_t)thread)

#define sys_port_trace_k_thread_sched_pend(thread)                                                 \
	SEGGER_SYSVIEW_OnTaskStopReady((uint32_t)(uintptr_t)thread, 3 << 3)

#define sys_port_trace_k_thread_sched_resume(thread)

#define sys_port_trace_k_thread_sched_suspend(thread)                                              \
	SEGGER_SYSVIEW_OnTaskStopReady((uint32_t)(uintptr_t)thread, 3 << 3)

#define sys_port_trace_k_work_init(work)                                                           \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_WORK_INIT, (uint32_t)(uintptr_t)work)

#define sys_port_trace_k_work_submit_to_queue_enter(queue, work)                                   \
	SEGGER_SYSVIEW_RecordU32x2(SYS_TRACE_ID_WORK_SUBMIT_TO_QUEUE, (uint32_t)(uintptr_t)queue,  \
				   (uint32_t)(uintptr_t)work)

#define sys_port_trace_k_work_submit_to_queue_exit(queue, work, ret)                               \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_WORK_SUBMIT_TO_QUEUE, (uint32_t)ret)

#define sys_port_trace_k_work_submit_enter(work)                                                   \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_WORK_SUBMIT, (uint32_t)(uintptr_t)work)

#define sys_port_trace_k_work_submit_exit(work, ret)                                               \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_WORK_SUBMIT, (uint32_t)ret)

#define sys_port_trace_k_work_flush_enter(work)                                                    \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_WORK_FLUSH, (uint32_t)(uintptr_t)work)

#define sys_port_trace_k_work_flush_blocking(work, timeout)

#define sys_port_trace_k_work_flush_exit(work, ret)                                                \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_WORK_FLUSH, (uint32_t)ret)

#define sys_port_trace_k_work_cancel_enter(work)                                                   \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_WORK_CANCEL, (uint32_t)(uintptr_t)work)

#define sys_port_trace_k_work_cancel_exit(work, ret)                                               \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_WORK_CANCEL, (uint32_t)ret)

#define sys_port_trace_k_work_cancel_sync_enter(work, sync)                                        \
	SEGGER_SYSVIEW_RecordU32x2(SYS_TRACE_ID_WORK_CANCEL_SYNC, (uint32_t)(uintptr_t)work,       \
				   (uint32_t)(uintptr_t)sync)

#define sys_port_trace_k_work_cancel_sync_blocking(work, sync)

#define sys_port_trace_k_work_cancel_sync_exit(work, sync, ret)                                    \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_WORK_CANCEL_SYNC, (uint32_t)ret)

#define sys_port_trace_k_work_queue_start_enter(queue)                                             \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_WORK_QUEUE_START, (uint32_t)(uintptr_t)queue)

#define sys_port_trace_k_work_queue_start_exit(queue)                                              \
	SEGGER_SYSVIEW_RecordEndCall(SYS_TRACE_ID_WORK_QUEUE_START)

#define sys_port_trace_k_work_queue_drain_enter(queue)                                             \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_WORK_QUEUE_DRAIN, (uint32_t)(uintptr_t)queue)

#define sys_port_trace_k_work_queue_drain_exit(queue, ret)                                         \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_WORK_QUEUE_DRAIN, (uint32_t)ret)

#define sys_port_trace_k_work_queue_unplug_enter(queue)                                            \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_WORK_QUEUE_UNPLUG, (uint32_t)(uintptr_t)queue)

#define sys_port_trace_k_work_queue_unplug_exit(queue, ret)                                        \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_WORK_QUEUE_UNPLUG, (uint32_t)ret)

#define sys_port_trace_k_work_delayable_init(dwork)                                                \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_WORK_DELAYABLE_INIT, (uint32_t)(uintptr_t)dwork)

#define sys_port_trace_k_work_schedule_for_queue_enter(queue, dwork, delay)                        \
	SEGGER_SYSVIEW_RecordU32x3(SYS_TRACE_ID_WORK_SCHEDULE_FOR_QUEUE,                           \
				   (uint32_t)(uintptr_t)queue, (uint32_t)(uintptr_t)dwork,         \
				   k_ticks_to_ms_floor32((uint32_t)delay.ticks))

#define sys_port_trace_k_work_schedule_for_queue_exit(queue, dwork, delay, ret)                    \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_WORK_SCHEDULE_FOR_QUEUE, (uint32_t)ret)

#define sys_port_trace_k_work_schedule_enter(dwork, delay)                                         \
	SEGGER_SYSVIEW_RecordU32x2(SYS_TRACE_ID_WORK_SCHEDULE, (uint32_t)(uintptr_t)dwork,         \
				   k_ticks_to_ms_floor32((uint32_t)delay.ticks))

#define sys_port_trace_k_work_schedule_exit(dwork, delay, ret)                                     \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_WORK_SCHEDULE, (uint32_t)ret)

#define sys_port_trace_k_work_reschedule_for_queue_enter(queue, dwork, delay)                      \
	SEGGER_SYSVIEW_RecordU32x3(SYS_TRACE_ID_WORK_RESCHEDULE_FOR_QUEUE,                         \
				   (uint32_t)(uintptr_t)queue, (uint32_t)(uintptr_t)dwork,         \
				   k_ticks_to_ms_floor32((uint32_t)delay.ticks))

#define sys_port_trace_k_work_reschedule_for_queue_exit(queue, dwork, delay, ret)                  \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_WORK_RESCHEDULE_FOR_QUEUE, (uint32_t)ret)

#define sys_port_trace_k_work_reschedule_enter(dwork, delay)                                       \
	SEGGER_SYSVIEW_RecordU32x2(SYS_TRACE_ID_WORK_RESCHEDULE, (uint32_t)(uintptr_t)dwork,       \
				   k_ticks_to_ms_floor32((uint32_t)delay.ticks))

#define sys_port_trace_k_work_reschedule_exit(dwork, delay, ret)                                   \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_WORK_RESCHEDULE, (uint32_t)ret)

#define sys_port_trace_k_work_flush_delayable_enter(dwork, sync)                                   \
	SEGGER_SYSVIEW_RecordU32x2(SYS_TRACE_ID_WORK_FLUSH_DELAYABLE, (uint32_t)(uintptr_t)dwork,  \
				   (uint32_t)(uintptr_t)sync)

#define sys_port_trace_k_work_flush_delayable_exit(dwork, sync, ret)                               \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_WORK_FLUSH_DELAYABLE, (uint32_t)ret)

#define sys_port_trace_k_work_cancel_delayable_enter(dwork)                                        \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_WORK_CANCEL_DELAYABLE, (uint32_t)(uintptr_t)dwork)

#define sys_port_trace_k_work_cancel_delayable_exit(dwork, ret)                                    \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_WORK_CANCEL_DELAYABLE, (uint32_t)ret)

#define sys_port_trace_k_work_cancel_delayable_sync_enter(dwork, sync)                             \
	SEGGER_SYSVIEW_RecordU32x2(SYS_TRACE_ID_WORK_CANCEL_DELAYABLE_SYNC,                        \
				   (uint32_t)(uintptr_t)dwork, (uint32_t)(uintptr_t)sync)

#define sys_port_trace_k_work_cancel_delayable_sync_exit(dwork, sync, ret)                         \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_WORK_CANCEL_DELAYABLE_SYNC, (uint32_t)ret)

#define sys_port_trace_k_work_poll_init_enter(work)                                                \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_WORK_POLL_INIT, (uint32_t)(uintptr_t)work)

#define sys_port_trace_k_work_poll_init_exit(work)                                                 \
	SEGGER_SYSVIEW_RecordEndCall(SYS_TRACE_ID_WORK_POLL_INIT)

#define sys_port_trace_k_work_poll_submit_to_queue_enter(work_q, work, timeout)                    \
	SEGGER_SYSVIEW_RecordU32x3(SYS_TRACE_ID_WORK_POLL_SUBMIT_TO_QUEUE,                         \
				   (uint32_t)(uintptr_t)work_q, (uint32_t)(uintptr_t)work,         \
				   k_ticks_to_ms_floor32((uint32_t)timeout.ticks))

#define sys_port_trace_k_work_poll_submit_to_queue_blocking(work_q, work, timeout)

#define sys_port_trace_k_work_poll_submit_to_queue_exit(work_q, work, timeout, ret)                \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_WORK_POLL_SUBMIT_TO_QUEUE, (uint32_t)ret)

#define sys_port_trace_k_work_poll_submit_enter(work, timeout)                                     \
	SEGGER_SYSVIEW_RecordU32x2(SYS_TRACE_ID_WORK_POLL_SUBMIT, (uint32_t)(uintptr_t)work,       \
				   k_ticks_to_ms_floor32((uint32_t)timeout.ticks))

#define sys_port_trace_k_work_poll_submit_exit(work, timeout, ret)                                 \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_WORK_POLL_SUBMIT, (uint32_t)ret)

#define sys_port_trace_k_work_poll_cancel_enter(work)                                              \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_WORK_POLL_CANCEL, (uint32_t)(uintptr_t)work)

#define sys_port_trace_k_work_poll_cancel_exit(work, ret)                                          \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_WORK_POLL_CANCEL, (uint32_t)ret)

#define sys_port_trace_k_poll_api_event_init(event)
#define sys_port_trace_k_poll_api_poll_enter(events)
#define sys_port_trace_k_poll_api_poll_exit(events, ret)
#define sys_port_trace_k_poll_api_signal_init(signal)
#define sys_port_trace_k_poll_api_signal_reset(signal)
#define sys_port_trace_k_poll_api_signal_check(signal)
#define sys_port_trace_k_poll_api_signal_raise(signal, ret)

#define sys_port_trace_k_sem_init(sem, ret)                                                        \
	SEGGER_SYSVIEW_RecordU32x2(SYS_TRACE_ID_SEMA_INIT, (uint32_t)(uintptr_t)sem, (int32_t)ret)

#define sys_port_trace_k_sem_give_enter(sem)                                                       \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_SEMA_GIVE, (uint32_t)(uintptr_t)sem)

#define sys_port_trace_k_sem_give_exit(sem) SEGGER_SYSVIEW_RecordEndCall(SYS_TRACE_ID_SEMA_GIVE)

#define sys_port_trace_k_sem_take_enter(sem, timeout)                                              \
	SEGGER_SYSVIEW_RecordU32x2(SYS_TRACE_ID_SEMA_TAKE, (uint32_t)(uintptr_t)sem,               \
				   k_ticks_to_ms_floor32((uint32_t)timeout.ticks))

#define sys_port_trace_k_sem_take_blocking(sem, timeout)                                           \
	SEGGER_SYSVIEW_RecordU32x2(SYS_TRACE_ID_SEMA_BLOCKING, (uint32_t)(uintptr_t)sem,           \
				   k_ticks_to_ms_floor32((uint32_t)timeout.ticks))

#define sys_port_trace_k_sem_take_exit(sem, timeout, ret)                                          \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_SEMA_TAKE, (int32_t)ret)

#define sys_port_trace_k_sem_reset(sem)                                                            \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_SEMA_RESET, (uint32_t)(uintptr_t)sem)

#define sys_port_trace_k_mutex_init(mutex, ret)                                                    \
	SEGGER_SYSVIEW_RecordU32x2(SYS_TRACE_ID_MUTEX_INIT, (uint32_t)(uintptr_t)mutex,            \
				   (int32_t)ret)

#define sys_port_trace_k_mutex_lock_enter(mutex, timeout)                                          \
	SEGGER_SYSVIEW_RecordU32x2(SYS_TRACE_ID_MUTEX_LOCK, (uint32_t)(uintptr_t)mutex,            \
				   k_ticks_to_ms_floor32((uint32_t)timeout.ticks))

#define sys_port_trace_k_mutex_lock_blocking(mutex, timeout)

#define sys_port_trace_k_mutex_lock_exit(mutex, timeout, ret)                                      \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_MUTEX_LOCK, (int32_t)ret)

#define sys_port_trace_k_mutex_unlock_enter(mutex)                                                 \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_MUTEX_UNLOCK, (uint32_t)(uintptr_t)mutex)

#define sys_port_trace_k_mutex_unlock_exit(mutex, ret)                                             \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_MUTEX_UNLOCK, (uint32_t)ret)

#define sys_port_trace_k_condvar_init(condvar, ret)                                                \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_CONDVAR_INIT, (uint32_t)(uintptr_t)condvar)

#define sys_port_trace_k_condvar_signal_enter(condvar)                                             \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_CONDVAR_SIGNAL, (uint32_t)(uintptr_t)condvar)

#define sys_port_trace_k_condvar_signal_blocking(condvar, timeout)

#define sys_port_trace_k_condvar_signal_exit(condvar, ret)                                         \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_CONDVAR_SIGNAL, (uint32_t)ret)

#define sys_port_trace_k_condvar_broadcast_enter(condvar)                                          \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_CONDVAR_BROADCAST, (uint32_t)(uintptr_t)condvar)

#define sys_port_trace_k_condvar_broadcast_exit(condvar, ret)                                      \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_CONDVAR_BROADCAST, (uint32_t)ret)

#define sys_port_trace_k_condvar_wait_enter(condvar)                                               \
	SEGGER_SYSVIEW_RecordU32(SYS_TRACE_ID_CONDVAR_WAIT, (uint32_t)(uintptr_t)condvar)
#define sys_port_trace_k_condvar_wait_exit(condvar, ret)                                           \
	SEGGER_SYSVIEW_RecordEndCallU32(SYS_TRACE_ID_CONDVAR_WAIT, (uint32_t)ret)

#define sys_port_trace_k_queue_init(queue)
#define sys_port_trace_k_queue_cancel_wait(queue)
#define sys_port_trace_k_queue_queue_insert_enter(queue, alloc)
#define sys_port_trace_k_queue_queue_insert_blocking(queue, alloc, timeout)
#define sys_port_trace_k_queue_queue_insert_exit(queue, alloc, ret)
#define sys_port_trace_k_queue_append_enter(queue)
#define sys_port_trace_k_queue_append_exit(queue)
#define sys_port_trace_k_queue_alloc_append_enter(queue)
#define sys_port_trace_k_queue_alloc_append_exit(queue, ret)
#define sys_port_trace_k_queue_prepend_enter(queue)
#define sys_port_trace_k_queue_prepend_exit(queue)
#define sys_port_trace_k_queue_alloc_prepend_enter(queue)
#define sys_port_trace_k_queue_alloc_prepend_exit(queue, ret)
#define sys_port_trace_k_queue_insert_enter(queue)
#define sys_port_trace_k_queue_insert_blocking(queue, timeout)
#define sys_port_trace_k_queue_insert_exit(queue)
#define sys_port_trace_k_queue_append_list_enter(queue)
#define sys_port_trace_k_queue_append_list_exit(queue, ret)
#define sys_port_trace_k_queue_merge_slist_enter(queue)
#define sys_port_trace_k_queue_merge_slist_exit(queue, ret)
#define sys_port_trace_k_queue_get_enter(queue, timeout)
#define sys_port_trace_k_queue_get_blocking(queue, timeout)
#define sys_port_trace_k_queue_get_exit(queue, timeout, ret)
#define sys_port_trace_k_queue_remove_enter(queue)
#define sys_port_trace_k_queue_remove_exit(queue, ret)
#define sys_port_trace_k_queue_unique_append_enter(queue)
#define sys_port_trace_k_queue_unique_append_exit(queue, ret)
#define sys_port_trace_k_queue_peek_head(queue, ret)
#define sys_port_trace_k_queue_peek_tail(queue, ret)

#define sys_port_trace_k_fifo_init_enter(fifo)
#define sys_port_trace_k_fifo_init_exit(fifo)
#define sys_port_trace_k_fifo_cancel_wait_enter(fifo)
#define sys_port_trace_k_fifo_cancel_wait_exit(fifo)
#define sys_port_trace_k_fifo_put_enter(fifo, data)
#define sys_port_trace_k_fifo_put_exit(fifo, data)
#define sys_port_trace_k_fifo_alloc_put_enter(fifo, data)
#define sys_port_trace_k_fifo_alloc_put_exit(fifo, data, ret)
#define sys_port_trace_k_fifo_put_list_enter(fifo, head, tail)
#define sys_port_trace_k_fifo_put_list_exit(fifo, head, tail)
#define sys_port_trace_k_fifo_put_slist_enter(fifo, list)
#define sys_port_trace_k_fifo_put_slist_exit(fifo, list)
#define sys_port_trace_k_fifo_get_enter(fifo, timeout)
#define sys_port_trace_k_fifo_get_exit(fifo, timeout, ret)
#define sys_port_trace_k_fifo_peek_head_enter(fifo)
#define sys_port_trace_k_fifo_peek_head_exit(fifo, ret)
#define sys_port_trace_k_fifo_peek_tail_enter(fifo)
#define sys_port_trace_k_fifo_peek_tail_exit(fifo, ret)

#define sys_port_trace_k_lifo_init_enter(lifo)
#define sys_port_trace_k_lifo_init_exit(lifo)
#define sys_port_trace_k_lifo_put_enter(lifo, data)
#define sys_port_trace_k_lifo_put_exit(lifo, data)
#define sys_port_trace_k_lifo_alloc_put_enter(lifo, data)
#define sys_port_trace_k_lifo_alloc_put_exit(lifo, data, ret)
#define sys_port_trace_k_lifo_get_enter(lifo, timeout)
#define sys_port_trace_k_lifo_get_exit(lifo, timeout, ret)

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
#define sys_port_trace_k_pipe_put_enter(pipe, timeout)
#define sys_port_trace_k_pipe_put_blocking(pipe, timeout)
#define sys_port_trace_k_pipe_put_exit(pipe, timeout, ret)
#define sys_port_trace_k_pipe_get_enter(pipe, timeout)
#define sys_port_trace_k_pipe_get_blocking(pipe, timeout)
#define sys_port_trace_k_pipe_get_exit(pipe, timeout, ret)
#define sys_port_trace_k_pipe_block_put_enter(pipe, sem)
#define sys_port_trace_k_pipe_block_put_exit(pipe, sem)

#define sys_port_trace_k_heap_init(heap)
#define sys_port_trace_k_heap_aligned_alloc_enter(heap, timeout)
#define sys_port_trace_k_heap_aligned_alloc_blocking(heap, timeout)
#define sys_port_trace_k_heap_aligned_alloc_exit(heap, timeout, ret)
#define sys_port_trace_k_heap_alloc_enter(heap, timeout)
#define sys_port_trace_k_heap_alloc_exit(heap, timeout, ret)
#define sys_port_trace_k_heap_free(heap)
#define sys_port_trace_k_heap_sys_k_aligned_alloc_enter(heap)
#define sys_port_trace_k_heap_sys_k_aligned_alloc_exit(heap, ret)
#define sys_port_trace_k_heap_sys_k_malloc_enter(heap)
#define sys_port_trace_k_heap_sys_k_malloc_exit(heap, ret)
#define sys_port_trace_k_heap_sys_k_free_enter(heap)
#define sys_port_trace_k_heap_sys_k_free_exit(heap)
#define sys_port_trace_k_heap_sys_k_calloc_enter(heap)
#define sys_port_trace_k_heap_sys_k_calloc_exit(heap, ret)

#define sys_port_trace_k_mem_slab_init(slab, rc)
#define sys_port_trace_k_mem_slab_alloc_enter(slab, timeout)
#define sys_port_trace_k_mem_slab_alloc_blocking(slab, timeout)
#define sys_port_trace_k_mem_slab_alloc_exit(slab, timeout, ret)
#define sys_port_trace_k_mem_slab_free_enter(slab)
#define sys_port_trace_k_mem_slab_free_exit(slab)

#define sys_port_trace_k_timer_init(timer)
#define sys_port_trace_k_timer_start(timer)
#define sys_port_trace_k_timer_stop(timer)
#define sys_port_trace_k_timer_status_sync_enter(timer)
#define sys_port_trace_k_timer_status_sync_blocking(timer, timeout)
#define sys_port_trace_k_timer_status_sync_exit(timer, result)

#define sys_port_trace_k_thread_resume_exit(thread)

#define sys_port_trace_syscall_enter()
#define sys_port_trace_syscall_exit()

void sys_trace_idle(void);

void sys_trace_k_thread_create(struct k_thread *new_thread, size_t stack_size, int prio);
void sys_trace_k_thread_user_mode_enter(k_thread_entry_t entry, void *p1, void *p2, void *p3);

void sys_trace_k_thread_join_blocking(struct k_thread *thread, k_timeout_t timeout);
void sys_trace_k_thread_join_exit(struct k_thread *thread, k_timeout_t timeout, int ret);

void sys_trace_k_thread_yield(void);
void sys_trace_k_thread_wakeup(struct k_thread *thread);
void sys_trace_k_thread_abort(struct k_thread *thread);
void sys_trace_k_thread_start(struct k_thread *thread);
void sys_trace_k_thread_priority_set(struct k_thread *thread);
void sys_trace_k_thread_suspend(struct k_thread *thread);
void sys_trace_k_thread_resume(struct k_thread *thread);
void sys_trace_k_thread_sched_lock(void);
void sys_trace_k_thread_sched_unlock(void);
void sys_trace_k_thread_name_set(struct k_thread *thread, int ret);
void sys_trace_k_thread_switched_out(void);
void sys_trace_k_thread_switched_in(void);
void sys_trace_k_thread_ready(struct k_thread *thread);
void sys_trace_k_thread_pend(struct k_thread *thread);
void sys_trace_k_thread_info(struct k_thread *thread);

/* Semaphore */

void sys_trace_k_sem_init(struct k_sem *sem, uint32_t initial_count, uint32_t limit, int ret);
void sys_trace_k_sem_give_enter(struct k_sem *sem);

void sys_trace_k_sem_give_exit(struct k_sem *sem);

void sys_trace_k_sem_take_enter(struct k_sem *sem, k_timeout_t timeout);
void sys_trace_k_sem_take_blocking(struct k_sem *sem, k_timeout_t timeout);
void sys_trace_k_sem_take_exit(struct k_sem *sem, k_timeout_t timeout, int ret);
void sys_trace_k_sem_reset(struct k_sem *sem);

/* Mutex */
void sys_trace_k_mutex_init(struct k_mutex *mutex, int ret);
void sys_trace_k_mutex_lock_enter(struct k_mutex *mutex, k_timeout_t timeout);
void sys_trace_k_mutex_lock_blocking(struct k_mutex *mutex, k_timeout_t timeout);
void sys_trace_k_mutex_lock_exit(struct k_mutex *mutex, k_timeout_t timeout, int ret);
void sys_trace_k_mutex_unlock_enter(struct k_mutex *mutex);
void sys_trace_k_mutex_unlock_exit(struct k_mutex *mutex, int ret);

void sys_trace_k_timer_init(struct k_timer *timer, k_timer_expiry_t expiry_fn,
			    k_timer_expiry_t stop_fn);
void sys_trace_k_timer_start(struct k_timer *timer, k_timeout_t duration, k_timeout_t period);
void sys_trace_k_timer_stop(struct k_timer *timer);
void sys_trace_k_timer_status_sync_blocking(struct k_timer *timer);
void sys_trace_k_timer_status_sync_exit(struct k_timer *timer, uint32_t result);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TRACE_SYSVIEW_H */
