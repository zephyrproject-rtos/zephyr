/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup subsys_tracing_apis
 * @brief Canonical tracing hook list.
 *
 * Single source of truth for the full set of `sys_port_trace_*` hook macros.
 *
 * Each hook is wrapped in `#ifndef`, so this header may be included *after* a
 * tracing backend header (tracing_ctf.h, tracing_sysview.h, ...). Any hook the
 * selected backend did not define falls back to a no-op here. Backends therefore
 * only define the hooks they actually record, and a forgotten hook degrades to a
 * no-op instead of a build break or silent drift.
 *
 * To add a new hook: add it here (no-op) and implement a body in whichever
 * backends should record it.
 */

#ifndef ZEPHYR_INCLUDE_TRACING_TRACING_HOOKS_H_
#define ZEPHYR_INCLUDE_TRACING_TRACING_HOOKS_H_

/**
 * @brief Tracing hooks for thread events
 * @defgroup subsys_tracing_apis_thread Thread
 * @{
 */

/**
 * @brief Called when entering a k_thread_foreach call
 */
#ifndef sys_port_trace_k_thread_foreach_enter
#define sys_port_trace_k_thread_foreach_enter()
#endif

/**
 * @brief Called when exiting a k_thread_foreach call
 */
#ifndef sys_port_trace_k_thread_foreach_exit
#define sys_port_trace_k_thread_foreach_exit()
#endif

/**
 * @brief Called when entering a k_thread_foreach_unlocked
 */
#ifndef sys_port_trace_k_thread_foreach_unlocked_enter
#define sys_port_trace_k_thread_foreach_unlocked_enter()
#endif

/**
 * @brief Called when exiting a k_thread_foreach_unlocked
 */
#ifndef sys_port_trace_k_thread_foreach_unlocked_exit
#define sys_port_trace_k_thread_foreach_unlocked_exit()
#endif

/**
 * @brief Trace creating a Thread
 * @param new_thread Thread object
 */
#ifndef sys_port_trace_k_thread_create
#define sys_port_trace_k_thread_create(new_thread)
#endif

/**
 * @brief Trace Thread entering user mode
 */
#ifndef sys_port_trace_k_thread_user_mode_enter
#define sys_port_trace_k_thread_user_mode_enter()
#endif

/**
 * @brief Called when entering a k_thread_join
 * @param thread Thread object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_thread_join_enter
#define sys_port_trace_k_thread_join_enter(thread, timeout)
#endif

/**
 * @brief Called when k_thread_join blocks
 * @param thread Thread object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_thread_join_blocking
#define sys_port_trace_k_thread_join_blocking(thread, timeout)
#endif

/**
 * @brief Called when exiting k_thread_join
 * @param thread Thread object
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_thread_join_exit
#define sys_port_trace_k_thread_join_exit(thread, timeout, ret)
#endif

/**
 * @brief Called when entering k_thread_sleep
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_thread_sleep_enter
#define sys_port_trace_k_thread_sleep_enter(timeout)
#endif

/**
 * @brief Called when exiting k_thread_sleep
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_thread_sleep_exit
#define sys_port_trace_k_thread_sleep_exit(timeout, ret)
#endif

/**
 * @brief Called when entering k_thread_msleep
 * @param ms Duration in milliseconds
 */
#ifndef sys_port_trace_k_thread_msleep_enter
#define sys_port_trace_k_thread_msleep_enter(ms)
#endif

/**
 * @brief Called when exiting k_thread_msleep
 * @param ms Duration in milliseconds
 * @param ret Return value
 */
#ifndef sys_port_trace_k_thread_msleep_exit
#define sys_port_trace_k_thread_msleep_exit(ms, ret)
#endif

/**
 * @brief Called when entering k_thread_usleep
 * @param us Duration in microseconds
 */
#ifndef sys_port_trace_k_thread_usleep_enter
#define sys_port_trace_k_thread_usleep_enter(us)
#endif

/**
 * @brief Called when exiting k_thread_usleep
 * @param us Duration in microseconds
 * @param ret Return value
 */
#ifndef sys_port_trace_k_thread_usleep_exit
#define sys_port_trace_k_thread_usleep_exit(us, ret)
#endif

/**
 * @brief Called when entering k_thread_busy_wait
 * @param usec_to_wait Duration in microseconds
 */
#ifndef sys_port_trace_k_thread_busy_wait_enter
#define sys_port_trace_k_thread_busy_wait_enter(usec_to_wait)
#endif

/**
 * @brief Called when exiting k_thread_busy_wait
 * @param usec_to_wait Duration in microseconds
 */
#ifndef sys_port_trace_k_thread_busy_wait_exit
#define sys_port_trace_k_thread_busy_wait_exit(usec_to_wait)
#endif

/**
 * @brief Called when a thread yields
 */
#ifndef sys_port_trace_k_thread_yield
#define sys_port_trace_k_thread_yield()
#endif

/**
 * @brief Called when a thread wakes up
 * @param thread Thread object
 */
#ifndef sys_port_trace_k_thread_wakeup
#define sys_port_trace_k_thread_wakeup(thread)
#endif

/**
 * @brief Called when a thread is started
 * @param thread Thread object
 */
#ifndef sys_port_trace_k_thread_start
#define sys_port_trace_k_thread_start(thread)
#endif

/**
 * @brief Called when a thread is being aborted
 * @param thread Thread object
 */
#ifndef sys_port_trace_k_thread_abort
#define sys_port_trace_k_thread_abort(thread)
#endif

/**
 * @brief Called when a thread enters the k_thread_abort routine
 * @param thread Thread object
 */
#ifndef sys_port_trace_k_thread_abort_enter
#define sys_port_trace_k_thread_abort_enter(thread)
#endif

/**
 * @brief Called when a thread exits the k_thread_abort routine
 * @param thread Thread object
 */
#ifndef sys_port_trace_k_thread_abort_exit
#define sys_port_trace_k_thread_abort_exit(thread)
#endif

/**
 * @brief Called when setting priority of a thread
 * @param thread Thread object
 */
#ifndef sys_port_trace_k_thread_priority_set
#define sys_port_trace_k_thread_priority_set(thread)
#endif

/**
 * @brief Called when a thread enters the k_thread_suspend
 * function.
 * @param thread Thread object
 */
#ifndef sys_port_trace_k_thread_suspend_enter
#define sys_port_trace_k_thread_suspend_enter(thread)
#endif

/**
 * @brief Called when a thread exits the k_thread_suspend
 * function.
 * @param thread Thread object
 */
#ifndef sys_port_trace_k_thread_suspend_exit
#define sys_port_trace_k_thread_suspend_exit(thread)
#endif

/**
 * @brief Called when a thread enters the resume from suspension
 * function.
 * @param thread Thread object
 */
#ifndef sys_port_trace_k_thread_resume_enter
#define sys_port_trace_k_thread_resume_enter(thread)
#endif

/**
 * @brief Called when a thread exits the resumed from suspension
 * function.
 * @param thread Thread object
 */
#ifndef sys_port_trace_k_thread_resume_exit
#define sys_port_trace_k_thread_resume_exit(thread)
#endif

/**
 * @brief Called when the thread scheduler is locked
 */
#ifndef sys_port_trace_k_thread_sched_lock
#define sys_port_trace_k_thread_sched_lock()
#endif

/**
 * @brief Called when the thread scheduler is unlocked
 */
#ifndef sys_port_trace_k_thread_sched_unlock
#define sys_port_trace_k_thread_sched_unlock()
#endif

/**
 * @brief Called when a thread name is set
 * @param thread Thread object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_thread_name_set
#define sys_port_trace_k_thread_name_set(thread, ret)
#endif

/**
 * @brief Called before a thread has been selected to run
 */
#ifndef sys_port_trace_k_thread_switched_out
#define sys_port_trace_k_thread_switched_out()
#endif

/**
 * @brief Called after a thread has been selected to run
 */
#ifndef sys_port_trace_k_thread_switched_in
#define sys_port_trace_k_thread_switched_in()
#endif

/**
 * @brief Called when a thread is ready to run
 * @param thread Thread object
 */
#ifndef sys_port_trace_k_thread_ready
#define sys_port_trace_k_thread_ready(thread)
#endif

/**
 * @brief Called when a thread is pending
 * @param thread Thread object
 */
#ifndef sys_port_trace_k_thread_pend
#define sys_port_trace_k_thread_pend(thread)
#endif

/**
 * @brief Provide information about specific thread
 * @param thread Thread object
 */
#ifndef sys_port_trace_k_thread_info
#define sys_port_trace_k_thread_info(thread)
#endif

/**
 * @brief Trace implicit thread wakeup invocation by the scheduler
 * @param thread Thread object
 */
#ifndef sys_port_trace_k_thread_sched_wakeup
#define sys_port_trace_k_thread_sched_wakeup(thread)
#endif

/**
 * @brief Trace implicit thread abort invocation by the scheduler
 * @param thread Thread object
 */
#ifndef sys_port_trace_k_thread_sched_abort
#define sys_port_trace_k_thread_sched_abort(thread)
#endif

/**
 * @brief Trace implicit thread set priority invocation by the scheduler
 * @param thread Thread object
 * @param prio Thread priority
 */
#ifndef sys_port_trace_k_thread_sched_priority_set
#define sys_port_trace_k_thread_sched_priority_set(thread, prio)
#endif

/**
 * @brief Trace implicit thread ready invocation by the scheduler
 * @param thread Thread object
 */
#ifndef sys_port_trace_k_thread_sched_ready
#define sys_port_trace_k_thread_sched_ready(thread)
#endif

/**
 * @brief Trace implicit thread pend invocation by the scheduler
 * @param thread Thread object
 */
#ifndef sys_port_trace_k_thread_sched_pend
#define sys_port_trace_k_thread_sched_pend(thread)
#endif

/**
 * @brief Trace implicit thread resume invocation by the scheduler
 * @param thread Thread object
 */
#ifndef sys_port_trace_k_thread_sched_resume
#define sys_port_trace_k_thread_sched_resume(thread)
#endif

/**
 * @brief Trace implicit thread suspend invocation by the scheduler
 * @param thread Thread object
 */
#ifndef sys_port_trace_k_thread_sched_suspend
#define sys_port_trace_k_thread_sched_suspend(thread)
#endif

/** @}c*/ /* end of subsys_tracing_apis_thread */

/**
 * @brief Tracing hooks for work item events
 * @defgroup subsys_tracing_apis_work Work item
 * @{
 */

/**
 * @brief Trace initialisation of a Work structure
 * @param work Work structure
 */
#ifndef sys_port_trace_k_work_init
#define sys_port_trace_k_work_init(work)
#endif

/**
 * @brief Trace submit work to work queue call entry
 * @param queue Work queue structure
 * @param work Work structure
 */
#ifndef sys_port_trace_k_work_submit_to_queue_enter
#define sys_port_trace_k_work_submit_to_queue_enter(queue, work)
#endif

/**
 * @brief Trace submit work to work queue call exit
 * @param queue Work queue structure
 * @param work Work structure
 * @param ret Return value
 */
#ifndef sys_port_trace_k_work_submit_to_queue_exit
#define sys_port_trace_k_work_submit_to_queue_exit(queue, work, ret)
#endif

/**
 * @brief Trace submit work to system work queue call entry
 * @param work Work structure
 */
#ifndef sys_port_trace_k_work_submit_enter
#define sys_port_trace_k_work_submit_enter(work)
#endif

/**
 * @brief Trace submit work to system work queue call exit
 * @param work Work structure
 * @param ret Return value
 */
#ifndef sys_port_trace_k_work_submit_exit
#define sys_port_trace_k_work_submit_exit(work, ret)
#endif

/**
 * @brief Trace flush work call entry
 * @param work Work structure
 */
#ifndef sys_port_trace_k_work_flush_enter
#define sys_port_trace_k_work_flush_enter(work)
#endif

/**
 * @brief Trace flush work call blocking
 * @param work Work structure
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_work_flush_blocking
#define sys_port_trace_k_work_flush_blocking(work, timeout)
#endif

/**
 * @brief Trace flush work call exit
 * @param work Work structure
 * @param ret Return value
 */
#ifndef sys_port_trace_k_work_flush_exit
#define sys_port_trace_k_work_flush_exit(work, ret)
#endif

/**
 * @brief Trace cancel work call entry
 * @param work Work structure
 */
#ifndef sys_port_trace_k_work_cancel_enter
#define sys_port_trace_k_work_cancel_enter(work)
#endif

/**
 * @brief Trace cancel work call exit
 * @param work Work structure
 * @param ret Return value
 */
