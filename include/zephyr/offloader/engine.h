/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Generic call offloading engine.
 *
 * An offloading engine is a pool of worker threads fed by a priority-sorted queue. A caller hands
 * a function and an argument blob to offloader_dispatch() and blocks until a worker has run it, so
 * the stack depth of the call is paid once by the engine instead of once by every calling thread.
 */
#ifndef _OFFLOADER_ENGINE_H_
#define _OFFLOADER_ENGINE_H_

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup offloader Call offloading
 * @ingroup os_services
 * @{
 */

/** @brief An offloading engine and the pool of worker threads serving it. */
struct offloader_engine {
	/** @cond INTERNAL_HIDDEN */
	struct k_spinlock lock;
	_wait_q_t waitq;
	sys_slist_t queue;
	size_t num_workers;
	sys_slist_t active;
	struct k_thread *workers;
	/** @endcond */
};

/**
 * @brief Function executed by an offloading engine on behalf of a caller.
 *
 * @param args Argument blob passed to offloader_dispatch().
 */
typedef void (*offloader_work_fn_t)(void *args);

/**
 * @brief Run a function on an offloading engine and wait for it to complete.
 *
 * Queues @p fn on @p engine, blocks until a worker thread has run it, and returns. When the caller
 * is itself one of the worker threads of @p engine, @p fn is called inline instead, as queueing it
 * would deadlock.
 *
 * @param engine Engine to run the function on.
 * @param fn Function to run.
 * @param args Argument blob handed to @p fn. It must stay valid for the duration of the call.
 *
 * @note The function runs at the priority of the caller. Requests are queued by priority and in
 *	 submission order within a priority.
 * @note Must not be called from an ISR.
 */
void offloader_dispatch(struct offloader_engine *engine, offloader_work_fn_t fn, void *args);

/** @cond INTERNAL_HIDDEN */
#define OFFLOADER_ENGINE_INIT(_name, _workers, _num_workers)	\
{								\
	.waitq = Z_WAIT_Q_INIT(&_name.waitq),			\
	.queue = SYS_SLIST_STATIC_INIT(&_name.queue),		\
	.active = SYS_SLIST_STATIC_INIT(&_name.active),		\
	.workers = _workers,					\
	.num_workers = _num_workers,				\
}
void offloader_engine_thread(void *p1, void *p2, void *p3);
/** @endcond */

/**
 * @brief Define an offloading engine and its worker threads.
 *
 * Defines the engine @p _name at file scope together with its worker objects and stacks, and
 * registers a SYS_INIT() and executes it as soon as possible after kernel initialization to start
 * the worker threads. A call dispatched before that entry will deadlock.
 *
 * @param _name Name of the engine.
 * @param _num_threads Number of worker threads to start.
 * @param _stack_size Stack size of each worker thread, in bytes.
 */
#define OFFLOADER_ENGINE_DEFINE(_name, _num_threads, _stack_size)				   \
	static K_KERNEL_STACK_ARRAY_DEFINE(_name##_stacks, _num_threads, _stack_size);		   \
	static struct k_thread _name##_workers[_num_threads];					   \
	static struct offloader_engine _name =							   \
		OFFLOADER_ENGINE_INIT(_name, _name##_workers, _num_threads);			   \
	static int _name##_init(void)								   \
	{											   \
		for (size_t i = 0; i < ARRAY_SIZE(_name##_workers); i++) {			   \
			k_tid_t tid = k_thread_create(&_name##_workers[i],			   \
						      _name##_stacks[i],			   \
						      K_KERNEL_STACK_SIZEOF(_name##_stacks[i]),	   \
						      offloader_engine_thread, &_name, NULL, NULL, \
						      K_PRIO_COOP(0), 0, K_NO_WAIT);		   \
			(void)k_thread_name_set(tid, #_name);					   \
		}										   \
		return 0;									   \
	}											   \
	SYS_INIT(_name##_init, POST_KERNEL, 1)

/** @} */


#ifdef __cplusplus
}
#endif
#endif /* _OFFLOADER_ENGINE_H_ */
