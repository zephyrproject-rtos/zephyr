/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_TRACING_TRACING_H_
#define ZEPHYR_INCLUDE_TRACING_TRACING_H_

#include <zephyr/kernel.h>

#include "tracking.h"

#if defined CONFIG_SEGGER_SYSTEMVIEW
#include "tracing_sysview.h"
#elif defined CONFIG_TRACING_CTF
#include "tracing_ctf.h"
#elif defined CONFIG_TRACING_TEST
#include "tracing_test.h"
#elif defined CONFIG_TRACING_USER
#include "tracing_user.h"
#else
/**
 * @brief Tracing
 *
 * The tracing subsystem provides hooks that permits you to collect data from
 * your application and allows tools running on a host to visualize the
 * inner-working of the kernel and various other subsystems.
 *
 * @defgroup subsys_tracing Tracing
 * @ingroup os_services
 * @{
 */

/**
 * @brief Tracing APIs
 * @defgroup subsys_tracing_apis Tracing APIs
 * @{
 */

/**
 * @brief Thread Tracing APIs
 * @defgroup subsys_tracing_apis_thread Thread Tracing APIs
 * @{
 */

/**
 * @brief Called when entering a k_thread_foreach call
 */
#define sys_port_trace_k_thread_foreach_enter()

/**
 * @brief Called when exiting a k_thread_foreach call
 */
#define sys_port_trace_k_thread_foreach_exit()

/**
 * @brief Called when entering a k_thread_foreach_unlocked
 */
#define sys_port_trace_k_thread_foreach_unlocked_enter()

/**
 * @brief Called when exiting a k_thread_foreach_unlocked
 */
#define sys_port_trace_k_thread_foreach_unlocked_exit()

/**
 * @brief Trace creating a Thread
 * @param new_thread Thread object
 */
#define sys_port_trace_k_thread_create(new_thread)

/**
 * @brief Trace Thread entering user mode
 */
#define sys_port_trace_k_thread_user_mode_enter()

/**
 * @brief Called when entering a k_thread_join
 * @param thread Thread object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_thread_join_enter(thread, timeout)

/**
 * @brief Called when k_thread_join blocks
 * @param thread Thread object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_thread_join_blocking(thread, timeout)

/**
 * @brief Called when exiting k_thread_join
 * @param thread Thread object
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_thread_join_exit(thread, timeout, ret)

/**
 * @brief Called when entering k_thread_sleep
 * @param timeout Timeout period
 */
#define sys_port_trace_k_thread_sleep_enter(timeout)

/**
 * @brief Called when exiting k_thread_sleep
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_thread_sleep_exit(timeout, ret)

/**
 * @brief Called when entering k_thread_msleep
 * @param ms Duration in milliseconds
 */
#define sys_port_trace_k_thread_msleep_enter(ms)

/**
 * @brief Called when exiting k_thread_msleep
 * @param ms Duration in milliseconds
 * @param ret Return value
 */
#define sys_port_trace_k_thread_msleep_exit(ms, ret)

/**
 * @brief Called when entering k_thread_usleep
 * @param us Duration in microseconds
 */
#define sys_port_trace_k_thread_usleep_enter(us)

/**
 * @brief Called when exiting k_thread_usleep
 * @param us Duration in microseconds
 * @param ret Return value
 */
#define sys_port_trace_k_thread_usleep_exit(us, ret)

/**
 * @brief Called when entering k_thread_busy_wait
 * @param usec_to_wait Duration in microseconds
 */
#define sys_port_trace_k_thread_busy_wait_enter(usec_to_wait)

/**
 * @brief Called when exiting k_thread_busy_wait
 * @param usec_to_wait Duration in microseconds
 */
#define sys_port_trace_k_thread_busy_wait_exit(usec_to_wait)

/**
 * @brief Called when a thread yields
 */
#define sys_port_trace_k_thread_yield()

/**
 * @brief Called when a thread wakes up
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_wakeup(thread)

/**
 * @brief Called when a thread is started
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_start(thread)

/**
 * @brief Called when a thread is being aborted
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_abort(thread)

/**
 * @brief Called when a thread enters the k_thread_abort routine
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_abort_enter(thread)

/**
 * @brief Called when a thread exits the k_thread_abort routine
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_abort_exit(thread)

/**
 * @brief Called when setting priority of a thread
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_priority_set(thread)

/**
 * @brief Called when a thread enters the k_thread_suspend
 * function.
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_suspend_enter(thread)

/**
 * @brief Called when a thread exits the k_thread_suspend
 * function.
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_suspend_exit(thread)

/**
 * @brief Called when a thread enters the resume from suspension
 * function.
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_resume_enter(thread)

/**
 * @brief Called when a thread exits the resumed from suspension
 * function.
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_resume_exit(thread)

/**
 * @brief Called when the thread scheduler is locked
 */
#define sys_port_trace_k_thread_sched_lock()

/**
 * @brief Called when the thread scheduler is unlocked
 */
#define sys_port_trace_k_thread_sched_unlock()

/**
 * @brief Called when a thread name is set
 * @param thread Thread object
 * @param ret Return value
 */
#define sys_port_trace_k_thread_name_set(thread, ret)

/**
 * @brief Called before a thread has been selected to run
 */
#define sys_port_trace_k_thread_switched_out()

/**
 * @brief Called after a thread has been selected to run
 */
#define sys_port_trace_k_thread_switched_in()

/**
 * @brief Called when a thread is ready to run
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_ready(thread)

/**
 * @brief Called when a thread is pending
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_pend(thread)

/**
 * @brief Provide information about specific thread
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_info(thread)

/**
 * @brief Trace implicit thread wakeup invocation by the scheduler
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_sched_wakeup(thread)

/**
 * @brief Trace implicit thread abort invocation by the scheduler
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_sched_abort(thread)

/**
 * @brief Trace implicit thread set priority invocation by the scheduler
 * @param thread Thread object
 * @param prio Thread priority
 */
#define sys_port_trace_k_thread_sched_priority_set(thread, prio)

/**
 * @brief Trace implicit thread ready invocation by the scheduler
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_sched_ready(thread)

/**
 * @brief Trace implicit thread pend invocation by the scheduler
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_sched_pend(thread)

/**
 * @brief Trace implicit thread resume invocation by the scheduler
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_sched_resume(thread)

/**
 * @brief Trace implicit thread suspend invocation by the scheduler
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_sched_suspend(thread)

/** @}c*/ /* end of subsys_tracing_apis_thread */

/**
 * @brief Work Tracing APIs
 * @defgroup subsys_tracing_apis_work Work Tracing APIs
 * @{
 */

/**
 * @brief Trace initialisation of a Work structure
 * @param work Work structure
 */
#define sys_port_trace_k_work_init(work)

/**
 * @brief Trace submit work to work queue call entry
 * @param queue Work queue structure
 * @param work Work structure
 */
#define sys_port_trace_k_work_submit_to_queue_enter(queue, work)

/**
 * @brief Trace submit work to work queue call exit
 * @param queue Work queue structure
 * @param work Work structure
 * @param ret Return value
 */
#define sys_port_trace_k_work_submit_to_queue_exit(queue, work, ret)

/**
 * @brief Trace submit work to system work queue call entry
 * @param work Work structure
 */
#define sys_port_trace_k_work_submit_enter(work)

/**
 * @brief Trace submit work to system work queue call exit
 * @param work Work structure
 * @param ret Return value
 */
#define sys_port_trace_k_work_submit_exit(work, ret)

/**
 * @brief Trace flush work call entry
 * @param work Work structure
 */
#define sys_port_trace_k_work_flush_enter(work)

/**
 * @brief Trace flush work call blocking
 * @param work Work structure
 * @param timeout Timeout period
 */
#define sys_port_trace_k_work_flush_blocking(work, timeout)