#ifndef sys_port_trace_k_work_cancel_exit
#define sys_port_trace_k_work_cancel_exit(work, ret)
#endif

/**
 * @brief Trace cancel sync work call entry
 * @param work Work structure
 * @param sync Sync object
 */
#ifndef sys_port_trace_k_work_cancel_sync_enter
#define sys_port_trace_k_work_cancel_sync_enter(work, sync)
#endif

/**
 * @brief Trace cancel sync work call blocking
 * @param work Work structure
 * @param sync Sync object
 */
#ifndef sys_port_trace_k_work_cancel_sync_blocking
#define sys_port_trace_k_work_cancel_sync_blocking(work, sync)
#endif

/**
 * @brief Trace cancel sync work call exit
 * @param work Work structure
 * @param sync Sync object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_work_cancel_sync_exit
#define sys_port_trace_k_work_cancel_sync_exit(work, sync, ret)
#endif

/** @} */ /* end of subsys_tracing_apis_work */

/**
 * @brief Tracing hooks for work queue events
 * @defgroup subsys_tracing_apis_work_q Work queue
 * @{
 */

/**
 * @brief Trace initialisation of a Work Queue structure
 * @param queue Work Queue structure
 */
#ifndef sys_port_trace_k_work_queue_init
#define sys_port_trace_k_work_queue_init(queue)
#endif

/**
 * @brief Trace start of a Work Queue call entry
 * @param queue Work Queue structure
 */
#ifndef sys_port_trace_k_work_queue_start_enter
#define sys_port_trace_k_work_queue_start_enter(queue)
#endif

/**
 * @brief Trace start of a Work Queue call exit
 * @param queue Work Queue structure
 */
#ifndef sys_port_trace_k_work_queue_start_exit
#define sys_port_trace_k_work_queue_start_exit(queue)
#endif

/**
 * @brief Trace stop of a Work Queue call entry
 * @param queue Work Queue structure
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_work_queue_stop_enter
#define sys_port_trace_k_work_queue_stop_enter(queue, timeout)
#endif

/**
 * @brief Trace stop of a Work Queue call blocking
 * @param queue Work Queue structure
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_work_queue_stop_blocking
#define sys_port_trace_k_work_queue_stop_blocking(queue, timeout)
#endif

/**
 * @brief Trace stop of a Work Queue call exit
 * @param queue Work Queue structure
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_work_queue_stop_exit
#define sys_port_trace_k_work_queue_stop_exit(queue, timeout, ret)
#endif

/**
 * @brief Trace Work Queue drain call entry
 * @param queue Work Queue structure
 */
#ifndef sys_port_trace_k_work_queue_drain_enter
#define sys_port_trace_k_work_queue_drain_enter(queue)
#endif

/**
 * @brief Trace Work Queue drain call exit
 * @param queue Work Queue structure
 * @param ret Return value
 */
#ifndef sys_port_trace_k_work_queue_drain_exit
#define sys_port_trace_k_work_queue_drain_exit(queue, ret)
#endif

/**
 * @brief Trace Work Queue unplug call entry
 * @param queue Work Queue structure
 */
#ifndef sys_port_trace_k_work_queue_unplug_enter
#define sys_port_trace_k_work_queue_unplug_enter(queue)
#endif

/**
 * @brief Trace Work Queue unplug call exit
 * @param queue Work Queue structure
 * @param ret Return value
 */
#ifndef sys_port_trace_k_work_queue_unplug_exit
#define sys_port_trace_k_work_queue_unplug_exit(queue, ret)
#endif

/** @} */ /* end of subsys_tracing_apis_work_q */

/**
 * @brief Tracing hooks for delayable work item events
 * @defgroup subsys_tracing_apis_work_delayable Delayable work item
 * @{
 */

/**
 * @brief Trace initialisation of a Delayable Work structure
 * @param dwork Delayable Work structure
 */
#ifndef sys_port_trace_k_work_delayable_init
#define sys_port_trace_k_work_delayable_init(dwork)
#endif

/**
 * @brief Trace schedule delayable work for queue enter
 * @param queue Work Queue structure
 * @param dwork Delayable Work structure
 * @param delay Delay period
 */
#ifndef sys_port_trace_k_work_schedule_for_queue_enter
#define sys_port_trace_k_work_schedule_for_queue_enter(queue, dwork, delay)
#endif

/**
 * @brief Trace schedule delayable work for queue exit
 * @param queue Work Queue structure
 * @param dwork Delayable Work structure
 * @param delay Delay period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_work_schedule_for_queue_exit
#define sys_port_trace_k_work_schedule_for_queue_exit(queue, dwork, delay, ret)
#endif

/**
 * @brief Trace schedule delayable work for system work queue enter
 * @param dwork Delayable Work structure
 * @param delay Delay period
 */
#ifndef sys_port_trace_k_work_schedule_enter
#define sys_port_trace_k_work_schedule_enter(dwork, delay)
#endif

/**
 * @brief Trace schedule delayable work for system work queue exit
 * @param dwork Delayable Work structure
 * @param delay Delay period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_work_schedule_exit
#define sys_port_trace_k_work_schedule_exit(dwork, delay, ret)
#endif

/**
 * @brief Trace reschedule delayable work for queue enter
 * @param queue Work Queue structure
 * @param dwork Delayable Work structure
 * @param delay Delay period
 */
#ifndef sys_port_trace_k_work_reschedule_for_queue_enter
#define sys_port_trace_k_work_reschedule_for_queue_enter(queue, dwork, delay)
#endif

/**
 * @brief Trace reschedule delayable work for queue exit
 * @param queue Work Queue structure
 * @param dwork Delayable Work structure
 * @param delay Delay period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_work_reschedule_for_queue_exit
#define sys_port_trace_k_work_reschedule_for_queue_exit(queue, dwork, delay, ret)
#endif

/**
 * @brief Trace reschedule delayable work for system queue enter
 * @param dwork Delayable Work structure
 * @param delay Delay period
 */
#ifndef sys_port_trace_k_work_reschedule_enter
#define sys_port_trace_k_work_reschedule_enter(dwork, delay)
#endif

/**
 * @brief Trace reschedule delayable work for system queue exit
 * @param dwork Delayable Work structure
 * @param delay Delay period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_work_reschedule_exit
#define sys_port_trace_k_work_reschedule_exit(dwork, delay, ret)
#endif

/**
 * @brief Trace delayable work flush enter
 * @param dwork Delayable Work structure
 * @param sync Sync object
 */
#ifndef sys_port_trace_k_work_flush_delayable_enter
#define sys_port_trace_k_work_flush_delayable_enter(dwork, sync)
#endif

/**
 * @brief Trace delayable work flush exit
 * @param dwork Delayable Work structure
 * @param sync Sync object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_work_flush_delayable_exit
#define sys_port_trace_k_work_flush_delayable_exit(dwork, sync, ret)
#endif

/**
 * @brief Trace delayable work cancel enter
 * @param dwork Delayable Work structure
 */
#ifndef sys_port_trace_k_work_cancel_delayable_enter
#define sys_port_trace_k_work_cancel_delayable_enter(dwork)
#endif

/**
 * @brief Trace delayable work cancel enter
 * @param dwork Delayable Work structure
 * @param ret Return value
 */
#ifndef sys_port_trace_k_work_cancel_delayable_exit
#define sys_port_trace_k_work_cancel_delayable_exit(dwork, ret)
#endif

/**
 * @brief Trace delayable work cancel sync enter
 * @param dwork Delayable Work structure
 * @param sync Sync object
 */
#ifndef sys_port_trace_k_work_cancel_delayable_sync_enter
#define sys_port_trace_k_work_cancel_delayable_sync_enter(dwork, sync)
#endif

/**
 * @brief Trace delayable work cancel sync enter
 * @param dwork Delayable Work structure
 * @param sync Sync object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_work_cancel_delayable_sync_exit
#define sys_port_trace_k_work_cancel_delayable_sync_exit(dwork, sync, ret)
#endif

/** @} */ /* end of subsys_tracing_apis_work_delayable */

/**
 * @brief Tracing hooks for triggered work item events
 * @defgroup subsys_tracing_apis_work_poll Triggered work item
 * @{
 */

/**
 * @brief Trace initialisation of a Work Poll structure enter
 * @param work Work structure
 */
#ifndef sys_port_trace_k_work_poll_init_enter
#define sys_port_trace_k_work_poll_init_enter(work)
#endif

/**
 * @brief Trace initialisation of a Work Poll structure exit
 * @param work Work structure
 */
#ifndef sys_port_trace_k_work_poll_init_exit
#define sys_port_trace_k_work_poll_init_exit(work)
#endif

/**
 * @brief Trace work poll submit to queue enter
 * @param work_q Work queue
 * @param work Work structure
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_work_poll_submit_to_queue_enter
#define sys_port_trace_k_work_poll_submit_to_queue_enter(work_q, work, timeout)
#endif

/**
 * @brief Trace work poll submit to queue blocking
 * @param work_q Work queue
 * @param work Work structure
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_work_poll_submit_to_queue_blocking
#define sys_port_trace_k_work_poll_submit_to_queue_blocking(work_q, work, timeout)
#endif

/**
 * @brief Trace work poll submit to queue exit
 * @param work_q Work queue
 * @param work Work structure
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_work_poll_submit_to_queue_exit
#define sys_port_trace_k_work_poll_submit_to_queue_exit(work_q, work, timeout, ret)
#endif

/**
 * @brief Trace work poll submit to system queue enter
 * @param work Work structure
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_work_poll_submit_enter
#define sys_port_trace_k_work_poll_submit_enter(work, timeout)
#endif

/**
 * @brief Trace work poll submit to system queue exit
 * @param work Work structure
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_work_poll_submit_exit
#define sys_port_trace_k_work_poll_submit_exit(work, timeout, ret)
#endif

/**
 * @brief Trace work poll cancel enter
 * @param work Work structure
 */
#ifndef sys_port_trace_k_work_poll_cancel_enter
#define sys_port_trace_k_work_poll_cancel_enter(work)
#endif

/**
 * @brief Trace work poll cancel exit
 * @param work Work structure
 * @param ret Return value
 */
#ifndef sys_port_trace_k_work_poll_cancel_exit
#define sys_port_trace_k_work_poll_cancel_exit(work, ret)
#endif

/** @} */ /* end of subsys_tracing_apis_work_poll */

/**
 * @brief Tracing hooks for polling events
 * @defgroup subsys_tracing_apis_poll Polling
 * @{
 */

/**
 * @brief Trace initialisation of a Poll Event
 * @param event Poll Event
 */
#ifndef sys_port_trace_k_poll_api_event_init
#define sys_port_trace_k_poll_api_event_init(event)
#endif

/**
 * @brief Trace Polling call start
 * @param events Poll Events
 */
#ifndef sys_port_trace_k_poll_api_poll_enter
#define sys_port_trace_k_poll_api_poll_enter(events)
#endif

/**
 * @brief Trace Polling call outcome
 * @param events Poll Events
 * @param ret Return value
 */
#ifndef sys_port_trace_k_poll_api_poll_exit
#define sys_port_trace_k_poll_api_poll_exit(events, ret)
#endif

/**
 * @brief Trace initialisation of a Poll Signal
 * @param signal Poll Signal
 */
#ifndef sys_port_trace_k_poll_api_signal_init
#define sys_port_trace_k_poll_api_signal_init(signal)
#endif

/**
 * @brief Trace resetting of Poll Signal
 * @param signal Poll Signal
 */
#ifndef sys_port_trace_k_poll_api_signal_reset
#define sys_port_trace_k_poll_api_signal_reset(signal)
#endif

/**
 * @brief Trace checking of Poll Signal
 * @param signal Poll Signal
 */
#ifndef sys_port_trace_k_poll_api_signal_check
#define sys_port_trace_k_poll_api_signal_check(signal)
#endif

/**
 * @brief Trace raising of Poll Signal
 * @param signal Poll Signal
 * @param ret Return value
 */
#ifndef sys_port_trace_k_poll_api_signal_raise
#define sys_port_trace_k_poll_api_signal_raise(signal, ret)
#endif

/** @} */ /* end of subsys_tracing_apis_poll */

/**
 * @brief Tracing hooks for semaphore events
 * @defgroup subsys_tracing_apis_sem Semaphore
 * @{
 */

/**
 * @brief Trace initialisation of a Semaphore
 * @param sem Semaphore object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_sem_init
#define sys_port_trace_k_sem_init(sem, ret)
#endif

/**
 * @brief Trace giving a Semaphore entry
 * @param sem Semaphore object
 */
#ifndef sys_port_trace_k_sem_give_enter
#define sys_port_trace_k_sem_give_enter(sem)
#endif

