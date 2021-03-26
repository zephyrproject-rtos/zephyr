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
 * @brief Called before a thread has been selected to run
 */
#define sys_trace_thread_switched_out()

/**
 * @brief Called after a thread has been selected to run
 */
#define sys_trace_thread_switched_in()

/**
 * @brief Called when setting priority of a thread
 * @param thread Thread structure
 */
#define sys_trace_thread_priority_set(thread)

/**
 * @brief Called when a thread is being created
 * @param thread Thread structure
 */
#define sys_trace_thread_create(thread)

/**
 * @brief Called when a thread is being aborted
 * @param thread Thread structure
 *
 */
#define sys_trace_thread_abort(thread)

/**
 * @brief Called when a thread is being suspended
 * @param thread Thread structure
 */
#define sys_trace_thread_suspend(thread)

/**
 * @brief Called when a thread is being resumed from suspension
 * @param thread Thread structure
 */
#define sys_trace_thread_resume(thread)

/**
 * @brief Called when a thread is ready to run
 * @param thread Thread structure
 */
#define sys_trace_thread_ready(thread)

/**
 * @brief Called when a thread is pending
 * @param thread Thread structure
 */
#define sys_trace_thread_pend(thread)

/**
 * @brief Provide information about specific thread
 * @param thread Thread structure
 */
#define sys_trace_thread_info(thread)

/**
 * @brief Called when a thread name is set
 * @param thread Thread structure
 */
#define sys_trace_thread_name_set(thread)

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
 * @brief Can be called with any id signifying a new call
 * @param id ID of the operation that was started
 */
#define sys_trace_void(id)

/**
 * @brief Can be called with any id signifying ending a call
 * @param id ID of the operation that was completed
 */
#define sys_trace_end_call(id)

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
 * @}
 */

#endif
#endif
