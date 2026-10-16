/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Generic call offloading engine.
 *
 * An offloading engine is a pool of worker threads fed by a FIFO. A caller hands a function and an
 * argument blob to offloader_dispatch() and blocks until a worker has run it, so the stack depth
 * of the call is paid once by the engine instead of once by every calling thread.
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
	struct k_fifo fifo;
	struct k_thread *threads;
	size_t num_threads;
	/** @endcond */
};

/**
 * @brief Function executed by an offloading engine on behalf of a caller.
 *
 * @param args Argument blob passed to offloader_dispatch().
 */
typedef void (*offloader_work_fn_t)(void *args);

/** @cond INTERNAL_HIDDEN */
struct offloader_req {
	sys_sfnode_t node;
	struct k_sem done;
	offloader_work_fn_t fn;
	void *args;
};

void offloader_engine_main(void *p1, void *p2, void *p3);
/** @endcond */

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
 * @note The function runs at the priority of the engine, not at the priority of the caller. There
 *       is no priority inheritance.
 * @note Must not be called from an ISR.
 */
void offloader_dispatch(struct offloader_engine *engine, offloader_work_fn_t fn, void *args);

/**
 * @brief Define an offloading engine and its worker threads.
 *
 * Defines the engine @p _name at file scope together with its thread objects and stacks, and
 * registers a SYS_INIT() entry at POST_KERNEL, CONFIG_OFFLOADER_INIT_PRIORITY that starts the
 * worker threads.
 *
 * @param _name Name of the engine.
 * @param _num_threads Number of worker threads to start.
 * @param _stack_size Stack size of each worker thread, in bytes.
 * @param _prio Priority the worker threads run at.
 */
#define OFFLOADER_ENGINE_DEFINE(_name, _num_threads, _stack_size, _prio)                           \
	static K_KERNEL_STACK_ARRAY_DEFINE(_name##_stacks, _num_threads, _stack_size);             \
	static struct k_thread _name##_threads[_num_threads];                                      \
	struct offloader_engine _name;                                                             \
	static int _name##_init(void)                                                              \
	{                                                                                          \
		k_fifo_init(&_name.fifo);                                                          \
		_name.num_threads = _num_threads;                                                  \
		_name.threads = _name##_threads;                                                   \
		for (size_t i = 0; i < ARRAY_SIZE(_name##_threads); i++) {                         \
			k_tid_t tid = k_thread_create(&_name##_threads[i], _name##_stacks[i],      \
						      K_KERNEL_STACK_SIZEOF(_name##_stacks[i]),    \
						      offloader_engine_main, &_name, NULL, NULL,   \
						      _prio, 0, K_NO_WAIT);                        \
			(void)k_thread_name_set(tid, #_name);                                      \
		}                                                                                  \
		return 0;                                                                          \
	}                                                                                          \
	SYS_INIT(_name##_init, POST_KERNEL, CONFIG_OFFLOADER_INIT_PRIORITY)

/**
 * @brief Declare an offloading engine defined in another translation unit.
 *
 * @param _name Name of the engine.
 */
#define OFFLOADER_ENGINE_DECLARE(_name) extern struct offloader_engine _name

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* _OFFLOADER_ENGINE_H_ */