/**
 * @brief Trace giving a Semaphore exit
 * @param sem Semaphore object
 */
#ifndef sys_port_trace_k_sem_give_exit
#define sys_port_trace_k_sem_give_exit(sem)
#endif

/**
 * @brief Trace taking a Semaphore attempt start
 * @param sem Semaphore object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_sem_take_enter
#define sys_port_trace_k_sem_take_enter(sem, timeout)
#endif

/**
 * @brief Trace taking a Semaphore attempt blocking
 * @param sem Semaphore object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_sem_take_blocking
#define sys_port_trace_k_sem_take_blocking(sem, timeout)
#endif

/**
 * @brief Trace taking a Semaphore attempt outcome
 * @param sem Semaphore object
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_sem_take_exit
#define sys_port_trace_k_sem_take_exit(sem, timeout, ret)
#endif

/**
 * @brief Trace resetting a Semaphore
 * @param sem Semaphore object
 */
#ifndef sys_port_trace_k_sem_reset
#define sys_port_trace_k_sem_reset(sem)
#endif

/** @} */ /* end of subsys_tracing_apis_sem */

/**
 * @brief Tracing hooks for mutex events
 * @defgroup subsys_tracing_apis_mutex Mutex
 * @{
 */

/**
 * @brief Trace initialization of Mutex
 * @param mutex Mutex object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_mutex_init
#define sys_port_trace_k_mutex_init(mutex, ret)
#endif

/**
 * @brief Trace Mutex lock attempt start
 * @param mutex Mutex object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_mutex_lock_enter
#define sys_port_trace_k_mutex_lock_enter(mutex, timeout)
#endif

/**
 * @brief Trace Mutex lock attempt blocking
 * @param mutex Mutex object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_mutex_lock_blocking
#define sys_port_trace_k_mutex_lock_blocking(mutex, timeout)
#endif

/**
 * @brief Trace Mutex lock attempt outcome
 * @param mutex Mutex object
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_mutex_lock_exit
#define sys_port_trace_k_mutex_lock_exit(mutex, timeout, ret)
#endif

/**
 * @brief Trace Mutex unlock entry
 * @param mutex Mutex object
 */
#ifndef sys_port_trace_k_mutex_unlock_enter
#define sys_port_trace_k_mutex_unlock_enter(mutex)
#endif

/**
 * @brief Trace Mutex unlock exit
 */
#ifndef sys_port_trace_k_mutex_unlock_exit
#define sys_port_trace_k_mutex_unlock_exit(mutex, ret)
#endif

/** @} */ /* end of subsys_tracing_apis_mutex */

/**
 * @brief Tracing hooks for conditional variable events
 * @defgroup subsys_tracing_apis_condvar Conditional variable
 * @{
 */

/**
 * @brief Trace initialization of Conditional Variable
 * @param condvar Conditional Variable object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_condvar_init
#define sys_port_trace_k_condvar_init(condvar, ret)
#endif

/**
 * @brief Trace Conditional Variable signaling start
 * @param condvar Conditional Variable object
 */
#ifndef sys_port_trace_k_condvar_signal_enter
#define sys_port_trace_k_condvar_signal_enter(condvar)
#endif

/**
 * @brief Trace Conditional Variable signaling blocking
 * @param condvar Conditional Variable object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_condvar_signal_blocking
#define sys_port_trace_k_condvar_signal_blocking(condvar, timeout)
#endif

/**
 * @brief Trace Conditional Variable signaling outcome
 * @param condvar Conditional Variable object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_condvar_signal_exit
#define sys_port_trace_k_condvar_signal_exit(condvar, ret)
#endif

/**
 * @brief Trace Conditional Variable broadcast enter
 * @param condvar Conditional Variable object
 */
#ifndef sys_port_trace_k_condvar_broadcast_enter
#define sys_port_trace_k_condvar_broadcast_enter(condvar)
#endif

/**
 * @brief Trace Conditional Variable broadcast exit
 * @param condvar Conditional Variable object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_condvar_broadcast_exit
#define sys_port_trace_k_condvar_broadcast_exit(condvar, ret)
#endif

/**
 * @brief Trace Conditional Variable wait enter
 * @param condvar Conditional Variable object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_condvar_wait_enter
#define sys_port_trace_k_condvar_wait_enter(condvar, timeout)
#endif

/**
 * @brief Trace Conditional Variable wait exit
 * @param condvar Conditional Variable object
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_condvar_wait_exit
#define sys_port_trace_k_condvar_wait_exit(condvar, timeout, ret)
#endif

/** @} */ /* end of subsys_tracing_apis_condvar */

/**
 * @brief Tracing hooks for queue events
 * @defgroup subsys_tracing_apis_queue Queue
 * @{
 */

/**
 * @brief Trace initialization of Queue
 * @param queue Queue object
 */
#ifndef sys_port_trace_k_queue_init
#define sys_port_trace_k_queue_init(queue)
#endif

/**
 * @brief Trace Queue cancel wait
 * @param queue Queue object
 */
#ifndef sys_port_trace_k_queue_cancel_wait
#define sys_port_trace_k_queue_cancel_wait(queue)
#endif

/**
 * @brief Trace Queue insert attempt entry
 * @param queue Queue object
 * @param alloc Allocation flag
 */
#ifndef sys_port_trace_k_queue_queue_insert_enter
#define sys_port_trace_k_queue_queue_insert_enter(queue, alloc)
#endif

/**
 * @brief Trace Queue insert attempt blocking
 * @param queue Queue object
 * @param alloc Allocation flag
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_queue_queue_insert_blocking
#define sys_port_trace_k_queue_queue_insert_blocking(queue, alloc, timeout)
#endif

/**
 * @brief Trace Queue insert attempt outcome
 * @param queue Queue object
 * @param alloc Allocation flag
 * @param ret Return value
 */
#ifndef sys_port_trace_k_queue_queue_insert_exit
#define sys_port_trace_k_queue_queue_insert_exit(queue, alloc, ret)
#endif

/**
 * @brief Trace Queue append enter
 * @param queue Queue object
 */
#ifndef sys_port_trace_k_queue_append_enter
#define sys_port_trace_k_queue_append_enter(queue)
#endif

/**
 * @brief Trace Queue append exit
 * @param queue Queue object
 */
#ifndef sys_port_trace_k_queue_append_exit
#define sys_port_trace_k_queue_append_exit(queue)
#endif

/**
 * @brief Trace Queue alloc append enter
 * @param queue Queue object
 */
#ifndef sys_port_trace_k_queue_alloc_append_enter
#define sys_port_trace_k_queue_alloc_append_enter(queue)
#endif

/**
 * @brief Trace Queue alloc append exit
 * @param queue Queue object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_queue_alloc_append_exit
#define sys_port_trace_k_queue_alloc_append_exit(queue, ret)
#endif

/**
 * @brief Trace Queue prepend enter
 * @param queue Queue object
 */
#ifndef sys_port_trace_k_queue_prepend_enter
#define sys_port_trace_k_queue_prepend_enter(queue)
#endif

/**
 * @brief Trace Queue prepend exit
 * @param queue Queue object
 */
#ifndef sys_port_trace_k_queue_prepend_exit
#define sys_port_trace_k_queue_prepend_exit(queue)
#endif

/**
 * @brief Trace Queue alloc prepend enter
 * @param queue Queue object
 */
#ifndef sys_port_trace_k_queue_alloc_prepend_enter
#define sys_port_trace_k_queue_alloc_prepend_enter(queue)
#endif

/**
 * @brief Trace Queue alloc prepend exit
 * @param queue Queue object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_queue_alloc_prepend_exit
#define sys_port_trace_k_queue_alloc_prepend_exit(queue, ret)
#endif

/**
 * @brief Trace Queue insert attempt entry
 * @param queue Queue object
 */
#ifndef sys_port_trace_k_queue_insert_enter
#define sys_port_trace_k_queue_insert_enter(queue)
#endif

/**
 * @brief Trace Queue insert attempt blocking
 * @param queue Queue object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_queue_insert_blocking
#define sys_port_trace_k_queue_insert_blocking(queue, timeout)
#endif

/**
 * @brief Trace Queue insert attempt exit
 * @param queue Queue object
 */
#ifndef sys_port_trace_k_queue_insert_exit
#define sys_port_trace_k_queue_insert_exit(queue)
#endif

/**
 * @brief Trace Queue append list enter
 * @param queue Queue object
 */
#ifndef sys_port_trace_k_queue_append_list_enter
#define sys_port_trace_k_queue_append_list_enter(queue)
#endif

/**
 * @brief Trace Queue append list exit
 * @param queue Queue object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_queue_append_list_exit
#define sys_port_trace_k_queue_append_list_exit(queue, ret)
#endif

/**
 * @brief Trace Queue merge slist enter
 * @param queue Queue object
 */
#ifndef sys_port_trace_k_queue_merge_slist_enter
#define sys_port_trace_k_queue_merge_slist_enter(queue)
#endif

/**
 * @brief Trace Queue merge slist exit
 * @param queue Queue object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_queue_merge_slist_exit
#define sys_port_trace_k_queue_merge_slist_exit(queue, ret)
#endif

/**
 * @brief Trace Queue get attempt enter
 * @param queue Queue object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_queue_get_enter
#define sys_port_trace_k_queue_get_enter(queue, timeout)
#endif

/**
 * @brief Trace Queue get attempt blockings
 * @param queue Queue object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_queue_get_blocking
#define sys_port_trace_k_queue_get_blocking(queue, timeout)
#endif

/**
 * @brief Trace Queue get attempt outcome
 * @param queue Queue object
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_queue_get_exit
#define sys_port_trace_k_queue_get_exit(queue, timeout, ret)
#endif

/**
 * @brief Trace Queue remove enter
 * @param queue Queue object
 */
#ifndef sys_port_trace_k_queue_remove_enter
#define sys_port_trace_k_queue_remove_enter(queue)
#endif

/**
 * @brief Trace Queue remove exit
 * @param queue Queue object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_queue_remove_exit
#define sys_port_trace_k_queue_remove_exit(queue, ret)
#endif

/**
 * @brief Trace Queue unique append enter
 * @param queue Queue object
 */
#ifndef sys_port_trace_k_queue_unique_append_enter
#define sys_port_trace_k_queue_unique_append_enter(queue)
#endif

/**
 * @brief Trace Queue unique append exit
 * @param queue Queue object
 *
 * @param ret Return value
 */
#ifndef sys_port_trace_k_queue_unique_append_exit
#define sys_port_trace_k_queue_unique_append_exit(queue, ret)
#endif

/**
 * @brief Trace Queue peek head
 * @param queue Queue object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_queue_peek_head
#define sys_port_trace_k_queue_peek_head(queue, ret)
#endif

/**
 * @brief Trace Queue peek tail
 * @param queue Queue object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_queue_peek_tail
#define sys_port_trace_k_queue_peek_tail(queue, ret)
#endif

/** @} */ /* end of subsys_tracing_apis_queue */

/**
 * @brief Tracing hooks for FIFO events
 * @defgroup subsys_tracing_apis_fifo FIFO
 * @{
 */

/**
 * @brief Trace initialization of FIFO Queue entry
 * @param fifo FIFO object
 */
#ifndef sys_port_trace_k_fifo_init_enter
#define sys_port_trace_k_fifo_init_enter(fifo)
#endif

/**
 * @brief Trace initialization of FIFO Queue exit
 * @param fifo FIFO object
 */
#ifndef sys_port_trace_k_fifo_init_exit
#define sys_port_trace_k_fifo_init_exit(fifo)
#endif

/**
 * @brief Trace FIFO Queue cancel wait entry
 * @param fifo FIFO object
 */
#ifndef sys_port_trace_k_fifo_cancel_wait_enter
#define sys_port_trace_k_fifo_cancel_wait_enter(fifo)
#endif

/**
 * @brief Trace FIFO Queue cancel wait exit
 * @param fifo FIFO object
 */
#ifndef sys_port_trace_k_fifo_cancel_wait_exit
#define sys_port_trace_k_fifo_cancel_wait_exit(fifo)
#endif

/**
 * @brief Trace FIFO Queue put entry
 * @param fifo FIFO object
 * @param data Data item
 */
#ifndef sys_port_trace_k_fifo_put_enter
#define sys_port_trace_k_fifo_put_enter(fifo, data)
#endif

/**
 * @brief Trace FIFO Queue put exit
 * @param fifo FIFO object
 * @param data Data item
 */
#ifndef sys_port_trace_k_fifo_put_exit
#define sys_port_trace_k_fifo_put_exit(fifo, data)
#endif