/**
 * @brief Trace flush work call exit
 * @param work Work structure
 * @param ret Return value
 */
#define sys_port_trace_k_work_flush_exit(work, ret)

/**
 * @brief Trace cancel work call entry
 * @param work Work structure
 */
#define sys_port_trace_k_work_cancel_enter(work)

/**
 * @brief Trace cancel work call exit
 * @param work Work structure
 * @param ret Return value
 */
#define sys_port_trace_k_work_cancel_exit(work, ret)

/**
 * @brief Trace cancel sync work call entry
 * @param work Work structure
 * @param sync Sync object
 */
#define sys_port_trace_k_work_cancel_sync_enter(work, sync)

/**
 * @brief Trace cancel sync work call blocking
 * @param work Work structure
 * @param sync Sync object
 */
#define sys_port_trace_k_work_cancel_sync_blocking(work, sync)

/**
 * @brief Trace cancel sync work call exit
 * @param work Work structure
 * @param sync Sync object
 * @param ret Return value
 */
#define sys_port_trace_k_work_cancel_sync_exit(work, sync, ret)

/** @} */ /* end of subsys_tracing_apis_work */

/**
 * @brief Work Queue Tracing APIs
 * @defgroup subsys_tracing_apis_work_q Work Queue Tracing APIs
 * @{
 */

/**
 * @brief Trace initialisation of a Work Queue structure
 * @param queue Work Queue structure
 */
#define sys_port_trace_k_work_queue_init(queue)

/**
 * @brief Trace start of a Work Queue call entry
 * @param queue Work Queue structure
 */
#define sys_port_trace_k_work_queue_start_enter(queue)

/**
 * @brief Trace start of a Work Queue call exit
 * @param queue Work Queue structure
 */
#define sys_port_trace_k_work_queue_start_exit(queue)

/**
 * @brief Trace Work Queue drain call entry
 * @param queue Work Queue structure
 */
#define sys_port_trace_k_work_queue_drain_enter(queue)

/**
 * @brief Trace Work Queue drain call exit
 * @param queue Work Queue structure
 * @param ret Return value
 */
#define sys_port_trace_k_work_queue_drain_exit(queue, ret)

/**
 * @brief Trace Work Queue unplug call entry
 * @param queue Work Queue structure
 */
#define sys_port_trace_k_work_queue_unplug_enter(queue)

/**
 * @brief Trace Work Queue unplug call exit
 * @param queue Work Queue structure
 * @param ret Return value
 */
#define sys_port_trace_k_work_queue_unplug_exit(queue, ret)

/** @} */ /* end of subsys_tracing_apis_work_q */

/**
 * @brief Work Delayable Tracing APIs
 * @defgroup subsys_tracing_apis_work_delayable Work Delayable Tracing APIs
 * @{
 */

/**
 * @brief Trace initialisation of a Delayable Work structure
 * @param dwork Delayable Work structure
 */
#define sys_port_trace_k_work_delayable_init(dwork)

/**
 * @brief Trace schedule delayable work for queue enter
 * @param queue Work Queue structure
 * @param dwork Delayable Work structure
 * @param delay Delay period
 */
#define sys_port_trace_k_work_schedule_for_queue_enter(queue, dwork, delay)

/**
 * @brief Trace schedule delayable work for queue exit
 * @param queue Work Queue structure
 * @param dwork Delayable Work structure
 * @param delay Delay period
 * @param ret Return value
 */
#define sys_port_trace_k_work_schedule_for_queue_exit(queue, dwork, delay, ret)

/**
 * @brief Trace schedule delayable work for system work queue enter
 * @param dwork Delayable Work structure
 * @param delay Delay period
 */
#define sys_port_trace_k_work_schedule_enter(dwork, delay)

/**
 * @brief Trace schedule delayable work for system work queue exit
 * @param dwork Delayable Work structure
 * @param delay Delay period
 * @param ret Return value
 */
#define sys_port_trace_k_work_schedule_exit(dwork, delay, ret)

/**
 * @brief Trace reschedule delayable work for queue enter
 * @param queue Work Queue structure
 * @param dwork Delayable Work structure
 * @param delay Delay period
 */
#define sys_port_trace_k_work_reschedule_for_queue_enter(queue, dwork, delay)

/**
 * @brief Trace reschedule delayable work for queue exit
 * @param queue Work Queue structure
 * @param dwork Delayable Work structure
 * @param delay Delay period
 * @param ret Return value
 */
#define sys_port_trace_k_work_reschedule_for_queue_exit(queue, dwork, delay, ret)

/**
 * @brief Trace reschedule delayable work for system queue enter
 * @param dwork Delayable Work structure
 * @param delay Delay period
 */
#define sys_port_trace_k_work_reschedule_enter(dwork, delay)

/**
 * @brief Trace reschedule delayable work for system queue exit
 * @param dwork Delayable Work structure
 * @param delay Delay period
 * @param ret Return value
 */
#define sys_port_trace_k_work_reschedule_exit(dwork, delay, ret)

/**
 * @brief Trace delayable work flush enter
 * @param dwork Delayable Work structure
 * @param sync Sync object
 */
#define sys_port_trace_k_work_flush_delayable_enter(dwork, sync)

/**
 * @brief Trace delayable work flush exit
 * @param dwork Delayable Work structure
 * @param sync Sync object
 * @param ret Return value
 */
#define sys_port_trace_k_work_flush_delayable_exit(dwork, sync, ret)

/**
 * @brief Trace delayable work cancel enter
 * @param dwork Delayable Work structure
 */
#define sys_port_trace_k_work_cancel_delayable_enter(dwork)

/**
 * @brief Trace delayable work cancel enter
 * @param dwork Delayable Work structure
 * @param ret Return value
 */
#define sys_port_trace_k_work_cancel_delayable_exit(dwork, ret)

/**
 * @brief Trace delayable work cancel sync enter
 * @param dwork Delayable Work structure
 * @param sync Sync object
 */
#define sys_port_trace_k_work_cancel_delayable_sync_enter(dwork, sync)

/**
 * @brief Trace delayable work cancel sync enter
 * @param dwork Delayable Work structure
 * @param sync Sync object
 * @param ret Return value
 */
#define sys_port_trace_k_work_cancel_delayable_sync_exit(dwork, sync, ret)

/** @} */ /* end of subsys_tracing_apis_work_delayable */

/**
 * @brief Work Poll Tracing APIs
 * @defgroup subsys_tracing_apis_work_poll Work Poll Tracing APIs
 * @{
 */

/**
 * @brief Trace initialisation of a Work Poll structure enter
 * @param work Work structure
 */
#define sys_port_trace_k_work_poll_init_enter(work)

/**
 * @brief Trace initialisation of a Work Poll structure exit
 * @param work Work structure
 */
#define sys_port_trace_k_work_poll_init_exit(work)

/**
 * @brief Trace work poll submit to queue enter
 * @param work_q Work queue
 * @param work Work structure
 * @param timeout Timeout period
 */
#define sys_port_trace_k_work_poll_submit_to_queue_enter(work_q, work, timeout)

/**
 * @brief Trace work poll submit to queue blocking
 * @param work_q Work queue
 * @param work Work structure
 * @param timeout Timeout period
 */
#define sys_port_trace_k_work_poll_submit_to_queue_blocking(work_q, work, timeout)

/**
 * @brief Trace work poll submit to queue exit
 * @param work_q Work queue
 * @param work Work structure
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_work_poll_submit_to_queue_exit(work_q, work, timeout, ret)

/**
 * @brief Trace work poll submit to system queue enter
 * @param work Work structure
 * @param timeout Timeout period
 */
#define sys_port_trace_k_work_poll_submit_enter(work, timeout)

/**
 * @brief Trace work poll submit to system queue exit
 * @param work Work structure
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_work_poll_submit_exit(work, timeout, ret)

/**
 * @brief Trace work poll cancel enter
 * @param work Work structure
 */
