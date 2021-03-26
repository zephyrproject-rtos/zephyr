/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_TRACING_TRACING_H_
#define ZEPHYR_INCLUDE_TRACING_TRACING_H_

#include <kernel.h>

/* Below IDs are used with systemview, not final to the zephyr tracing API */
#define SYS_TRACE_ID_OFFSET                  (32u)

#define SYS_TRACE_ID_MUTEX_INIT              (1u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_MUTEX_UNLOCK            (2u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_MUTEX_LOCK              (3u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_SEMA_INIT               (4u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_SEMA_GIVE               (5u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_SEMA_TAKE               (6u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_SLEEP                   (7u + SYS_TRACE_ID_OFFSET)

#if defined CONFIG_SEGGER_SYSTEMVIEW
#include "tracing_sysview.h"

#elif defined CONFIG_TRACING_CTF
#include "tracing_ctf.h"

#elif defined CONFIG_TRACING_TEST
#include "tracing_test.h"

#else

/**
 * @brief Tracing APIs
 * @defgroup tracing_apis Tracing APIs
 * @{
 */

/**
 * @brief Called when entering an ISR
 */
#define sys_trace_isr_enter()

/**
 * @brief Called when exiting an ISR
 */
#define sys_trace_isr_exit()

/**
 * @brief Called when exiting an ISR and switching to scheduler
 */
#define sys_trace_isr_exit_to_scheduler()

/**
 * @brief Called when the cpu enters the idle state
 */
#define sys_trace_idle()
/**
 * @}
 */


/**
 * @brief Tracing APIs
 * @defgroup tracing_apis Tracing APIs
 * @{
 */


/**
 * @brief Thread Tracing APIs
 * @defgroup thread_tracing_apis Thread Tracing APIs
 * @ingroup tracing_apis
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
 * @brief Called when setting priority of a thread
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_priority_set(thread)

/**
 * @brief Called when a thread is being suspended
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_suspend(thread)

/**
 * @brief Called when a thread is being resumed from suspension
 * @param thread Thread object
 */
#define sys_port_trace_k_thread_resume(thread)

/**
 * @brief Called when the thread scheduler is locked
 */
#define sys_port_trace_k_thread_sched_lock()

/**
 * @brief Called when the thread sceduler is unlocked
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
 * @brief Trace implicit thread wakup invocation by the scheduler
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

/**
 * @}
 */ /* end of thread_tracing_apis */




/**
 * @brief Semaphore Tracing APIs
 * @defgroup sem_tracing_apis Semaphore Tracing APIs
 * @ingroup tracing_apis
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

/**
 * @}
 */ /* end of sem_tracing_apis */




/**
 * @brief Mutex Tracing APIs
 * @defgroup mutex_tracing_apis Mutex Tracing APIs
 * @ingroup tracing_apis
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

/**
 * @}
 */ /* end of mutex_tracing_apis */




/**
 * @brief Conditional Variable Tracing APIs
 * @defgroup condvar_tracing_apis Conditional Variable Tracing APIs
 * @ingroup tracing_apis
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

/**
 * @}
 */ /* end of condvar_tracing_apis */


/**
 * @}
 */

#endif
#endif