/**
 * @brief Trace FIFO Queue alloc put entry
 * @param fifo FIFO object
 * @param data Data item
 */
#ifndef sys_port_trace_k_fifo_alloc_put_enter
#define sys_port_trace_k_fifo_alloc_put_enter(fifo, data)
#endif

/**
 * @brief Trace FIFO Queue alloc put exit
 * @param fifo FIFO object
 * @param data Data item
 * @param ret Return value
 */
#ifndef sys_port_trace_k_fifo_alloc_put_exit
#define sys_port_trace_k_fifo_alloc_put_exit(fifo, data, ret)
#endif

/**
 * @brief Trace FIFO Queue put list entry
 * @param fifo FIFO object
 * @param head First ll-node
 * @param tail Last ll-node
 */
#ifndef sys_port_trace_k_fifo_put_list_enter
#define sys_port_trace_k_fifo_put_list_enter(fifo, head, tail)
#endif

/**
 * @brief Trace FIFO Queue put list exit
 * @param fifo FIFO object
 * @param head First ll-node
 * @param tail Last ll-node
 */
#ifndef sys_port_trace_k_fifo_put_list_exit
#define sys_port_trace_k_fifo_put_list_exit(fifo, head, tail)
#endif

/**
 * @brief Trace FIFO Queue put slist entry
 * @param fifo FIFO object
 * @param list Syslist object
 */
#ifndef sys_port_trace_k_fifo_put_slist_enter
#define sys_port_trace_k_fifo_put_slist_enter(fifo, list)
#endif

/**
 * @brief Trace FIFO Queue put slist exit
 * @param fifo FIFO object
 * @param list Syslist object
 */
#ifndef sys_port_trace_k_fifo_put_slist_exit
#define sys_port_trace_k_fifo_put_slist_exit(fifo, list)
#endif

/**
 * @brief Trace FIFO Queue get entry
 * @param fifo FIFO object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_fifo_get_enter
#define sys_port_trace_k_fifo_get_enter(fifo, timeout)
#endif

/**
 * @brief Trace FIFO Queue get exit
 * @param fifo FIFO object
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_fifo_get_exit
#define sys_port_trace_k_fifo_get_exit(fifo, timeout, ret)
#endif

/**
 * @brief Trace FIFO Queue peek head entry
 * @param fifo FIFO object
 */
#ifndef sys_port_trace_k_fifo_peek_head_enter
#define sys_port_trace_k_fifo_peek_head_enter(fifo)
#endif

/**
 * @brief Trace FIFO Queue peek head exit
 * @param fifo FIFO object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_fifo_peek_head_exit
#define sys_port_trace_k_fifo_peek_head_exit(fifo, ret)
#endif

/**
 * @brief Trace FIFO Queue peek tail entry
 * @param fifo FIFO object
 */
#ifndef sys_port_trace_k_fifo_peek_tail_enter
#define sys_port_trace_k_fifo_peek_tail_enter(fifo)
#endif

/**
 * @brief Trace FIFO Queue peek tail exit
 * @param fifo FIFO object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_fifo_peek_tail_exit
#define sys_port_trace_k_fifo_peek_tail_exit(fifo, ret)
#endif

/** @} */ /* end of subsys_tracing_apis_fifo */

/**
 * @brief Tracing hooks for LIFO events
 * @defgroup subsys_tracing_apis_lifo LIFO
 * @{
 */

/**
 * @brief Trace initialization of LIFO Queue entry
 * @param lifo LIFO object
 */
#ifndef sys_port_trace_k_lifo_init_enter
#define sys_port_trace_k_lifo_init_enter(lifo)
#endif

/**
 * @brief Trace initialization of LIFO Queue exit
 * @param lifo LIFO object
 */
#ifndef sys_port_trace_k_lifo_init_exit
#define sys_port_trace_k_lifo_init_exit(lifo)
#endif

/**
 * @brief Trace LIFO Queue put entry
 * @param lifo LIFO object
 * @param data Data item
 */
#ifndef sys_port_trace_k_lifo_put_enter
#define sys_port_trace_k_lifo_put_enter(lifo, data)
#endif

/**
 * @brief Trace LIFO Queue put exit
 * @param lifo LIFO object
 * @param data Data item
 */
#ifndef sys_port_trace_k_lifo_put_exit
#define sys_port_trace_k_lifo_put_exit(lifo, data)
#endif

/**
 * @brief Trace LIFO Queue alloc put entry
 * @param lifo LIFO object
 * @param data Data item
 */
#ifndef sys_port_trace_k_lifo_alloc_put_enter
#define sys_port_trace_k_lifo_alloc_put_enter(lifo, data)
#endif

/**
 * @brief Trace LIFO Queue alloc put exit
 * @param lifo LIFO object
 * @param data Data item
 * @param ret Return value
 */
#ifndef sys_port_trace_k_lifo_alloc_put_exit
#define sys_port_trace_k_lifo_alloc_put_exit(lifo, data, ret)
#endif

/**
 * @brief Trace LIFO Queue get entry
 * @param lifo LIFO object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_lifo_get_enter
#define sys_port_trace_k_lifo_get_enter(lifo, timeout)
#endif

/**
 * @brief Trace LIFO Queue get exit
 * @param lifo LIFO object
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_lifo_get_exit
#define sys_port_trace_k_lifo_get_exit(lifo, timeout, ret)
#endif

/** @} */ /* end of subsys_tracing_apis_lifo */

/**
 * @brief Tracing hooks for stack events
 * @defgroup subsys_tracing_apis_stack Stack
 * @{
 */

/**
 * @brief Trace initialization of Stack
 * @param stack Stack object
 */
#ifndef sys_port_trace_k_stack_init
#define sys_port_trace_k_stack_init(stack)
#endif

/**
 * @brief Trace Stack alloc init attempt entry
 * @param stack Stack object
 */
#ifndef sys_port_trace_k_stack_alloc_init_enter
#define sys_port_trace_k_stack_alloc_init_enter(stack)
#endif

/**
 * @brief Trace Stack alloc init outcome
 * @param stack Stack object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_stack_alloc_init_exit
#define sys_port_trace_k_stack_alloc_init_exit(stack, ret)
#endif

/**
 * @brief Trace Stack cleanup attempt entry
 * @param stack Stack object
 */
#ifndef sys_port_trace_k_stack_cleanup_enter
#define sys_port_trace_k_stack_cleanup_enter(stack)
#endif

/**
 * @brief Trace Stack cleanup outcome
 * @param stack Stack object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_stack_cleanup_exit
#define sys_port_trace_k_stack_cleanup_exit(stack, ret)
#endif

/**
 * @brief Trace Stack push attempt entry
 * @param stack Stack object
 */
#ifndef sys_port_trace_k_stack_push_enter
#define sys_port_trace_k_stack_push_enter(stack)
#endif

/**
 * @brief Trace Stack push attempt outcome
 * @param stack Stack object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_stack_push_exit
#define sys_port_trace_k_stack_push_exit(stack, ret)
#endif

/**
 * @brief Trace Stack pop attempt entry
 * @param stack Stack object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_stack_pop_enter
#define sys_port_trace_k_stack_pop_enter(stack, timeout)
#endif

/**
 * @brief Trace Stack pop attempt blocking
 * @param stack Stack object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_stack_pop_blocking
#define sys_port_trace_k_stack_pop_blocking(stack, timeout)
#endif

/**
 * @brief Trace Stack pop attempt outcome
 * @param stack Stack object
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_stack_pop_exit
#define sys_port_trace_k_stack_pop_exit(stack, timeout, ret)
#endif

/** @} */ /* end of subsys_tracing_apis_stack */

/**
 * @brief Tracing hooks for message queue events
 * @defgroup subsys_tracing_apis_msgq Message queue
 * @{
 */

/**
 * @brief Trace initialization of Message Queue
 * @param msgq Message Queue object
 */
#ifndef sys_port_trace_k_msgq_init
#define sys_port_trace_k_msgq_init(msgq)
#endif

/**
 * @brief Trace Message Queue alloc init attempt entry
 * @param msgq Message Queue object
 */
#ifndef sys_port_trace_k_msgq_alloc_init_enter
#define sys_port_trace_k_msgq_alloc_init_enter(msgq)
#endif

/**
 * @brief Trace Message Queue alloc init attempt outcome
 * @param msgq Message Queue object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_msgq_alloc_init_exit
#define sys_port_trace_k_msgq_alloc_init_exit(msgq, ret)
#endif

/**
 * @brief Trace Message Queue cleanup attempt entry
 * @param msgq Message Queue object
 */
#ifndef sys_port_trace_k_msgq_cleanup_enter
#define sys_port_trace_k_msgq_cleanup_enter(msgq)
#endif

/**
 * @brief Trace Message Queue cleanup attempt outcome
 * @param msgq Message Queue object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_msgq_cleanup_exit
#define sys_port_trace_k_msgq_cleanup_exit(msgq, ret)
#endif

/**
 * @brief Trace Message Queue put attempt entry
 * @param msgq Message Queue object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_msgq_put_enter
#define sys_port_trace_k_msgq_put_enter(msgq, timeout)
#endif

/**
 * @brief Trace Message Queue put attempt blocking
 * @param msgq Message Queue object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_msgq_put_blocking
#define sys_port_trace_k_msgq_put_blocking(msgq, timeout)
#endif

/**
 * @brief Trace Message Queue put attempt outcome
 * @param msgq Message Queue object
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_msgq_put_exit
#define sys_port_trace_k_msgq_put_exit(msgq, timeout, ret)
#endif

/**
 * @brief Trace Message Queue put at front attempt entry
 * @param msgq Message Queue object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_msgq_put_front_enter
#define sys_port_trace_k_msgq_put_front_enter(msgq, timeout)
#endif

/**
 * @brief Trace Message Queue put at front attempt blocking
 * @param msgq Message Queue object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_msgq_put_front_blocking
#define sys_port_trace_k_msgq_put_front_blocking(msgq, timeout)
#endif

/**
 * @brief Trace Message Queue put at front attempt outcome
 * @param msgq Message Queue object
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_msgq_put_front_exit
#define sys_port_trace_k_msgq_put_front_exit(msgq, timeout, ret)
#endif

/**
 * @brief Trace Message Queue get attempt entry
 * @param msgq Message Queue object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_msgq_get_enter
#define sys_port_trace_k_msgq_get_enter(msgq, timeout)
#endif

/**
 * @brief Trace Message Queue get attempt blockings
 * @param msgq Message Queue object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_msgq_get_blocking
#define sys_port_trace_k_msgq_get_blocking(msgq, timeout)
#endif

/**
 * @brief Trace Message Queue get attempt outcome
 * @param msgq Message Queue object
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_msgq_get_exit
#define sys_port_trace_k_msgq_get_exit(msgq, timeout, ret)
#endif

/**
 * @brief Trace Message Queue peek
 * @param msgq Message Queue object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_msgq_peek
#define sys_port_trace_k_msgq_peek(msgq, ret)
#endif

/**
 * @brief Trace Message Queue purge
 * @param msgq Message Queue object
 */
#ifndef sys_port_trace_k_msgq_purge
#define sys_port_trace_k_msgq_purge(msgq)
#endif

/** @} */ /* end of subsys_tracing_apis_msgq */

/**
 * @brief Tracing hooks for mailbox events
 * @defgroup subsys_tracing_apis_mbox Mailbox
 * @{
 */

/**
 * @brief Trace initialization of Mailbox
 * @param mbox Mailbox object
 */
#ifndef sys_port_trace_k_mbox_init
#define sys_port_trace_k_mbox_init(mbox)
#endif

/**
 * @brief Trace Mailbox message put attempt entry
 * @param mbox Mailbox object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_mbox_message_put_enter
#define sys_port_trace_k_mbox_message_put_enter(mbox, timeout)
#endif

/**
 * @brief Trace Mailbox message put attempt blocking
 * @param mbox Mailbox object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_mbox_message_put_blocking
#define sys_port_trace_k_mbox_message_put_blocking(mbox, timeout)
#endif

/**
 * @brief Trace Mailbox message put attempt outcome
 * @param mbox Mailbox object
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_mbox_message_put_exit
#define sys_port_trace_k_mbox_message_put_exit(mbox, timeout, ret)
#endif

/**
 * @brief Trace Mailbox put attempt entry
 * @param mbox Mailbox object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_mbox_put_enter
#define sys_port_trace_k_mbox_put_enter(mbox, timeout)
#endif

/**
 * @brief Trace Mailbox put attempt blocking
 * @param mbox Mailbox object
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_mbox_put_exit
#define sys_port_trace_k_mbox_put_exit(mbox, timeout, ret)
#endif

/**
 * @brief Trace Mailbox async put entry
 * @param mbox Mailbox object
 * @param sem Semaphore object
 */