#define sys_port_trace_k_work_poll_cancel_enter(work)

/**
 * @brief Trace work poll cancel exit
 * @param work Work structure
 * @param ret Return value
 */
#define sys_port_trace_k_work_poll_cancel_exit(work, ret)

/** @} */ /* end of subsys_tracing_apis_work_poll */

/**
 * @brief Poll Tracing APIs
 * @defgroup subsys_tracing_apis_poll Poll Tracing APIs
 * @{
 */

/**
 * @brief Trace initialisation of a Poll Event
 * @param event Poll Event
 */
#define sys_port_trace_k_poll_api_event_init(event)

/**
 * @brief Trace Polling call start
 * @param events Poll Events
 */
#define sys_port_trace_k_poll_api_poll_enter(events)

/**
 * @brief Trace Polling call outcome
 * @param events Poll Events
 * @param ret Return value
 */
#define sys_port_trace_k_poll_api_poll_exit(events, ret)

/**
 * @brief Trace initialisation of a Poll Signal
 * @param signal Poll Signal
 */
#define sys_port_trace_k_poll_api_signal_init(signal)

/**
 * @brief Trace resetting of Poll Signal
 * @param signal Poll Signal
 */
#define sys_port_trace_k_poll_api_signal_reset(signal)

/**
 * @brief Trace checking of Poll Signal
 * @param signal Poll Signal
 */
#define sys_port_trace_k_poll_api_signal_check(signal)

/**
 * @brief Trace raising of Poll Signal
 * @param signal Poll Signal
 * @param ret Return value
 */
#define sys_port_trace_k_poll_api_signal_raise(signal, ret)

/** @} */ /* end of subsys_tracing_apis_poll */

/**
 * @brief Semaphore Tracing APIs
 * @defgroup subsys_tracing_apis_sem Semaphore Tracing APIs
 * @{
 */

/**
 * @brief Trace initialisation of a Semaphore
 * @param sem Semaphore object
 * @param ret Return value
 */
#define sys_port_trace_k_sem_init(sem, ret)

/**
 * @brief Trace giving a Semaphore entry
 * @param sem Semaphore object
 */
#define sys_port_trace_k_sem_give_enter(sem)

/**
 * @brief Trace giving a Semaphore exit
 * @param sem Semaphore object
 */
#define sys_port_trace_k_sem_give_exit(sem)

/**
 * @brief Trace taking a Semaphore attempt start
 * @param sem Semaphore object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_sem_take_enter(sem, timeout)

/**
 * @brief Trace taking a Semaphore attempt blocking
 * @param sem Semaphore object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_sem_take_blocking(sem, timeout)

/**
 * @brief Trace taking a Semaphore attempt outcome
 * @param sem Semaphore object
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_sem_take_exit(sem, timeout, ret)

/**
 * @brief Trace resetting a Semaphore
 * @param sem Semaphore object
 */
#define sys_port_trace_k_sem_reset(sem)

/** @} */ /* end of subsys_tracing_apis_sem */

/**
 * @brief Mutex Tracing APIs
 * @defgroup subsys_tracing_apis_mutex Mutex Tracing APIs
 * @{
 */

/**
 * @brief Trace initialization of Mutex
 * @param mutex Mutex object
 * @param ret Return value
 */
#define sys_port_trace_k_mutex_init(mutex, ret)

/**
 * @brief Trace Mutex lock attempt start
 * @param mutex Mutex object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_mutex_lock_enter(mutex, timeout)

/**
 * @brief Trace Mutex lock attempt blocking
 * @param mutex Mutex object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_mutex_lock_blocking(mutex, timeout)

/**
 * @brief Trace Mutex lock attempt outcome
 * @param mutex Mutex object
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_mutex_lock_exit(mutex, timeout, ret)

/**
 * @brief Trace Mutex unlock entry
 * @param mutex Mutex object
 */
#define sys_port_trace_k_mutex_unlock_enter(mutex)

/**
 * @brief Trace Mutex unlock exit
 */
#define sys_port_trace_k_mutex_unlock_exit(mutex, ret)

/** @} */ /* end of subsys_tracing_apis_mutex */

/**
 * @brief Conditional Variable Tracing APIs
 * @defgroup subsys_tracing_apis_condvar Conditional Variable Tracing APIs
 * @{
 */

/**
 * @brief Trace initialization of Conditional Variable
 * @param condvar Conditional Variable object
 * @param ret Return value
 */
#define sys_port_trace_k_condvar_init(condvar, ret)

/**
 * @brief Trace Conditional Variable signaling start
 * @param condvar Conditional Variable object
 */
#define sys_port_trace_k_condvar_signal_enter(condvar)

/**
 * @brief Trace Conditional Variable signaling blocking
 * @param condvar Conditional Variable object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_condvar_signal_blocking(condvar, timeout)

/**
 * @brief Trace Conditional Variable signaling outcome
 * @param condvar Conditional Variable object
 * @param ret Return value
 */
#define sys_port_trace_k_condvar_signal_exit(condvar, ret)

/**
 * @brief Trace Conditional Variable broadcast enter
 * @param condvar Conditional Variable object
 */
#define sys_port_trace_k_condvar_broadcast_enter(condvar)

/**
 * @brief Trace Conditional Variable broadcast exit
 * @param condvar Conditional Variable object
 * @param ret Return value
 */
#define sys_port_trace_k_condvar_broadcast_exit(condvar, ret)

/**
 * @brief Trace Conditional Variable wait enter
 * @param condvar Conditional Variable object
 */
#define sys_port_trace_k_condvar_wait_enter(condvar)

/**
 * @brief Trace Conditional Variable wait exit
 * @param condvar Conditional Variable object
 * @param ret Return value
 */
#define sys_port_trace_k_condvar_wait_exit(condvar, ret)

/** @} */ /* end of subsys_tracing_apis_condvar */

/**
 * @brief Queue Tracing APIs
 * @defgroup subsys_tracing_apis_queue Queue Tracing APIs
 * @{
 */

/**
 * @brief Trace initialization of Queue
 * @param queue Queue object
 */
#define sys_port_trace_k_queue_init(queue)

/**
 * @brief Trace Queue cancel wait
 * @param queue Queue object
 */
#define sys_port_trace_k_queue_cancel_wait(queue)

/**
 * @brief Trace Queue insert attempt entry
 * @param queue Queue object
 * @param alloc Allocation flag
 */
#define sys_port_trace_k_queue_queue_insert_enter(queue, alloc)

/**
 * @brief Trace Queue insert attempt blocking
 * @param queue Queue object
 * @param alloc Allocation flag
 * @param timeout Timeout period
 */
#define sys_port_trace_k_queue_queue_insert_blocking(queue, alloc, timeout)

/**
 * @brief Trace Queue insert attempt outcome
 * @param queue Queue object
 * @param alloc Allocation flag
 * @param ret Return value
 */
#define sys_port_trace_k_queue_queue_insert_exit(queue, alloc, ret)

/**
 * @brief Trace Queue append enter
 * @param queue Queue object
 */
#define sys_port_trace_k_queue_append_enter(queue)

/**
 * @brief Trace Queue append exit
 * @param queue Queue object
 */
#define sys_port_trace_k_queue_append_exit(queue)

/**
 * @brief Trace Queue alloc append enter
 * @param queue Queue object
 */
#define sys_port_trace_k_queue_alloc_append_enter(queue)

/**
 * @brief Trace Queue alloc append exit
 * @param queue Queue object
 * @param ret Return value
 */
#define sys_port_trace_k_queue_alloc_append_exit(queue, ret)

/**
 * @brief Trace Queue prepend enter
 * @param queue Queue object
 */
#define sys_port_trace_k_queue_prepend_enter(queue)

