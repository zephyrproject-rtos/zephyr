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
 * @brief Queue Tracing APIs
 * @defgroup queue_tracing_apis Queue Tracing APIs
 * @ingroup tracing_apis
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
#define sys_port_trace_queue_remove_exit(queue, ret)

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

/**
 * @}
 */ /* end of queue_tracing_apis */




/**
 * @brief FIFO Tracing APIs
 * @defgroup fifo_tracing_apis FIFO Tracing APIs
 * @ingroup tracing_apis
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
#define sys_port_trace_k_fifo_alloc_put_list_enter(fifo, head, tail)

/**
 * @brief Trace FIFO Queue put list exit
 * @param fifo FIFO object
 * @param head First ll-node
 * @param tail Last ll-node
 */
#define sys_port_trace_k_fifo_alloc_put_list_exit(fifo, head, tail)

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
#define sys_port_trace_k_fifo_peek_head_entry(fifo)

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
#define sys_port_trace_k_fifo_peek_tail_entry(fifo)

/**
 * @brief Trace FIFO Queue peek tail exit
 * @param fifo FIFO object
 * @param ret Return value
 */
#define sys_port_trace_k_fifo_peek_tail_exit(fifo, ret)

/**
 * @}
 */ /* end of fifo_tracing_apis */




/**
 * @brief LIFO Tracing APIs
 * @defgroup lifo_tracing_apis LIFO Tracing APIs
 * @ingroup tracing_apis
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

/**
 * @}
 */ /* end of lifo_tracing_apis */




/**
 * @brief Stack Tracing APIs
 * @defgroup stack_tracing_apis Stack Tracing APIs
 * @ingroup tracing_apis
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

/**
 * @}
 */ /* end of stack_tracing_apis */




/**
 * @brief Message Queue Tracing APIs
 * @defgroup msgq_tracing_apis Message Queue Tracing APIs
 * @ingroup tracing_apis
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

/**
 * @}
 */ /* end of msgq_tracing_apis */




/**
 * @brief Mailbox Tracing APIs
 * @defgroup mbox_tracing_apis Mailbox Tracing APIs
 * @ingroup tracing_apis
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

/**
 * @}
 */ /* end of mbox_tracing_apis */


/**
 * @}
 */

#endif
#endif