#ifndef sys_port_trace_k_mbox_async_put_enter
#define sys_port_trace_k_mbox_async_put_enter(mbox, sem)
#endif

/**
 * @brief Trace Mailbox async put exit
 * @param mbox Mailbox object
 * @param sem Semaphore object
 */
#ifndef sys_port_trace_k_mbox_async_put_exit
#define sys_port_trace_k_mbox_async_put_exit(mbox, sem)
#endif

/**
 * @brief Trace Mailbox get attempt entry
 * @param mbox Mailbox entry
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_mbox_get_enter
#define sys_port_trace_k_mbox_get_enter(mbox, timeout)
#endif

/**
 * @brief Trace Mailbox get attempt blocking
 * @param mbox Mailbox entry
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_mbox_get_blocking
#define sys_port_trace_k_mbox_get_blocking(mbox, timeout)
#endif

/**
 * @brief Trace Mailbox get attempt outcome
 * @param mbox Mailbox entry
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_mbox_get_exit
#define sys_port_trace_k_mbox_get_exit(mbox, timeout, ret)
#endif

/**
 * @brief Trace Mailbox data get
 * @brief rx_msg Receive Message object
 */
#ifndef sys_port_trace_k_mbox_data_get
#define sys_port_trace_k_mbox_data_get(rx_msg)
#endif

/** @} */ /* end of subsys_tracing_apis_mbox */

/**
 * @brief Tracing hooks for pipe events
 * @defgroup subsys_tracing_apis_pipe Pipe
 * @{
 */

/**
 * @brief Trace initialization of Pipe
 * @param pipe Pipe object
 * @param buffer data buffer
 * @param size data buffer size
 */
#ifndef sys_port_trace_k_pipe_init
#define sys_port_trace_k_pipe_init(pipe, buffer, size)
#endif

/**
 * @brief Trace Pipe reset entry
 * @param pipe Pipe object
 */
#ifndef sys_port_trace_k_pipe_reset_enter
#define sys_port_trace_k_pipe_reset_enter(pipe)
#endif

/**
 * @brief Trace Pipe reset exit
 * @param pipe Pipe object
 */
#ifndef sys_port_trace_k_pipe_reset_exit
#define sys_port_trace_k_pipe_reset_exit(pipe)
#endif

/**
 * @brief Trace Pipe close entry
 * @param pipe Pipe object
 */
#ifndef sys_port_trace_k_pipe_close_enter
#define sys_port_trace_k_pipe_close_enter(pipe)
#endif

/**
 * @brief Trace Pipe close exit
 * @param pipe Pipe object
 */
#ifndef sys_port_trace_k_pipe_close_exit
#define sys_port_trace_k_pipe_close_exit(pipe)
#endif

/**
 * @brief Trace Pipe write attempt entry
 * @param pipe Pipe object
 * @param data pointer to data
 * @param len length of data
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_pipe_write_enter
#define sys_port_trace_k_pipe_write_enter(pipe, data, len, timeout)
#endif

/**
 * @brief Trace Pipe write attempt blocking
 * @param pipe Pipe object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_pipe_write_blocking
#define sys_port_trace_k_pipe_write_blocking(pipe, timeout)
#endif

/**
 * @brief Trace Pipe write attempt outcome
 * @param pipe Pipe object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_pipe_write_exit
#define sys_port_trace_k_pipe_write_exit(pipe, ret)
#endif

/**
 * @brief Trace Pipe read attempt entry
 * @param pipe Pipe object
 * @param data Pointer to data
 * @param len Length of data
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_pipe_read_enter
#define sys_port_trace_k_pipe_read_enter(pipe, data, len, timeout)
#endif

/**
 * @brief Trace Pipe read attempt blocking
 * @param pipe Pipe object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_pipe_read_blocking
#define sys_port_trace_k_pipe_read_blocking(pipe, timeout)
#endif

/**
 * @brief Trace Pipe read attempt outcome
 * @param pipe Pipe object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_pipe_read_exit
#define sys_port_trace_k_pipe_read_exit(pipe, ret)
#endif

/** @} */ /* end of subsys_tracing_apis_pipe */

/**
 * @brief Tracing hooks for heap events
 * @defgroup subsys_tracing_apis_heap Heap
 * @{
 */

/**
 * @brief Trace initialization of Heap
 * @param h Heap object
 */
#ifndef sys_port_trace_k_heap_init
#define sys_port_trace_k_heap_init(h)
#endif

/**
 * @brief Trace Heap aligned alloc attempt entry
 * @param h Heap object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_heap_aligned_alloc_enter
#define sys_port_trace_k_heap_aligned_alloc_enter(h, timeout)
#endif

/**
 * @brief Trace Heap align alloc attempt blocking
 * @param h Heap object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_heap_alloc_helper_blocking
#define sys_port_trace_k_heap_alloc_helper_blocking(h, timeout)
#endif

/**
 * @brief Trace Heap align alloc attempt outcome
 * @param h Heap object
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_heap_aligned_alloc_exit
#define sys_port_trace_k_heap_aligned_alloc_exit(h, timeout, ret)
#endif

/**
 * @brief Trace Heap alloc enter
 * @param h Heap object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_heap_alloc_enter
#define sys_port_trace_k_heap_alloc_enter(h, timeout)
#endif

/**
 * @brief Trace Heap alloc exit
 * @param h Heap object
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_heap_alloc_exit
#define sys_port_trace_k_heap_alloc_exit(h, timeout, ret)
#endif

/**
 * @brief Trace Heap calloc enter
 * @param h Heap object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_heap_calloc_enter
#define sys_port_trace_k_heap_calloc_enter(h, timeout)
#endif

/**
 * @brief Trace Heap calloc exit
 * @param h Heap object
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_heap_calloc_exit
#define sys_port_trace_k_heap_calloc_exit(h, timeout, ret)
#endif

/**
 * @brief Trace Heap free
 * @param h Heap object
 */
#ifndef sys_port_trace_k_heap_free
#define sys_port_trace_k_heap_free(h)
#endif

/**
 * @brief Trace Heap realloc enter
 * @param h Heap object
 * @param ptr Pointer to reallocate
 * @param bytes Bytes to reallocate
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_heap_realloc_enter
#define sys_port_trace_k_heap_realloc_enter(h, ptr, bytes, timeout)
#endif

/**
 * @brief Trace Heap realloc exit
 * @param h Heap object
 * @param ptr Pointer to reallocate
 * @param bytes Bytes to reallocate
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_heap_realloc_exit
#define sys_port_trace_k_heap_realloc_exit(h, ptr, bytes, timeout, ret)
#endif

/**
 * @brief Trace System Heap aligned alloc enter
 * @param heap Heap object
 */
#ifndef sys_port_trace_k_heap_sys_k_aligned_alloc_enter
#define sys_port_trace_k_heap_sys_k_aligned_alloc_enter(heap)
#endif

/**
 * @brief Trace System Heap aligned alloc exit
 * @param heap Heap object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_heap_sys_k_aligned_alloc_exit
#define sys_port_trace_k_heap_sys_k_aligned_alloc_exit(heap, ret)
#endif

/**
 * @brief Trace System Heap aligned alloc enter
 * @param heap Heap object
 */
#ifndef sys_port_trace_k_heap_sys_k_malloc_enter
#define sys_port_trace_k_heap_sys_k_malloc_enter(heap)
#endif

/**
 * @brief Trace System Heap aligned alloc exit
 * @param heap Heap object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_heap_sys_k_malloc_exit
#define sys_port_trace_k_heap_sys_k_malloc_exit(heap, ret)
#endif

/**
 * @brief Trace System Heap free entry
 * @param heap Heap object
 * @param heap_ref Heap reference
 */
#ifndef sys_port_trace_k_heap_sys_k_free_enter
#define sys_port_trace_k_heap_sys_k_free_enter(heap, heap_ref)
#endif

/**
 * @brief Trace System Heap free exit
 * @param heap Heap object
 * @param heap_ref Heap reference
 */
#ifndef sys_port_trace_k_heap_sys_k_free_exit
#define sys_port_trace_k_heap_sys_k_free_exit(heap, heap_ref)
#endif

/**
 * @brief Trace System heap calloc enter
 * @param heap
 */
#ifndef sys_port_trace_k_heap_sys_k_calloc_enter
#define sys_port_trace_k_heap_sys_k_calloc_enter(heap)
#endif

/**
 * @brief Trace System heap calloc exit
 * @param heap Heap object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_heap_sys_k_calloc_exit
#define sys_port_trace_k_heap_sys_k_calloc_exit(heap, ret)
#endif

/**
 * @brief Trace System heap realloc enter
 * @param heap
 * @param ptr
 */
#ifndef sys_port_trace_k_heap_sys_k_realloc_enter
#define sys_port_trace_k_heap_sys_k_realloc_enter(heap, ptr)
#endif

/**
 * @brief Trace System heap realloc exit
 * @param heap Heap object
 * @param ptr Memory pointer
 * @param ret Return value
 */
#ifndef sys_port_trace_k_heap_sys_k_realloc_exit
#define sys_port_trace_k_heap_sys_k_realloc_exit(heap, ptr, ret)
#endif

/** @} */ /* end of subsys_tracing_apis_heap */

/**
 * @brief Tracing hooks for memory slab events
 * @defgroup subsys_tracing_apis_mslab Memory slab
 * @{
 */

/**
 * @brief Trace initialization of Memory Slab
 * @param slab Memory Slab object
 * @param rc Return value
 */
#ifndef sys_port_trace_k_mem_slab_init
#define sys_port_trace_k_mem_slab_init(slab, rc)
#endif

/**
 * @brief Trace Memory Slab alloc attempt entry
 * @param slab Memory Slab object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_mem_slab_alloc_enter
#define sys_port_trace_k_mem_slab_alloc_enter(slab, timeout)
#endif

/**
 * @brief Trace Memory Slab alloc attempt blocking
 * @param slab Memory Slab object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_mem_slab_alloc_blocking
#define sys_port_trace_k_mem_slab_alloc_blocking(slab, timeout)
#endif

/**
 * @brief Trace Memory Slab alloc attempt outcome
 * @param slab Memory Slab object
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_mem_slab_alloc_exit
#define sys_port_trace_k_mem_slab_alloc_exit(slab, timeout, ret)
#endif

/**
 * @brief Trace Memory Slab free entry
 * @param slab Memory Slab object
 */
#ifndef sys_port_trace_k_mem_slab_free_enter
#define sys_port_trace_k_mem_slab_free_enter(slab)
#endif

/**
 * @brief Trace Memory Slab free exit
 * @param slab Memory Slab object
 */
#ifndef sys_port_trace_k_mem_slab_free_exit
#define sys_port_trace_k_mem_slab_free_exit(slab)
#endif

/** @} */ /* end of subsys_tracing_apis_mslab */

/**
 * @brief Tracing hooks for timer events
 * @defgroup subsys_tracing_apis_timer Timer
 * @{
 */

/**
 * @brief Trace initialization of Timer
 * @param timer Timer object
 */
#ifndef sys_port_trace_k_timer_init
#define sys_port_trace_k_timer_init(timer)
#endif

/**
 * @brief Trace Timer start
 * @param timer Timer object
 * @param duration Timer duration
 * @param period Timer period
 */
#ifndef sys_port_trace_k_timer_start
#define sys_port_trace_k_timer_start(timer, duration, period)
#endif

/**
 * @brief Trace Timer stop
 * @param timer Timer object
 */
#ifndef sys_port_trace_k_timer_stop
#define sys_port_trace_k_timer_stop(timer)
#endif

/**
 * @brief Trace Timer status sync entry
 * @param timer Timer object
 */
#ifndef sys_port_trace_k_timer_status_sync_enter
#define sys_port_trace_k_timer_status_sync_enter(timer)
#endif

/**
 * @brief Trace Timer Status sync blocking
 * @param timer Timer object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_timer_status_sync_blocking
#define sys_port_trace_k_timer_status_sync_blocking(timer, timeout)
#endif

/**
 * @brief Trace Time Status sync outcome
 * @param timer Timer object
 * @param result Return value
 */
#ifndef sys_port_trace_k_timer_status_sync_exit
#define sys_port_trace_k_timer_status_sync_exit(timer, result)
#endif

/**
 * @brief Trace Timer expiry entry
 * @param timer Timer object
 */
#ifndef sys_port_trace_k_timer_expiry_enter
#define sys_port_trace_k_timer_expiry_enter(timer)
#endif

/**
 * @brief Trace Timer expiry exit
 * @param timer Timer object
 */