/**
 * @brief Trace Queue prepend exit
 * @param queue Queue object
 */
#define sys_port_trace_k_queue_prepend_exit(queue)

/**
 * @brief Trace Queue alloc prepend enter
 * @param queue Queue object
 */
#define sys_port_trace_k_queue_alloc_prepend_enter(queue)

/**
 * @brief Trace Queue alloc prepend exit
 * @param queue Queue object
 * @param ret Return value
 */
#define sys_port_trace_k_queue_alloc_prepend_exit(queue, ret)

/**
 * @brief Trace Queue insert attempt entry
 * @param queue Queue object
 */
#define sys_port_trace_k_queue_insert_enter(queue)

/**
 * @brief Trace Queue insert attempt blocking
 * @param queue Queue object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_queue_insert_blocking(queue, timeout)

/**
 * @brief Trace Queue insert attempt exit
 * @param queue Queue object
 */
#define sys_port_trace_k_queue_insert_exit(queue)

/**
 * @brief Trace Queue append list enter
 * @param queue Queue object
 */
#define sys_port_trace_k_queue_append_list_enter(queue)

/**
 * @brief Trace Queue append list exit
 * @param queue Queue object
 * @param ret Return value
 */
#define sys_port_trace_k_queue_append_list_exit(queue, ret)

/**
 * @brief Trace Queue merge slist enter
 * @param queue Queue object
 */
#define sys_port_trace_k_queue_merge_slist_enter(queue)

/**
 * @brief Trace Queue merge slist exit
 * @param queue Queue object
 * @param ret Return value
 */
#define sys_port_trace_k_queue_merge_slist_exit(queue, ret)

/**
 * @brief Trace Queue get attempt enter
 * @param queue Queue object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_queue_get_enter(queue, timeout)

/**
 * @brief Trace Queue get attempt blockings
 * @param queue Queue object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_queue_get_blocking(queue, timeout)

/**
 * @brief Trace Queue get attempt outcome
 * @param queue Queue object
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_queue_get_exit(queue, timeout, ret)

/**
 * @brief Trace Queue remove enter
 * @param queue Queue object
 */
#define sys_port_trace_k_queue_remove_enter(queue)

/**
 * @brief Trace Queue remove exit
 * @param queue Queue object
 * @param ret Return value
 */
#define sys_port_trace_k_queue_remove_exit(queue, ret)

/**
 * @brief Trace Queue unique append enter
 * @param queue Queue object
 */
#define sys_port_trace_k_queue_unique_append_enter(queue)

/**
 * @brief Trace Queue unique append exit
 * @param queue Queue object
 *
 * @param ret Return value
 */
#define sys_port_trace_k_queue_unique_append_exit(queue, ret)

/**
 * @brief Trace Queue peek head
 * @param queue Queue object
 * @param ret Return value
 */
#define sys_port_trace_k_queue_peek_head(queue, ret)

/**
 * @brief Trace Queue peek tail
 * @param queue Queue object
 * @param ret Return value
 */
#define sys_port_trace_k_queue_peek_tail(queue, ret)

/** @} */ /* end of subsys_tracing_apis_queue */

/**
 * @brief FIFO Tracing APIs
 * @defgroup subsys_tracing_apis_fifo FIFO Tracing APIs
 * @{
 */

/**
 * @brief Trace initialization of FIFO Queue entry
 * @param fifo FIFO object
 */
#define sys_port_trace_k_fifo_init_enter(fifo)

/**
 * @brief Trace initialization of FIFO Queue exit
 * @param fifo FIFO object
 */
#define sys_port_trace_k_fifo_init_exit(fifo)

/**
 * @brief Trace FIFO Queue cancel wait entry
 * @param fifo FIFO object
 */
#define sys_port_trace_k_fifo_cancel_wait_enter(fifo)

/**
 * @brief Trace FIFO Queue cancel wait exit
 * @param fifo FIFO object
 */
#define sys_port_trace_k_fifo_cancel_wait_exit(fifo)

/**
 * @brief Trace FIFO Queue put entry
 * @param fifo FIFO object
 * @param data Data item
 */
#define sys_port_trace_k_fifo_put_enter(fifo, data)

/**
 * @brief Trace FIFO Queue put exit
 * @param fifo FIFO object
 * @param data Data item
 */
#define sys_port_trace_k_fifo_put_exit(fifo, data)

/**
 * @brief Trace FIFO Queue alloc put entry
 * @param fifo FIFO object
 * @param data Data item
 */
#define sys_port_trace_k_fifo_alloc_put_enter(fifo, data)

/**
 * @brief Trace FIFO Queue alloc put exit
 * @param fifo FIFO object
 * @param data Data item
 * @param ret Return value
 */
#define sys_port_trace_k_fifo_alloc_put_exit(fifo, data, ret)

/**
 * @brief Trace FIFO Queue put list entry
 * @param fifo FIFO object
 * @param head First ll-node
 * @param tail Last ll-node
 */
#define sys_port_trace_k_fifo_put_list_enter(fifo, head, tail)

/**
 * @brief Trace FIFO Queue put list exit
 * @param fifo FIFO object
 * @param head First ll-node
 * @param tail Last ll-node
 */
#define sys_port_trace_k_fifo_put_list_exit(fifo, head, tail)

/**
 * @brief Trace FIFO Queue put slist entry
 * @param fifo FIFO object
 * @param list Syslist object
 */
#define sys_port_trace_k_fifo_alloc_put_slist_enter(fifo, list)

/**
 * @brief Trace FIFO Queue put slist exit
 * @param fifo FIFO object
 * @param list Syslist object
 */
#define sys_port_trace_k_fifo_alloc_put_slist_exit(fifo, list)

/**
 * @brief Trace FIFO Queue get entry
 * @param fifo FIFO object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_fifo_get_enter(fifo, timeout)

/**
 * @brief Trace FIFO Queue get exit
 * @param fifo FIFO object
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_fifo_get_exit(fifo, timeout, ret)

/**
 * @brief Trace FIFO Queue peek head entry
 * @param fifo FIFO object
 */
#define sys_port_trace_k_fifo_peek_head_enter(fifo)

/**
 * @brief Trace FIFO Queue peek head exit
 * @param fifo FIFO object
 * @param ret Return value
 */
#define sys_port_trace_k_fifo_peek_head_exit(fifo, ret)

/**
 * @brief Trace FIFO Queue peek tail entry
 * @param fifo FIFO object
 */
#define sys_port_trace_k_fifo_peek_tail_enter(fifo)

/**
 * @brief Trace FIFO Queue peek tail exit
 * @param fifo FIFO object
 * @param ret Return value
 */
#define sys_port_trace_k_fifo_peek_tail_exit(fifo, ret)

/** @} */ /* end of subsys_tracing_apis_fifo */

/**
 * @brief LIFO Tracing APIs
 * @defgroup subsys_tracing_apis_lifo LIFO Tracing APIs
 * @{
 */

/**
 * @brief Trace initialization of LIFO Queue entry
 * @param lifo LIFO object
 */
#define sys_port_trace_k_lifo_init_enter(lifo)

/**
 * @brief Trace initialization of LIFO Queue exit
 * @param lifo LIFO object
 */
#define sys_port_trace_k_lifo_init_exit(lifo)

/**
 * @brief Trace LIFO Queue put entry
 * @param lifo LIFO object
 * @param data Data item
 */
#define sys_port_trace_k_lifo_put_enter(lifo, data)

/**
 * @brief Trace LIFO Queue put exit
 * @param lifo LIFO object
 * @param data Data item
 */
#define sys_port_trace_k_lifo_put_exit(lifo, data)

/**
 * @brief Trace LIFO Queue alloc put entry
 * @param lifo LIFO object
 * @param data Data item
 */
#define sys_port_trace_k_lifo_alloc_put_enter(lifo, data)

/**
 * @brief Trace LIFO Queue alloc put exit
 * @param lifo LIFO object
 * @param data Data item
 * @param ret Return value
 */
#define sys_port_trace_k_lifo_alloc_put_exit(lifo, data, ret)

/**
 * @brief Trace LIFO Queue get entry
 * @param lifo LIFO object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_lifo_get_enter(lifo, timeout)

/**
 * @brief Trace LIFO Queue get exit
 * @param lifo LIFO object
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_lifo_get_exit(lifo, timeout, ret)

/** @} */ /* end of subsys_tracing_apis_lifo */

/**
 * @brief Stack Tracing APIs
 * @defgroup subsys_tracing_apis_stack Stack Tracing APIs
 * @{
 */

/**
 * @brief Trace initialization of Stack
 * @param stack Stack object
 */
#define sys_port_trace_k_stack_init(stack)

/**
 * @brief Trace Stack alloc init attempt entry
 * @param stack Stack object
 */
#define sys_port_trace_k_stack_alloc_init_enter(stack)

/**
 * @brief Trace Stack alloc init outcome
 * @param stack Stack object
 * @param ret Return value
 */
#define sys_port_trace_k_stack_alloc_init_exit(stack, ret)

/**
 * @brief Trace Stack cleanup attempt entry
 * @param stack Stack object
 */
#define sys_port_trace_k_stack_cleanup_enter(stack)

/**
 * @brief Trace Stack cleanup outcome
 * @param stack Stack object
 * @param ret Return value
 */
#define sys_port_trace_k_stack_cleanup_exit(stack, ret)

/**
 * @brief Trace Stack push attempt entry
 * @param stack Stack object
 */
#define sys_port_trace_k_stack_push_enter(stack)

/**
 * @brief Trace Stack push attempt outcome
 * @param stack Stack object
 * @param ret Return value
 */
#define sys_port_trace_k_stack_push_exit(stack, ret)

/**
 * @brief Trace Stack pop attempt entry
 * @param stack Stack object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_stack_pop_enter(stack, timeout)

/**
 * @brief Trace Stack pop attempt blocking
 * @param stack Stack object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_stack_pop_blocking(stack, timeout)

/**
 * @brief Trace Stack pop attempt outcome
 * @param stack Stack object
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_stack_pop_exit(stack, timeout, ret)

/** @} */ /* end of subsys_tracing_apis_stack */

/**
 * @brief Message Queue Tracing APIs
 * @defgroup subsys_tracing_apis_msgq Message Queue Tracing APIs
 * @{
 */

/**
 * @brief Trace initialization of Message Queue
 * @param msgq Message Queue object
 */
#define sys_port_trace_k_msgq_init(msgq)

/**
 * @brief Trace Message Queue alloc init attempt entry
 * @param msgq Message Queue object
 */
#define sys_port_trace_k_msgq_alloc_init_enter(msgq)

/**
 * @brief Trace Message Queue alloc init attempt outcome
 * @param msgq Message Queue object
 * @param ret Return value
 */
#define sys_port_trace_k_msgq_alloc_init_exit(msgq, ret)

/**
 * @brief Trace Message Queue cleanup attempt entry
 * @param msgq Message Queue object
 */
#define sys_port_trace_k_msgq_cleanup_enter(msgq)

/**
 * @brief Trace Message Queue cleanup attempt outcome
 * @param msgq Message Queue object
 * @param ret Return value
 */
#define sys_port_trace_k_msgq_cleanup_exit(msgq, ret)

/**
 * @brief Trace Message Queue put attempt entry
 * @param msgq Message Queue object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_msgq_put_enter(msgq, timeout)

/**
 * @brief Trace Message Queue put attempt blocking
 * @param msgq Message Queue object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_msgq_put_blocking(msgq, timeout)

/**
 * @brief Trace Message Queue put attempt outcome
 * @param msgq Message Queue object
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_msgq_put_exit(msgq, timeout, ret)

/**
 * @brief Trace Message Queue get attempt entry
 * @param msgq Message Queue object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_msgq_get_enter(msgq, timeout)

/**
 * @brief Trace Message Queue get attempt blockings
 * @param msgq Message Queue object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_msgq_get_blocking(msgq, timeout)

/**
 * @brief Trace Message Queue get attempt outcome
 * @param msgq Message Queue object
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_msgq_get_exit(msgq, timeout, ret)

/**
 * @brief Trace Message Queue peek
 * @param msgq Message Queue object
 * @param ret Return value
 */
#define sys_port_trace_k_msgq_peek(msgq, ret)

/**
 * @brief Trace Message Queue purge
 * @param msgq Message Queue object
 */
#define sys_port_trace_k_msgq_purge(msgq)

/** @} */ /* end of subsys_tracing_apis_msgq */

/**
 * @brief Mailbox Tracing APIs
 * @defgroup subsys_tracing_apis_mbox Mailbox Tracing APIs
 * @{
 */

/**
 * @brief Trace initialization of Mailbox
 * @param mbox Mailbox object
 */
#define sys_port_trace_k_mbox_init(mbox)

/**
 * @brief Trace Mailbox message put attempt entry
 * @param mbox Mailbox object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_mbox_message_put_enter(mbox, timeout)

/**
 * @brief Trace Mailbox message put attempt blocking
 * @param mbox Mailbox object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_mbox_message_put_blocking(mbox, timeout)

/**
 * @brief Trace Mailbox message put attempt outcome
 * @param mbox Mailbox object
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_mbox_message_put_exit(mbox, timeout, ret)

/**
 * @brief Trace Mailbox put attempt entry
 * @param mbox Mailbox object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_mbox_put_enter(mbox, timeout)

/**
 * @brief Trace Mailbox put attempt blocking
 * @param mbox Mailbox object
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_mbox_put_exit(mbox, timeout, ret)

/**
 * @brief Trace Mailbox async put entry
 * @param mbox Mailbox object
 * @param sem Semaphore object
 */
#define sys_port_trace_k_mbox_async_put_enter(mbox, sem)

/**
 * @brief Trace Mailbox async put exit
 * @param mbox Mailbox object
 * @param sem Semaphore object
 */
#define sys_port_trace_k_mbox_async_put_exit(mbox, sem)

/**
 * @brief Trace Mailbox get attempt entry
 * @param mbox Mailbox entry
 * @param timeout Timeout period
 */
#define sys_port_trace_k_mbox_get_enter(mbox, timeout)

/**
 * @brief Trace Mailbox get attempt blocking
 * @param mbox Mailbox entry
 * @param timeout Timeout period
 */
#define sys_port_trace_k_mbox_get_blocking(mbox, timeout)

/**
 * @brief Trace Mailbox get attempt outcome
 * @param mbox Mailbox entry
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_mbox_get_exit(mbox, timeout, ret)

/**
 * @brief Trace Mailbox data get
 * @brief rx_msg Receive Message object
 */
#define sys_port_trace_k_mbox_data_get(rx_msg)

/** @} */ /* end of subsys_tracing_apis_mbox */

/**
 * @brief Pipe Tracing APIs
 * @defgroup subsys_tracing_apis_pipe Pipe Tracing APIs
 * @{
 */

/**
 * @brief Trace initialization of Pipe
 * @param pipe Pipe object
 */
#define sys_port_trace_k_pipe_init(pipe)

/**
 * @brief Trace Pipe cleanup entry
 * @param pipe Pipe object
 */
#define sys_port_trace_k_pipe_cleanup_enter(pipe)

/**
 * @brief Trace Pipe cleanup exit
 * @param pipe Pipe object
 * @param ret Return value
 */
#define sys_port_trace_k_pipe_cleanup_exit(pipe, ret)

/**
 * @brief Trace Pipe alloc init entry
 * @param pipe Pipe object
 */
#define sys_port_trace_k_pipe_alloc_init_enter(pipe)