#ifndef sys_port_trace_k_timer_expiry_exit
#define sys_port_trace_k_timer_expiry_exit(timer)
#endif

/**
 * @brief Trace Timer stop function expiry entry
 * @param timer Timer object
 */
#ifndef sys_port_trace_k_timer_stop_fn_expiry_enter
#define sys_port_trace_k_timer_stop_fn_expiry_enter(timer)
#endif

/**
 * @brief Trace Timer stop function expiry exit
 * @param timer Timer object
 */
#ifndef sys_port_trace_k_timer_stop_fn_expiry_exit
#define sys_port_trace_k_timer_stop_fn_expiry_exit(timer)
#endif

/**
 * @brief Trace Timer cleanup attempt entry
 * @param timer Timer object
 */
#ifndef sys_port_trace_k_timer_cleanup_enter
#define sys_port_trace_k_timer_cleanup_enter(timer)
#endif

/**
 * @brief Trace Timer cleanup outcome
 * @param timer Timer object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_timer_cleanup_exit
#define sys_port_trace_k_timer_cleanup_exit(timer, ret)
#endif

/** @} */ /* end of subsys_tracing_apis_timer */

/**
 * @brief Tracing hooks for event events
 * @defgroup subsys_tracing_apis_event Event
 * @{
 */

/**
 * @brief Trace initialisation of an Event
 * @param event Event object
 */
#ifndef sys_port_trace_k_event_init
#define sys_port_trace_k_event_init(event)
#endif

/**
 * @brief Trace posting of an Event call entry
 * @param event Event object
 * @param events Set of posted events
 * @param events_mask Mask to apply against posted events
 */
#ifndef sys_port_trace_k_event_post_enter
#define sys_port_trace_k_event_post_enter(event, events, events_mask)
#endif

/**
 * @brief Trace posting of an Event call exit
 * @param event Event object
 * @param events Set of posted events
 * @param events_mask Mask to apply against posted events
 */
#ifndef sys_port_trace_k_event_post_exit
#define sys_port_trace_k_event_post_exit(event, events, events_mask)
#endif

/**
 * @brief Trace waiting of an Event call entry
 * @param event Event object
 * @param events Set of events for which to wait
 * @param options Event wait options
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_event_wait_enter
#define sys_port_trace_k_event_wait_enter(event, events, options, timeout)
#endif

/**
 * @brief Trace waiting of an Event call exit
 * @param event Event object
 * @param events Set of events for which to wait
 * @param options Event wait options
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_event_wait_blocking
#define sys_port_trace_k_event_wait_blocking(event, events, options, timeout)
#endif

/**
 * @brief Trace waiting of an Event call exit
 * @param event Event object
 * @param events Set of events for which to wait
 * @param ret Set of received events
 */
#ifndef sys_port_trace_k_event_wait_exit
#define sys_port_trace_k_event_wait_exit(event, events, ret)
#endif

/** @} */ /* end of subsys_tracing_apis_event */

/**
 * @brief Tracing hooks for system power management events
 * @defgroup subsys_tracing_apis_pm_system System PM
 * @{
 */

/**
 * @brief Trace system suspend call entry.
 * @param ticks Ticks.
 */
#ifndef sys_port_trace_pm_system_suspend_enter
#define sys_port_trace_pm_system_suspend_enter(ticks)
#endif

/**
 * @brief Trace system suspend call exit.
 * @param ticks Ticks.
 * @param state PM state.
 */
#ifndef sys_port_trace_pm_system_suspend_exit
#define sys_port_trace_pm_system_suspend_exit(ticks, state)
#endif

/** @} */ /* end of subsys_tracing_apis_pm_system */

/**
 * @brief Tracing hooks for device runtime power management events
 * @defgroup subsys_tracing_apis_pm_device_runtime PM Device Runtime
 * @{
 */

/**
 * @brief Trace getting a device call entry.
 * @param dev Device instance.
 */
#ifndef sys_port_trace_pm_device_runtime_get_enter
#define sys_port_trace_pm_device_runtime_get_enter(dev)
#endif

/**
 * @brief Trace getting a device call exit.
 * @param dev Device instance.
 * @param ret Return value.
 */
#ifndef sys_port_trace_pm_device_runtime_get_exit
#define sys_port_trace_pm_device_runtime_get_exit(dev, ret)
#endif

/**
 * @brief Trace putting a device call entry.
 * @param dev Device instance.
 */
#ifndef sys_port_trace_pm_device_runtime_put_enter
#define sys_port_trace_pm_device_runtime_put_enter(dev)
#endif

/**
 * @brief Trace putting a device call exit.
 * @param dev Device instance.
 * @param ret Return value.
 */
#ifndef sys_port_trace_pm_device_runtime_put_exit
#define sys_port_trace_pm_device_runtime_put_exit(dev, ret)
#endif

/**
 * @brief Trace putting a device (asynchronously) call entry.
 * @param dev Device instance.
 * @param delay Time to delay the operation
 */
#ifndef sys_port_trace_pm_device_runtime_put_async_enter
#define sys_port_trace_pm_device_runtime_put_async_enter(dev, delay)
#endif

/**
 * @brief Trace putting a device (asynchronously) call exit.
 * @param dev Device instance.
 * @param delay Time to delay the operation.
 * @param ret Return value.
 */
#ifndef sys_port_trace_pm_device_runtime_put_async_exit
#define sys_port_trace_pm_device_runtime_put_async_exit(dev, delay, ret)
#endif

/**
 * @brief Trace enabling device runtime PM call entry.
 * @param dev Device instance.
 */
#ifndef sys_port_trace_pm_device_runtime_enable_enter
#define sys_port_trace_pm_device_runtime_enable_enter(dev)
#endif

/**
 * @brief Trace enabling device runtime PM call exit.
 * @param dev Device instance.
 * @param ret Return value.
 */
#ifndef sys_port_trace_pm_device_runtime_enable_exit
#define sys_port_trace_pm_device_runtime_enable_exit(dev, ret)
#endif

/**
 * @brief Trace disabling device runtime PM call entry.
 * @param dev Device instance.
 */
#ifndef sys_port_trace_pm_device_runtime_disable_enter
#define sys_port_trace_pm_device_runtime_disable_enter(dev)
#endif

/**
 * @brief Trace disabling device runtime PM call exit.
 * @param dev Device instance.
 * @param ret Return value.
 */
#ifndef sys_port_trace_pm_device_runtime_disable_exit
#define sys_port_trace_pm_device_runtime_disable_exit(dev, ret)
#endif

/** @} */ /* end of subsys_tracing_apis_pm_device_runtime */

/**
 * @brief Tracing hooks for network events
 * @defgroup subsys_tracing_apis_net Network
 * @{
 */

/**
 * @brief Trace network data receive
 * @param iface Network interface
 * @param pkt Received network packet
 */
#ifndef sys_port_trace_net_recv_data_enter
#define sys_port_trace_net_recv_data_enter(iface, pkt)
#endif

/**
 * @brief Trace network data receive attempt
 * @param iface Network interface
 * @param pkt Received network packet
 * @param ret Return value
 */
#ifndef sys_port_trace_net_recv_data_exit
#define sys_port_trace_net_recv_data_exit(iface, pkt, ret)
#endif

/**
 * @brief Trace network data send
 * @param pkt Network packet to send
 */
#ifndef sys_port_trace_net_send_data_enter
#define sys_port_trace_net_send_data_enter(pkt)
#endif

/**
 * @brief Trace network data send attempt
 * @param pkt Received network packet
 * @param ret Return value
 */
#ifndef sys_port_trace_net_send_data_exit
#define sys_port_trace_net_send_data_exit(pkt, ret)
#endif

/**
 * @brief Trace network data receive time
 * @param pkt Received network packet
 * @param end_time When the RX processing stopped for this pkt (in ticks)
 */
#ifndef sys_port_trace_net_rx_time
#define sys_port_trace_net_rx_time(pkt, end_time)
#endif

/**
 * @brief Trace network data sent time
 * @param pkt Sent network packet
 * @param end_time When the TX processing stopped for this pkt (in ticks)
 */
#ifndef sys_port_trace_net_tx_time
#define sys_port_trace_net_tx_time(pkt, end_time)
#endif

/** @} */ /* end of subsys_tracing_apis_net */

/**
 * @brief Tracing hooks for network socket events
 * @defgroup subsys_tracing_apis_socket Network socket
 * @{
 */

/**
 * @brief Trace init of network sockets
 * @param socket Network socket is returned
 * @param family Socket address family
 * @param type Socket type
 * @param proto Socket protocol
 */
#ifndef sys_port_trace_socket_init
#define sys_port_trace_socket_init(socket, family, type, proto)
#endif

/**
 * @brief Trace close of network sockets
 * @param socket Socket object
 */
#ifndef sys_port_trace_socket_close_enter
#define sys_port_trace_socket_close_enter(socket)
#endif

/**
 * @brief Trace network socket close attempt
 * @param socket Socket object
 * @param ret Return value
 */
#ifndef sys_port_trace_socket_close_exit
#define sys_port_trace_socket_close_exit(socket, ret)
#endif

/**
 * @brief Trace shutdown of network sockets
 * @param socket Socket object
 * @param how Socket shutdown type
 */
#ifndef sys_port_trace_socket_shutdown_enter
#define sys_port_trace_socket_shutdown_enter(socket, how)
#endif

/**
 * @brief Trace network socket shutdown attempt
 * @param socket Socket object
 * @param ret Return value
 */
#ifndef sys_port_trace_socket_shutdown_exit
#define sys_port_trace_socket_shutdown_exit(socket, ret)
#endif

/**
 * @brief Trace bind of network sockets
 * @param socket Socket object
 * @param addr Network address to bind
 * @param addrlen Address length
 */
#ifndef sys_port_trace_socket_bind_enter
#define sys_port_trace_socket_bind_enter(socket, addr, addrlen)
#endif

/**
 * @brief Trace network socket bind attempt
 * @param socket Socket object
 * @param ret Return value
 */
#ifndef sys_port_trace_socket_bind_exit
#define sys_port_trace_socket_bind_exit(socket, ret)
#endif

/**
 * @brief Trace connect of network sockets
 * @param socket Socket object
 * @param addr Network address to bind
 * @param addrlen Address length
 */
#ifndef sys_port_trace_socket_connect_enter
#define sys_port_trace_socket_connect_enter(socket, addr, addrlen)
#endif

/**
 * @brief Trace network socket connect attempt
 * @param socket Socket object
 * @param ret Return value
 */
#ifndef sys_port_trace_socket_connect_exit
#define sys_port_trace_socket_connect_exit(socket, ret)
#endif

/**
 * @brief Trace listen of network sockets
 * @param socket Socket object
 * @param backlog Socket backlog length
 */
#ifndef sys_port_trace_socket_listen_enter
#define sys_port_trace_socket_listen_enter(socket, backlog)
#endif

/**
 * @brief Trace network socket listen attempt
 * @param socket Socket object
 * @param ret Return value
 */
#ifndef sys_port_trace_socket_listen_exit
#define sys_port_trace_socket_listen_exit(socket, ret)
#endif

/**
 * @brief Trace accept of network sockets
 * @param socket Socket object
 */
#ifndef sys_port_trace_socket_accept_enter
#define sys_port_trace_socket_accept_enter(socket)
#endif

/**
 * @brief Trace network socket accept attempt
 * @param socket Socket object
 * @param addr Peer network address
 * @param addrlen Network address length
 * @param ret Return value
 */
#ifndef sys_port_trace_socket_accept_exit
#define sys_port_trace_socket_accept_exit(socket, addr, addrlen, ret)
#endif

/**
 * @brief Trace sendto of network sockets
 * @param socket Socket object
 * @param len Length of the data to send
 * @param flags Flags for this send operation
 * @param dest_addr Destination network address
 * @param addrlen Network address length
 */
#ifndef sys_port_trace_socket_sendto_enter
#define sys_port_trace_socket_sendto_enter(socket, len, flags, dest_addr, addrlen)
#endif

/**
 * @brief Trace network socket sendto attempt
 * @param socket Socket object
 * @param ret Return value
 */
#ifndef sys_port_trace_socket_sendto_exit
#define sys_port_trace_socket_sendto_exit(socket, ret)
#endif

/**
 * @brief Trace sendmsg of network sockets
 * @param socket Socket object
 * @param msg Data to send
 * @param flags Flags for this send operation
 */
#ifndef sys_port_trace_socket_sendmsg_enter
#define sys_port_trace_socket_sendmsg_enter(socket, msg, flags)
#endif