/**
 * @brief Trace Pipe alloc init exit
 * @param pipe Pipe object
 * @param ret Return value
 */
#define sys_port_trace_k_pipe_alloc_init_exit(pipe, ret)

/**
 * @brief Trace Pipe flush entry
 * @param pipe Pipe object
 */
#define sys_port_trace_k_pipe_flush_enter(pipe)

/**
 * @brief Trace Pipe flush exit
 * @param pipe Pipe object
 */
#define sys_port_trace_k_pipe_flush_exit(pipe)

/**
 * @brief Trace Pipe buffer flush entry
 * @param pipe Pipe object
 */
#define sys_port_trace_k_pipe_buffer_flush_enter(pipe)

/**
 * @brief Trace Pipe buffer flush exit
 * @param pipe Pipe object
 */
#define sys_port_trace_k_pipe_buffer_flush_exit(pipe)

/**
 * @brief Trace Pipe put attempt entry
 * @param pipe Pipe object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_pipe_put_enter(pipe, timeout)

/**
 * @brief Trace Pipe put attempt blocking
 * @param pipe Pipe object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_pipe_put_blocking(pipe, timeout)

/**
 * @brief Trace Pipe put attempt outcome
 * @param pipe Pipe object
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_pipe_put_exit(pipe, timeout, ret)

/**
 * @brief Trace Pipe get attempt entry
 * @param pipe Pipe object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_pipe_get_enter(pipe, timeout)

/**
 * @brief Trace Pipe get attempt blocking
 * @param pipe Pipe object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_pipe_get_blocking(pipe, timeout)

/**
 * @brief Trace Pipe get attempt outcome
 * @param pipe Pipe object
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_pipe_get_exit(pipe, timeout, ret)

/** @} */ /* end of subsys_tracing_apis_pipe */

/**
 * @brief Heap Tracing APIs
 * @defgroup subsys_tracing_apis_heap Heap Tracing APIs
 * @{
 */

/**
 * @brief Trace initialization of Heap
 * @param h Heap object
 */
#define sys_port_trace_k_heap_init(h)

/**
 * @brief Trace Heap aligned alloc attempt entry
 * @param h Heap object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_heap_aligned_alloc_enter(h, timeout)

/**
 * @brief Trace Heap align alloc attempt blocking
 * @param h Heap object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_heap_aligned_alloc_blocking(h, timeout)

/**
 * @brief Trace Heap align alloc attempt outcome
 * @param h Heap object
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_heap_aligned_alloc_exit(h, timeout, ret)

/**
 * @brief Trace Heap alloc enter
 * @param h Heap object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_heap_alloc_enter(h, timeout)

/**
 * @brief Trace Heap alloc exit
 * @param h Heap object
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_heap_alloc_exit(h, timeout, ret)

/**
 * @brief Trace Heap free
 * @param h Heap object
 */
#define sys_port_trace_k_heap_free(h)

/**
 * @brief Trace Heap realloc enter
 * @param h Heap object
 * @param ptr Pointer to reallocate
 * @param bytes Bytes to reallocate
 * @param timeout Timeout period
 */
#define sys_port_trace_k_heap_realloc_enter(h, ptr, bytes, timeout)

/**
 * @brief Trace Heap realloc exit
 * @param h Heap object
 * @param ptr Pointer to reallocate
 * @param bytes Bytes to reallocate
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_heap_realloc_exit(h, ptr, bytes, timeout, ret)

/**
 * @brief Trace System Heap aligned alloc enter
 * @param heap Heap object
 */
#define sys_port_trace_k_heap_sys_k_aligned_alloc_enter(heap)

/**
 * @brief Trace System Heap aligned alloc exit
 * @param heap Heap object
 * @param ret Return value
 */
#define sys_port_trace_k_heap_sys_k_aligned_alloc_exit(heap, ret)

/**
 * @brief Trace System Heap aligned alloc enter
 * @param heap Heap object
 */
#define sys_port_trace_k_heap_sys_k_malloc_enter(heap)

/**
 * @brief Trace System Heap aligned alloc exit
 * @param heap Heap object
 * @param ret Return value
 */
#define sys_port_trace_k_heap_sys_k_malloc_exit(heap, ret)

/**
 * @brief Trace System Heap free entry
 * @param heap Heap object
 * @param heap_ref Heap reference
 */
#define sys_port_trace_k_heap_sys_k_free_enter(heap, heap_ref)

/**
 * @brief Trace System Heap free exit
 * @param heap Heap object
 * @param heap_ref Heap reference
 */
#define sys_port_trace_k_heap_sys_k_free_exit(heap, heap_ref)

/**
 * @brief Trace System heap calloc enter
 * @param heap
 */
#define sys_port_trace_k_heap_sys_k_calloc_enter(heap)

/**
 * @brief Trace System heap calloc exit
 * @param heap Heap object
 * @param ret Return value
 */
#define sys_port_trace_k_heap_sys_k_calloc_exit(heap, ret)

/**
 * @brief Trace System heap realloc enter
 * @param heap
 * @param ptr
 */
#define sys_port_trace_k_heap_sys_k_realloc_enter(heap, ptr)

/**
 * @brief Trace System heap realloc exit
 * @param heap Heap object
 * @param ptr Memory pointer
 * @param ret Return value
 */
#define sys_port_trace_k_heap_sys_k_realloc_exit(heap, ptr, ret)

/** @} */ /* end of subsys_tracing_apis_heap */

/**
 * @brief Memory Slab Tracing APIs
 * @defgroup subsys_tracing_apis_mslab Memory Slab Tracing APIs
 * @{
 */

/**
 * @brief Trace initialization of Memory Slab
 * @param slab Memory Slab object
 * @param rc Return value
 */
#define sys_port_trace_k_mem_slab_init(slab, rc)

/**
 * @brief Trace Memory Slab alloc attempt entry
 * @param slab Memory Slab object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_mem_slab_alloc_enter(slab, timeout)

/**
 * @brief Trace Memory Slab alloc attempt blocking
 * @param slab Memory Slab object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_mem_slab_alloc_blocking(slab, timeout)

/**
 * @brief Trace Memory Slab alloc attempt outcome
 * @param slab Memory Slab object
 * @param timeout Timeout period
 * @param ret Return value
 */
#define sys_port_trace_k_mem_slab_alloc_exit(slab, timeout, ret)

/**
 * @brief Trace Memory Slab free entry
 * @param slab Memory Slab object
 */
#define sys_port_trace_k_mem_slab_free_enter(slab)

/**
 * @brief Trace Memory Slab free exit
 * @param slab Memory Slab object
 */
#define sys_port_trace_k_mem_slab_free_exit(slab)

/** @} */ /* end of subsys_tracing_apis_mslab */

/**
 * @brief Timer Tracing APIs
 * @defgroup subsys_tracing_apis_timer Timer Tracing APIs
 * @{
 */

/**
 * @brief Trace initialization of Timer
 * @param timer Timer object
 */
#define sys_port_trace_k_timer_init(timer)

/**
 * @brief Trace Timer start
 * @param timer Timer object
 * @param duration Timer duration
 * @param period Timer period
 */
#define sys_port_trace_k_timer_start(timer, duration, period)

/**
 * @brief Trace Timer stop
 * @param timer Timer object
 */
#define sys_port_trace_k_timer_stop(timer)

/**
 * @brief Trace Timer status sync entry
 * @param timer Timer object
 */
#define sys_port_trace_k_timer_status_sync_enter(timer)

/**
 * @brief Trace Timer Status sync blocking
 * @param timer Timer object
 * @param timeout Timeout period
 */
#define sys_port_trace_k_timer_status_sync_blocking(timer, timeout)

/**
 * @brief Trace Time Status sync outcome
 * @param timer Timer object
 * @param result Return value
 */
#define sys_port_trace_k_timer_status_sync_exit(timer, result)

/** @} */ /* end of subsys_tracing_apis_timer */

/**
 * @brief Event Tracing APIs
 * @defgroup subsys_tracing_apis_event Event Tracing APIs
 * @{
 */

/**
 * @brief Trace initialisation of an Event
 * @param event Event object
 */
#define sys_port_trace_k_event_init(event)

/**
 * @brief Trace posting of an Event call entry
 * @param event Event object
 * @param events Set of posted events
 * @param events_mask Mask to apply against posted events
 */
#define sys_port_trace_k_event_post_enter(event, events, events_mask)

/**
 * @brief Trace posting of an Event call exit
 * @param event Event object
 * @param events Set of posted events
 * @param events_mask Mask to apply against posted events
 */
#define sys_port_trace_k_event_post_exit(event, events, events_mask)

/**
 * @brief Trace waiting of an Event call entry
 * @param event Event object
 * @param events Set of events for which to wait
 * @param options Event wait options
 * @param timeout Timeout period
 */
#define sys_port_trace_k_event_wait_enter(event, events, options, timeout)

/**
 * @brief Trace waiting of an Event call exit
 * @param event Event object
 * @param events Set of events for which to wait
 * @param options Event wait options
 * @param timeout Timeout period
 */
#define sys_port_trace_k_event_wait_blocking(event, events, options, timeout)

/**
 * @brief Trace waiting of an Event call exit
 * @param event Event object
 * @param events Set of events for which to wait
 * @param ret Set of received events
 */
#define sys_port_trace_k_event_wait_exit(event, events, ret)

/** @} */ /* end of subsys_tracing_apis_event */

/**
 * @brief System PM Tracing APIs
 * @defgroup subsys_tracing_apis_pm_system System PM Tracing APIs
 * @{
 */

/**
 * @brief Trace system suspend call entry.
 * @param ticks Ticks.
 */
#define sys_port_trace_pm_system_suspend_enter(ticks)

/**
 * @brief Trace system suspend call exit.
 * @param ticks Ticks.
 * @param state PM state.
 */
#define sys_port_trace_pm_system_suspend_exit(ticks, state)

/** @} */ /* end of subsys_tracing_apis_pm_system */

/**
 * @brief PM Device Runtime Tracing APIs
 * @defgroup subsys_tracing_apis_pm_device_runtime PM Device Runtime Tracing APIs
 * @{
 */

/**
 * @brief Trace getting a device call entry.
 * @param dev Device instance.
 */
#define sys_port_trace_pm_device_runtime_get_enter(dev)

/**
 * @brief Trace getting a device call exit.
 * @param dev Device instance.
 * @param ret Return value.
 */
#define sys_port_trace_pm_device_runtime_get_exit(dev, ret)

/**
 * @brief Trace putting a device call entry.
 * @param dev Device instance.
 */
#define sys_port_trace_pm_device_runtime_put_enter(dev)

/**
 * @brief Trace putting a device call exit.
 * @param dev Device instance.
 * @param ret Return value.
 */
#define sys_port_trace_pm_device_runtime_put_exit(dev, ret)

/**
 * @brief Trace putting a device (asynchronously) call entry.
 * @param dev Device instance.
 * @param delay Time to delay the operation
 */
#define sys_port_trace_pm_device_runtime_put_async_enter(dev, delay)

/**
 * @brief Trace putting a device (asynchronously) call exit.
 * @param dev Device instance.
 * @param delay Time to delay the operation.
 * @param ret Return value.
 */
#define sys_port_trace_pm_device_runtime_put_async_exit(dev, delay, ret)

/**
 * @brief Trace enabling device runtime PM call entry.
 * @param dev Device instance.
 */
#define sys_port_trace_pm_device_runtime_enable_enter(dev)

/**
 * @brief Trace enabling device runtime PM call exit.
 * @param dev Device instance.
 * @param ret Return value.
 */
#define sys_port_trace_pm_device_runtime_enable_exit(dev, ret)

/**
 * @brief Trace disabling device runtime PM call entry.
 * @param dev Device instance.
 */
#define sys_port_trace_pm_device_runtime_disable_enter(dev)

/**
 * @brief Trace disabling device runtime PM call exit.
 * @param dev Device instance.
 * @param ret Return value.
 */
#define sys_port_trace_pm_device_runtime_disable_exit(dev, ret)

/** @} */ /* end of subsys_tracing_apis_pm_device_runtime */

/**
 * @brief Network Core Tracing APIs
 * @defgroup subsys_tracing_apis_net Network Core Tracing APIs
 * @{
 */

/**
 * @brief Trace network data receive
 * @param iface Network interface
 * @param pkt Received network packet
 */
#define sys_port_trace_net_recv_data_enter(iface, pkt)

/**
 * @brief Trace network data receive attempt
 * @param iface Network interface
 * @param pkt Received network packet
 * @param ret Return value
 */
#define sys_port_trace_net_recv_data_exit(iface, pkt, ret)

/**
 * @brief Trace network data send
 * @param pkt Network packet to send
 */
#define sys_port_trace_net_send_data_enter(pkt)

/**
 * @brief Trace network data send attempt
 * @param pkt Received network packet
 * @param ret Return value
 */
#define sys_port_trace_net_send_data_exit(pkt, ret)

/**
 * @brief Trace network data receive time
 * @param pkt Received network packet
 * @param end_time When the RX processing stopped for this pkt (in ticks)
 */
#define sys_port_trace_net_rx_time(pkt, end_time)

/**
 * @brief Trace network data sent time
 * @param pkt Sent network packet
 * @param end_time When the TX processing stopped for this pkt (in ticks)
 */
#define sys_port_trace_net_tx_time(pkt, end_time)

/** @} */ /* end of subsys_tracing_apis_net */

/**
 * @brief Network Socket Tracing APIs
 * @defgroup subsys_tracing_apis_socket Network Socket Tracing APIs
 * @{
 */

/**
 * @brief Trace init of network sockets
 * @param socket Network socket is returned
 * @param family Socket address family
 * @param type Socket type
 * @param proto Socket protocol
 */
#define sys_port_trace_socket_init(socket, family, type, proto)

/**
 * @brief Trace close of network sockets
 * @param socket Socket object
 */
#define sys_port_trace_socket_close_enter(socket)

/**
 * @brief Trace network socket close attempt
 * @param socket Socket object
 * @param ret Return value
 */
#define sys_port_trace_socket_close_exit(socket, ret)

/**
 * @brief Trace shutdown of network sockets
 * @param socket Socket object
 * @param how Socket shutdown type
 */
#define sys_port_trace_socket_shutdown_enter(socket, how)

/**
 * @brief Trace network socket shutdown attempt
 * @param socket Socket object
 * @param ret Return value
 */
#define sys_port_trace_socket_shutdown_exit(socket, ret)

/**
 * @brief Trace bind of network sockets
 * @param socket Socket object
 * @param addr Network address to bind
 * @param addrlen Address length
 */
#define sys_port_trace_socket_bind_enter(socket, addr, addrlen)

/**
 * @brief Trace network socket bind attempt
 * @param socket Socket object
 * @param ret Return value
 */
#define sys_port_trace_socket_bind_exit(socket, ret)

/**
 * @brief Trace connect of network sockets
 * @param socket Socket object
 * @param addr Network address to bind
 * @param addrlen Address length
 */
#define sys_port_trace_socket_connect_enter(socket, addr, addrlen)

/**
 * @brief Trace network socket connect attempt
 * @param socket Socket object
 * @param ret Return value
 */
#define sys_port_trace_socket_connect_exit(socket, ret)

/**
 * @brief Trace listen of network sockets
 * @param socket Socket object
 * @param backlog Socket backlog length
 */
#define sys_port_trace_socket_listen_enter(socket, backlog)