/**
 * @brief Trace network socket sendmsg attempt
 * @param socket Socket object
 * @param ret Return value
 */
#ifndef sys_port_trace_socket_sendmsg_exit
#define sys_port_trace_socket_sendmsg_exit(socket, ret)
#endif

/**
 * @brief Trace recvfrom of network sockets
 * @param socket Socket object
 * @param max_len Maximum length of the data we can receive
 * @param flags Flags for this receive operation
 * @param addr Remote network address
 * @param addrlen Network address length
 */
#ifndef sys_port_trace_socket_recvfrom_enter
#define sys_port_trace_socket_recvfrom_enter(socket, max_len, flags, addr, addrlen)
#endif

/**
 * @brief Trace network socket recvfrom attempt
 * @param socket Socket object
 * @param src_addr Peer network address that send the data
 * @param addrlen Length of the network address
 * @param ret Return value
 */
#ifndef sys_port_trace_socket_recvfrom_exit
#define sys_port_trace_socket_recvfrom_exit(socket, src_addr, addrlen, ret)
#endif

/**
 * @brief Trace recvmsg of network sockets
 * @param socket Socket object
 * @param msg Message buffer to receive
 * @param flags Flags for this receive operation
 */
#ifndef sys_port_trace_socket_recvmsg_enter
#define sys_port_trace_socket_recvmsg_enter(socket, msg, flags)
#endif

/**
 * @brief Trace network socket recvmsg attempt
 * @param socket Socket object
 * @param msg Message buffer received
 * @param ret Return value
 */
#ifndef sys_port_trace_socket_recvmsg_exit
#define sys_port_trace_socket_recvmsg_exit(socket, msg, ret)
#endif

/**
 * @brief Trace fcntl of network sockets
 * @param socket Socket object
 * @param cmd Command to set for this socket
 * @param flags Flags for this receive operation
 */
#ifndef sys_port_trace_socket_fcntl_enter
#define sys_port_trace_socket_fcntl_enter(socket, cmd, flags)
#endif

/**
 * @brief Trace network socket fcntl attempt
 * @param socket Socket object
 * @param ret Return value
 */
#ifndef sys_port_trace_socket_fcntl_exit
#define sys_port_trace_socket_fcntl_exit(socket, ret)
#endif

/**
 * @brief Trace ioctl of network sockets
 * @param socket Socket object
 * @param req Request to set for this socket
 */
#ifndef sys_port_trace_socket_ioctl_enter
#define sys_port_trace_socket_ioctl_enter(socket, req)
#endif

/**
 * @brief Trace network socket ioctl attempt
 * @param socket Socket object
 * @param ret Return value
 */
#ifndef sys_port_trace_socket_ioctl_exit
#define sys_port_trace_socket_ioctl_exit(socket, ret)
#endif

/**
 * @brief Trace polling of network sockets
 * @param fds Set of socket object
 * @param nfds Number of socket objects in the set
 * @param timeout Timeout for the poll operation
 */
#ifndef sys_port_trace_socket_poll_enter
#define sys_port_trace_socket_poll_enter(fds, nfds, timeout)
#endif

/**
 * @brief Trace network socket poll attempt
 * @param fds Set of socket object
 * @param nfds Number of socket objects in the set
 * @param ret Return value
 */
#ifndef sys_port_trace_socket_poll_exit
#define sys_port_trace_socket_poll_exit(fds, nfds, ret)
#endif

/**
 * @brief Trace getsockopt of network sockets
 * @param socket Socket object
 * @param level Option level
 * @param optname Option name
 */
#ifndef sys_port_trace_socket_getsockopt_enter
#define sys_port_trace_socket_getsockopt_enter(socket, level, optname)
#endif

/**
 * @brief Trace network socket getsockopt attempt
 * @param socket Socket object
 * @param level Option level
 * @param optname Option name
 * @param optval Option value
 * @param optlen Option value length
 * @param ret Return value
 */
#ifndef sys_port_trace_socket_getsockopt_exit
#define sys_port_trace_socket_getsockopt_exit(socket, level, optname, optval, optlen, ret)
#endif

/**
 * @brief Trace setsockopt of network sockets
 * @param socket Socket object
 * @param level Option level
 * @param optname Option name
 * @param optval Option value
 * @param optlen Option value length
 */
#ifndef sys_port_trace_socket_setsockopt_enter
#define sys_port_trace_socket_setsockopt_enter(socket, level, optname, optval, optlen)
#endif

/**
 * @brief Trace network socket setsockopt attempt
 * @param socket Socket object
 * @param ret Return value
 */
#ifndef sys_port_trace_socket_setsockopt_exit
#define sys_port_trace_socket_setsockopt_exit(socket, ret)
#endif

/**
 * @brief Trace getpeername of network sockets
 * @param socket Socket object
 */
#ifndef sys_port_trace_socket_getpeername_enter
#define sys_port_trace_socket_getpeername_enter(socket)
#endif

/**
 * @brief Trace network socket getpeername attempt
 * @param socket Socket object
 * @param addr Peer socket network address
 * @param addrlen Length of the network address
 * @param ret Return value
 */
#ifndef sys_port_trace_socket_getpeername_exit
#define sys_port_trace_socket_getpeername_exit(socket, addr, addrlen, ret)
#endif

/**
 * @brief Trace getsockname of network sockets
 * @param socket Socket object
 */
#ifndef sys_port_trace_socket_getsockname_enter
#define sys_port_trace_socket_getsockname_enter(socket)
#endif

/**
 * @brief Trace network socket getsockname attempt
 * @param socket Socket object
 * @param addr Local socket network address
 * @param addrlen Length of the network address
 * @param ret Return value
 */
#ifndef sys_port_trace_socket_getsockname_exit
#define sys_port_trace_socket_getsockname_exit(socket, addr, addrlen, ret)
#endif

/**
 * @brief Trace socketpair enter call
 * @param family Network address family
 * @param type Socket type
 * @param proto Socket protocol
 * @param sv Socketpair buffer
 */
#ifndef sys_port_trace_socket_socketpair_enter
#define sys_port_trace_socket_socketpair_enter(family, type, proto, sv)
#endif

/**
 * @brief Trace network socketpair open attempt
 * @param socket_A Socketpair first socket object
 * @param socket_B Socketpair second socket object
 * @param ret Return value
 */
#ifndef sys_port_trace_socket_socketpair_exit
#define sys_port_trace_socket_socketpair_exit(socket_A, socket_B, ret)
#endif

/** @} */ /* end of subsys_tracing_apis_socket */

/*
 * Note: sys_trace_named_event() is a function-style public API (implemented as a
 * real function by backends such as CTF), not a sys_port_trace_* macro hook. Its
 * disabled-state no-op lives in tracing.h's no-format branch, not here, so that
 * this fallback (included for every backend) does not clobber a backend's function.
 */

/**
 * @brief Tracing hooks for GPIO events
 * @defgroup subsys_tracing_apis_gpio GPIO
 * @{
 */

/**
 * @brief Trace GPIO pin interrupt configure enter call
 * @param port Pointer to device structure for the driver instance
 * @param pin GPIO pin number
 * @param flags Interrupt configuration flags as defined by GPIO_INT_*
 */
#ifndef sys_port_trace_gpio_pin_interrupt_configure_enter
#define sys_port_trace_gpio_pin_interrupt_configure_enter(port, pin, flags)
#endif

/**
 * @brief Trace GPIO pin interrupt configure exit call
 * @param port Pointer to device structure for the driver instance
 * @param pin GPIO pin number
 * @param ret Return value
 */
#ifndef sys_port_trace_gpio_pin_interrupt_configure_exit
#define sys_port_trace_gpio_pin_interrupt_configure_exit(port, pin, ret)
#endif

/**
 * @brief Trace GPIO single pin configure enter call
 * @param port Pointer to device structure for the driver instance
 * @param pin GPIO pin number to configure
 * @param flags GPIO pin configuration flags
 */
#ifndef sys_port_trace_gpio_pin_configure_enter
#define sys_port_trace_gpio_pin_configure_enter(port, pin, flags)
#endif

/**
 * @brief Trace GPIO single pin configure exit call
 * @param port Pointer to device structure for the driver instance
 * @param pin GPIO pin number to configure
 * @param ret Return value
 */
#ifndef sys_port_trace_gpio_pin_configure_exit
#define sys_port_trace_gpio_pin_configure_exit(port, pin, ret)
#endif

/**
 * @brief Trace GPIO port get direction enter call
 * @param port Pointer to device structure for the driver instance
 * @param map Bitmap of pin directions to query
 * @param inputs Pointer to a variable where input directions will be stored
 * @param outputs Pointer to a variable where output directions will be stored
 */
#ifndef sys_port_trace_gpio_port_get_direction_enter
#define sys_port_trace_gpio_port_get_direction_enter(port, map, inputs, outputs)
#endif

/**
 * @brief Trace GPIO port get direction exit call
 * @param port Pointer to device structure for the driver instance
 * @param ret Return value
 */
#ifndef sys_port_trace_gpio_port_get_direction_exit
#define sys_port_trace_gpio_port_get_direction_exit(port, ret)
#endif

/**
 * @brief Trace GPIO pin gent config enter call
 * @param port Pointer to device structure for the driver instance
 * @param pin GPIO pin number to configure
 * @param flags GPIO pin configuration flags
 */
#ifndef sys_port_trace_gpio_pin_get_config_enter
#define sys_port_trace_gpio_pin_get_config_enter(port, pin, flags)
#endif

/**
 * @brief Trace GPIO pin get config exit call
 * @param port Pointer to device structure for the driver instance
 * @param pin GPIO pin number to configure
 * @param ret Return value
 */
#ifndef sys_port_trace_gpio_pin_get_config_exit
#define sys_port_trace_gpio_pin_get_config_exit(port, pin, ret)
#endif

/**
 * @brief Trace GPIO port get raw enter call
 * @param port Pointer to device structure for the driver instance
 * @param value Pointer to a variable where the raw value will be stored
 */
#ifndef sys_port_trace_gpio_port_get_raw_enter
#define sys_port_trace_gpio_port_get_raw_enter(port, value)
#endif

/**
 * @brief Trace GPIO port get raw exit call
 * @param port Pointer to device structure for the driver instance
 * @param ret Return value
 */
#ifndef sys_port_trace_gpio_port_get_raw_exit
#define sys_port_trace_gpio_port_get_raw_exit(port, ret)
#endif

/**
 * @brief Trace GPIO port set masked raw enter call
 * @param port Pointer to device structure for the driver instance
 * @param mask Mask indicating which pins will be modified
 * @param value Value to be written to the output pins
 */
#ifndef sys_port_trace_gpio_port_set_masked_raw_enter
#define sys_port_trace_gpio_port_set_masked_raw_enter(port, mask, value)
#endif

/**
 * @brief Trace GPIO port set masked raw exit call
 * @param port Pointer to device structure for the driver instance
 * @param ret Return value
 */
#ifndef sys_port_trace_gpio_port_set_masked_raw_exit
#define sys_port_trace_gpio_port_set_masked_raw_exit(port, ret)
#endif

/**
 * @brief Trace GPIO port set bits raw enter call
 * @param port Pointer to device structure for the driver instance
 * @param pins Value indicating which pins will be modified
 */
#ifndef sys_port_trace_gpio_port_set_bits_raw_enter
#define sys_port_trace_gpio_port_set_bits_raw_enter(port, pins)
#endif

/**
 * @brief Trace GPIO port set bits raw exit call
 * @param port Pointer to device structure for the driver instance
 * @param ret Return value
 */
#ifndef sys_port_trace_gpio_port_set_bits_raw_exit
#define sys_port_trace_gpio_port_set_bits_raw_exit(port, ret)
#endif

/**
 * @brief Trace GPIO port clear bits raw enter call
 * @param port Pointer to device structure for the driver instance
 * @param pins Value indicating which pins will be modified
 */
#ifndef sys_port_trace_gpio_port_clear_bits_raw_enter
#define sys_port_trace_gpio_port_clear_bits_raw_enter(port, pins)
#endif

/**
 * @brief Trace GPIO port clear bits raw exit call
 * @param port Pointer to device structure for the driver instance
 * @param ret Return value
 */
#ifndef sys_port_trace_gpio_port_clear_bits_raw_exit
#define sys_port_trace_gpio_port_clear_bits_raw_exit(port, ret)
#endif

/**
 * @brief Trace GPIO port toggle bits enter call
 * @param port Pointer to device structure for the driver instance
 * @param pins Value indicating which pins will be modified
 */
#ifndef sys_port_trace_gpio_port_toggle_bits_enter
#define sys_port_trace_gpio_port_toggle_bits_enter(port, pins)
#endif