/**
 * @brief Trace network socket listen attempt
 * @param socket Socket object
 * @param ret Return value
 */
#define sys_port_trace_socket_listen_exit(socket, ret)

/**
 * @brief Trace accept of network sockets
 * @param socket Socket object
 */
#define sys_port_trace_socket_accept_enter(socket)

/**
 * @brief Trace network socket accept attempt
 * @param socket Socket object
 * @param addr Peer network address
 * @param addrlen Network address length
 * @param ret Return value
 */
#define sys_port_trace_socket_accept_exit(socket, addr, addrlen, ret)

/**
 * @brief Trace sendto of network sockets
 * @param socket Socket object
 * @param len Length of the data to send
 * @param flags Flags for this send operation
 * @param dest_addr Destination network address
 * @param addrlen Network address length
 */
#define sys_port_trace_socket_sendto_enter(socket, len, flags, dest_addr, addrlen)

/**
 * @brief Trace network socket sendto attempt
 * @param socket Socket object
 * @param ret Return value
 */
#define sys_port_trace_socket_sendto_exit(socket, ret)

/**
 * @brief Trace sendmsg of network sockets
 * @param socket Socket object
 * @param msg Data to send
 * @param flags Flags for this send operation
 */
#define sys_port_trace_socket_sendmsg_enter(socket, msg, flags)

/**
 * @brief Trace network socket sendmsg attempt
 * @param socket Socket object
 * @param ret Return value
 */
#define sys_port_trace_socket_sendmsg_exit(socket, ret)

/**
 * @brief Trace recvfrom of network sockets
 * @param socket Socket object
 * @param max_len Maximum length of the data we can receive
 * @param flags Flags for this receive operation
 * @param addr Remote network address
 * @param addrlen Network address length
 */
#define sys_port_trace_socket_recvfrom_enter(socket, max_len, flags, addr, addrlen)

/**
 * @brief Trace network socket recvfrom attempt
 * @param socket Socket object
 * @param src_addr Peer network address that send the data
 * @param addrlen Length of the network address
 * @param ret Return value
 */
#define sys_port_trace_socket_recvfrom_exit(socket, src_addr, addrlen, ret)

/**
 * @brief Trace recvmsg of network sockets
 * @param socket Socket object
 * @param msg Message buffer to receive
 * @param flags Flags for this receive operation
 */
#define sys_port_trace_socket_recvmsg_enter(socket, msg, flags)

/**
 * @brief Trace network socket recvmsg attempt
 * @param socket Socket object
 * @param msg Message buffer received
 * @param ret Return value
 */
#define sys_port_trace_socket_recvmsg_exit(socket, msg, ret)

/**
 * @brief Trace fcntl of network sockets
 * @param socket Socket object
 * @param cmd Command to set for this socket
 * @param flags Flags for this receive operation
 */
#define sys_port_trace_socket_fcntl_enter(socket, cmd, flags)

/**
 * @brief Trace network socket fcntl attempt
 * @param socket Socket object
 * @param ret Return value
 */
#define sys_port_trace_socket_fcntl_exit(socket, ret)

/**
 * @brief Trace ioctl of network sockets
 * @param socket Socket object
 * @param req Request to set for this socket
 */
#define sys_port_trace_socket_ioctl_enter(socket, req)

/**
 * @brief Trace network socket ioctl attempt
 * @param socket Socket object
 * @param ret Return value
 */
#define sys_port_trace_socket_ioctl_exit(socket, ret)

/**
 * @brief Trace polling of network sockets
 * @param fds Set of socket object
 * @param nfds Number of socket objects in the set
 * @param timeout Timeout for the poll operation
 */
#define sys_port_trace_socket_poll_enter(fds, nfds, timeout)

/**
 * @brief Trace network socket poll attempt
 * @param fds Set of socket object
 * @param nfds Number of socket objects in the set
 * @param ret Return value
 */
#define sys_port_trace_socket_poll_exit(fds, nfds, ret)

/**
 * @brief Trace getsockopt of network sockets
 * @param socket Socket object
 * @param level Option level
 * @param optname Option name
 */
#define sys_port_trace_socket_getsockopt_enter(socket, level, optname)

/**
 * @brief Trace network socket getsockopt attempt
 * @param socket Socket object
 * @param level Option level
 * @param optname Option name
 * @param optval Option value
 * @param optlen Option value length
 * @param ret Return value
 */
#define sys_port_trace_socket_getsockopt_exit(socket, level, optname, optval, optlen, ret)

/**
 * @brief Trace setsockopt of network sockets
 * @param socket Socket object
 * @param level Option level
 * @param optname Option name
 * @param optval Option value
 * @param optlen Option value length
 */
#define sys_port_trace_socket_setsockopt_enter(socket, level, optname, optval, optlen)

/**
 * @brief Trace network socket setsockopt attempt
 * @param socket Socket object
 * @param ret Return value
 */
#define sys_port_trace_socket_setsockopt_exit(socket, ret)

/**
 * @brief Trace getpeername of network sockets
 * @param socket Socket object
 */
#define sys_port_trace_socket_getpeername_enter(socket)

/**
 * @brief Trace network socket getpeername attempt
 * @param socket Socket object
 * @param addr Peer socket network address
 * @param addrlen Length of the network address
 * @param ret Return value
 */
#define sys_port_trace_socket_getpeername_exit(socket, addr, addrlen, ret)

/**
 * @brief Trace getsockname of network sockets
 * @param socket Socket object
 */
#define sys_port_trace_socket_getsockname_enter(socket)

/**
 * @brief Trace network socket getsockname attempt
 * @param socket Socket object
 * @param addr Local socket network address
 * @param addrlen Length of the network address
 * @param ret Return value
 */
#define sys_port_trace_socket_getsockname_exit(socket, addr, addrlen, ret)

/**
 * @brief Trace socketpair enter call
 * @param family Network address family
 * @param type Socket type
 * @param proto Socket protocol
 * @param sv Socketpair buffer
 */
#define sys_port_trace_socket_socketpair_enter(family, type, proto, sv)

/**
 * @brief Trace network socketpair open attempt
 * @param socket_A Socketpair first socket object
 * @param socket_B Socketpair second socket object
 * @param ret Return value
 */
#define sys_port_trace_socket_socketpair_exit(socket_A, socket_B, ret)

/** @} */ /* end of subsys_tracing_apis_socket */

/**
 * @brief Named Tracing APIs
 * @defgroup subsys_tracing_apis_named Named tracing APIs
 * @{
 */

/*
 * @brief Called by user to generate named events
 *
 * @param name name of event. Tracing subsystems may place a limit on
 * the length of this string
 * @param arg0 arbitrary user-provided data for this event
 * @param arg1 arbitrary user-provided data for this event
 */
#define sys_trace_named_event(name, arg0, arg1)

/** @} */ /* end of subsys_tracing_apis_named */

#if defined(CONFIG_PERCEPIO_TRACERECORDER)
#include "tracing_tracerecorder.h"
#else
/**
 * @brief Called when entering an ISR
 */
void sys_trace_isr_enter(void);

/**
 * @brief Called when exiting an ISR
 */
void sys_trace_isr_exit(void);

/**
 * @brief Called when exiting an ISR and switching to scheduler
 */
void sys_trace_isr_exit_to_scheduler(void);

/**
 * @brief Called when the cpu enters the idle state
 */
void sys_trace_idle(void);
#endif /* CONFIG_PERCEPIO_TRACERECORDER */

/**
 * @brief Called when entering an init function
 */
#define sys_trace_sys_init_enter(entry, level)

/**
 * @brief Called when exiting an init function
 */
#define sys_trace_sys_init_exit(entry, level, result)

/** @} */ /* end of subsys_tracing_apis */

/** @} */ /* end of subsys_tracing */

#endif
#endif /* ZEPHYR_INCLUDE_TRACING_TRACING_H_ */