/**
 * @brief Trace GPIO port toggle bits exit call
 * @param port Pointer to device structure for the driver instance
 * @param ret Return value
 */
#ifndef sys_port_trace_gpio_port_toggle_bits_exit
#define sys_port_trace_gpio_port_toggle_bits_exit(port, ret)
#endif

/**
 * @brief Trace GPIO init callback enter call
 * @param callback A valid application's callback structure pointer
 * @param handler A valid handler function pointer
 * @param pin_mask A bit mask of relevant pins for the handler
 */
#ifndef sys_port_trace_gpio_init_callback_enter
#define sys_port_trace_gpio_init_callback_enter(callback, handler, pin_mask)
#endif

/**
 * @brief Trace GPIO init callback exit call
 * @param callback A valid application's callback structure pointer
 */
#ifndef sys_port_trace_gpio_init_callback_exit
#define sys_port_trace_gpio_init_callback_exit(callback)
#endif

/**
 * @brief Trace GPIO add callback enter call
 * @param port Pointer to device structure for the driver instance
 * @param callback A valid application's callback structure pointer
 */
#ifndef sys_port_trace_gpio_add_callback_enter
#define sys_port_trace_gpio_add_callback_enter(port, callback)
#endif

/**
 * @brief Trace GPIO add callback exit call
 * @param port Pointer to device structure for the driver instance
 * @param ret Return value
 */
#ifndef sys_port_trace_gpio_add_callback_exit
#define sys_port_trace_gpio_add_callback_exit(port, ret)
#endif

/**
 * @brief Trace GPIO remove callback enter call
 * @param port Pointer to device structure for the driver instance
 * @param callback A valid application's callback structure pointer
 */
#ifndef sys_port_trace_gpio_remove_callback_enter
#define sys_port_trace_gpio_remove_callback_enter(port, callback)
#endif

/**
 * @brief Trace GPIO remove callback exit call
 * @param port Pointer to device structure for the driver instance
 * @param ret Return value
 */
#ifndef sys_port_trace_gpio_remove_callback_exit
#define sys_port_trace_gpio_remove_callback_exit(port, ret)
#endif

/**
 * @brief Trace GPIO get pending interrupt enter call
 * @param dev Pointer to the device structure for the device instance
 */
#ifndef sys_port_trace_gpio_get_pending_int_enter
#define sys_port_trace_gpio_get_pending_int_enter(dev)
#endif

/**
 * @brief Trace GPIO get pending interrupt exit call
 * @param dev Pointer to the device structure for the device instance
 * @param ret Return value
 */
#ifndef sys_port_trace_gpio_get_pending_int_exit
#define sys_port_trace_gpio_get_pending_int_exit(dev, ret)
#endif

/**
 * @brief
 * @param list @ref sys_slist_t representing gpio_callback pointers
 * @param port @ref device representing the GPIO port
 * @param pins @ref gpio_pin_t representing the pins
 */
#ifndef sys_port_trace_gpio_fire_callbacks_enter
#define sys_port_trace_gpio_fire_callbacks_enter(list, port, pins)
#endif

/**
 * @brief
 * @param port @ref device representing the GPIO port
 * @param callback @ref gpio_callback a valid Application's callback structure pointer
 */
#ifndef sys_port_trace_gpio_fire_callback
#define sys_port_trace_gpio_fire_callback(port, callback)
#endif

/** @} */ /* end of subsys_tracing_apis_gpio */

/**
 * @brief RTIO Tracing APIs
 * @defgroup subsys_tracing_apis_rtio RTIO Tracing APIs
 * @{
 */

/**
 * @brief Trace RTIO Submit Enter API
 * @param rtio RTIO context
 * @param wait_count Number of submissions to wait for completion of.
 */
#ifndef sys_port_trace_rtio_submit_enter
#define sys_port_trace_rtio_submit_enter(rtio, wait_count)
#endif

/**
 * @brief Trace RTIO Submit Exit API
 * @param rtio RTIO context
 */
#ifndef sys_port_trace_rtio_submit_exit
#define sys_port_trace_rtio_submit_exit(rtio)
#endif

/**
 * @brief Trace RTIO Submission Queue Event Acquire Enter API
 * @param rtio RTIO context
 */
#ifndef sys_port_trace_rtio_sqe_acquire_enter
#define sys_port_trace_rtio_sqe_acquire_enter(rtio)
#endif

/**
 * @brief Trace RTIO Submission Queue Event Acquire Exit API
 * @param rtio RTIO context
 * @param sqe Submission Queue Event
 */
#ifndef sys_port_trace_rtio_sqe_acquire_exit
#define sys_port_trace_rtio_sqe_acquire_exit(rtio, sqe)
#endif

/**
 * @brief Trace RTIO Submission Queue Event Cancel API
 * @param sqe Submission Queue Event
 */
#ifndef sys_port_trace_rtio_sqe_cancel
#define sys_port_trace_rtio_sqe_cancel(sqe)
#endif

/**
 * @brief Trace RTIO Completion Queue Event Submit Enter API
 * @param rtio RTIO context
 * @param result Integer result code (could be -errno)
 * @param flags Flags to use for the CQE see RTIO_CQE_FLAG_*
 */
#ifndef sys_port_trace_rtio_cqe_submit_enter
#define sys_port_trace_rtio_cqe_submit_enter(rtio, result, flags)
#endif

/**
 * @brief Trace RTIO Completion Queue Event Submit Exit API
 * @param rtio RTIO context
 */
#ifndef sys_port_trace_rtio_cqe_submit_exit
#define sys_port_trace_rtio_cqe_submit_exit(rtio)
#endif

/**
 * @brief Trace RTIO Completion Queue Event Acquire Enter API
 * @param rtio RTIO context
 */
#ifndef sys_port_trace_rtio_cqe_acquire_enter
#define sys_port_trace_rtio_cqe_acquire_enter(rtio)
#endif

/**
 * @brief Trace RTIO Completion Queue Event Acquire Exit API
 * @param rtio RTIO context
 * @param cqe Complete Queue Event
 */
#ifndef sys_port_trace_rtio_cqe_acquire_exit
#define sys_port_trace_rtio_cqe_acquire_exit(rtio, cqe)
#endif

/**
 * @brief Trace RTIO Completion Queue Event Release API
 * @param rtio RTIO context
 * @param cqe Complete Queue Event
 */
#ifndef sys_port_trace_rtio_cqe_release
#define sys_port_trace_rtio_cqe_release(rtio, cqe)
#endif

/**
 * @brief Trace RTIO Completion Queue Event Consume Enter API
 * @param rtio RTIO context
 */
#ifndef sys_port_trace_rtio_cqe_consume_enter
#define sys_port_trace_rtio_cqe_consume_enter(rtio)
#endif

/**
 * @brief Trace RTIO Completion Queue Event Consume Exit API
 * @param rtio RTIO context
 * @param cqe Complete Queue Event
 */
#ifndef sys_port_trace_rtio_cqe_consume_exit
#define sys_port_trace_rtio_cqe_consume_exit(rtio, cqe)
#endif

/**
 * @brief Trace RTIO get next transaction Enter API
 * @param rtio RTIO context
 * @param iodev_sqe Current submission queue entry
 */
#ifndef sys_port_trace_rtio_txn_next_enter
#define sys_port_trace_rtio_txn_next_enter(rtio, iodev_sqe)
#endif

/**
 * @brief Trace RTIO get next transaction Exit API
 * @param rtio RTIO context
 * @param iodev_sqe Next submission queue entry
 */
#ifndef sys_port_trace_rtio_txn_next_exit
#define sys_port_trace_rtio_txn_next_exit(rtio, iodev_sqe)
#endif

/**
 * @brief Trace RTIO get next sqe in chain Enter API
 * @param rtio RTIO context
 * @param iodev_sqe Current submission queue entry
 */
#ifndef sys_port_trace_rtio_chain_next_enter
#define sys_port_trace_rtio_chain_next_enter(rtio, iodev_sqe)
#endif

/**
 * @brief Trace RTIO get next sqe in chain Exit API
 * @param rtio RTIO context
 * @param iodev_sqe Next submission queue entry
 */
#ifndef sys_port_trace_rtio_chain_next_exit
#define sys_port_trace_rtio_chain_next_exit(rtio, iodev_sqe)
#endif

/** @} */ /* end of subsys_tracing_apis_rtio */

/**
 * @defgroup subsys_tracing_apis_backend_ext Backend-specific extensions
 * @{
 *
 * Hooks defined only by specific backends (e.g. SystemView pipe internals).
 * Provided here as no-op fallbacks so every backend resolves the full set.
 */
/**
 * @brief Trace initialization of a dynamically allocated pipe Enter API
 * @param pipe Pipe object
 */
#ifndef sys_port_trace_k_pipe_alloc_init_enter
#define sys_port_trace_k_pipe_alloc_init_enter(pipe)
#endif
/**
 * @brief Trace initialization of a dynamically allocated pipe Exit API
 * @param pipe Pipe object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_pipe_alloc_init_exit
#define sys_port_trace_k_pipe_alloc_init_exit(pipe, ret)
#endif
/**
 * @brief Trace flushing a pipe buffer Enter API
 * @param pipe Pipe object
 */
#ifndef sys_port_trace_k_pipe_buffer_flush_enter
#define sys_port_trace_k_pipe_buffer_flush_enter(pipe)
#endif
/**
 * @brief Trace flushing a pipe buffer Exit API
 * @param pipe Pipe object
 */
#ifndef sys_port_trace_k_pipe_buffer_flush_exit
#define sys_port_trace_k_pipe_buffer_flush_exit(pipe)
#endif
/**
 * @brief Trace pipe cleanup Enter API
 * @param pipe Pipe object
 */
#ifndef sys_port_trace_k_pipe_cleanup_enter
#define sys_port_trace_k_pipe_cleanup_enter(pipe)
#endif
/**
 * @brief Trace pipe cleanup Exit API
 * @param pipe Pipe object
 * @param ret Return value
 */
#ifndef sys_port_trace_k_pipe_cleanup_exit
#define sys_port_trace_k_pipe_cleanup_exit(pipe, ret)
#endif
/**
 * @brief Trace flushing a pipe Enter API
 * @param pipe Pipe object
 */
#ifndef sys_port_trace_k_pipe_flush_enter
#define sys_port_trace_k_pipe_flush_enter(pipe)
#endif
/**
 * @brief Trace flushing a pipe Exit API
 * @param pipe Pipe object
 */
#ifndef sys_port_trace_k_pipe_flush_exit
#define sys_port_trace_k_pipe_flush_exit(pipe)
#endif
/**
 * @brief Trace getting data from a pipe Blocking API
 * @param pipe Pipe object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_pipe_get_blocking
#define sys_port_trace_k_pipe_get_blocking(pipe, timeout)
#endif
/**
 * @brief Trace getting data from a pipe Enter API
 * @param pipe Pipe object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_pipe_get_enter
#define sys_port_trace_k_pipe_get_enter(pipe, timeout)
#endif
/**
 * @brief Trace getting data from a pipe Exit API
 * @param pipe Pipe object
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_pipe_get_exit
#define sys_port_trace_k_pipe_get_exit(pipe, timeout, ret)
#endif
/**
 * @brief Trace putting data into a pipe Blocking API
 * @param pipe Pipe object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_pipe_put_blocking
#define sys_port_trace_k_pipe_put_blocking(pipe, timeout)
#endif
/**
 * @brief Trace putting data into a pipe Enter API
 * @param pipe Pipe object
 * @param timeout Timeout period
 */
#ifndef sys_port_trace_k_pipe_put_enter
#define sys_port_trace_k_pipe_put_enter(pipe, timeout)
#endif
/**
 * @brief Trace putting data into a pipe Exit API
 * @param pipe Pipe object
 * @param timeout Timeout period
 * @param ret Return value
 */
#ifndef sys_port_trace_k_pipe_put_exit
#define sys_port_trace_k_pipe_put_exit(pipe, timeout, ret)
#endif
/**
 * @brief Trace assigning a heap to a thread
 * @param thread Thread object
 * @param heap Heap object
 */
#ifndef sys_port_trace_k_thread_heap_assign
#define sys_port_trace_k_thread_heap_assign(thread, heap)
#endif
/**
 * @brief Trace a system call Exit API
 * @param id System call id
 * @param name System call name
 */
#ifndef sys_port_trace_syscall_exit
#define sys_port_trace_syscall_exit(id, name, ...)
#endif

/** @} */ /* end of subsys_tracing_apis_backend_ext */

#endif /* ZEPHYR_INCLUDE_TRACING_TRACING_HOOKS_H_ */
