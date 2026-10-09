/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Generic call offloading engine.
 *
 * An offloading engine is a pool of worker threads fed by a priority-sorted queue. A caller hands
 * a function and an argument blob to call_offloader_dispatch() and blocks until a worker has run
 * it, so the stack depth of the call is paid once by the engine instead of once by every calling
 * thread.
 */
#ifndef ZEPHYR_INCLUDE_CALL_OFFLOADER_CALL_OFFLOADER_H_
#define ZEPHYR_INCLUDE_CALL_OFFLOADER_CALL_OFFLOADER_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup call_offloader Call offloading
 * @ingroup os_services
 * @{
 */

/** @cond INTERNAL_HIDDEN */
struct call_offloader_worker {
	struct k_thread thread;
	struct k_thread *client;
	k_thread_stack_t *stack;
};
/** @endcond */

/** @brief An offloading engine and the pool of worker threads serving it. */
struct call_offloader_engine {
	/** @cond INTERNAL_HIDDEN */
	struct k_spinlock lock;
	_wait_q_t pending;
	_wait_q_t idle;
	size_t num_workers;
	size_t stack_size;
	const char *name;
	struct call_offloader_worker *workers;
	/** @endcond */
};

/**
 * @brief Function executed by an offloading engine on behalf of a caller.
 *
 * @param args Argument blob passed to call_offloader_dispatch().
 */
typedef void (*call_offloader_work_fn_t)(void *args);

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
 * @note Must not be called from an ISR or before the kernel has started.
 * @note A fatal error inside @p fn kills the worker running it. The engine recreates that worker
 *	 and aborts the caller, so this function does not return in that case.
 * @note The caller must not be aborted while this function is blocked, as the worker is still
 *	 reading @p args from the caller's stack.
 */
void call_offloader_dispatch(struct call_offloader_engine *engine, call_offloader_work_fn_t fn,
			     void *args);

/** @cond INTERNAL_HIDDEN */
#define CALL_OFFLOADER_ENGINE_INIT(_name, _workers, _num_workers, _stack_size) \
{                                                                              \
	.pending = Z_WAIT_Q_INIT(&_name.pending),                              \
	.idle = Z_WAIT_Q_INIT(&_name.idle),                                    \
	.workers = _workers,                                                   \
	.num_workers = _num_workers,                                           \
	.stack_size = _stack_size,                                             \
	.name = #_name,                                                        \
}

#define Z_CALL_OFFLOADER_WORKER_INIT(_i, _name) { .stack = _name##_stacks[_i] }
/** @endcond */

/**
 * @brief Define an offloading engine and its worker threads.
 *
 * Defines the engine @p _name at file scope together with its worker objects and stacks. The
 * worker threads are created and parked on the idle queue before POST_KERNEL initialization
 * begins, so the engine can be used from any point after the kernel has started.
 *
 * @param _name Name of the engine.
 * @param _num_threads Number of worker threads.
 * @param _stack_size Stack size of each worker thread, in bytes.
 */
#define CALL_OFFLOADER_ENGINE_DEFINE(_name, _num_threads, _stack_size)                 \
	BUILD_ASSERT((_num_threads) > 0, "an offloading engine needs a worker");       \
	static K_KERNEL_STACK_ARRAY_DEFINE(_name##_stacks, _num_threads, _stack_size); \
	static struct call_offloader_worker _name##_workers[_num_threads] = {          \
		LISTIFY(_num_threads, Z_CALL_OFFLOADER_WORKER_INIT, (,), _name)        \
	};                                                                             \
	static STRUCT_SECTION_ITERABLE(call_offloader_engine, _name) =                 \
		CALL_OFFLOADER_ENGINE_INIT(_name, _name##_workers, _num_threads,       \
					   K_KERNEL_STACK_SIZEOF(_name##_stacks[0]))

/** @cond INTERNAL_HIDDEN */
#define Z_CALL_OFFLOADER_DROP_TYPE(...)

#define Z_CALL_OFFLOADER_VOID_void 1
#define Z_CALL_OFFLOADER_IS_VOID(_ret) \
	IS_ENABLED(Z_CALL_OFFLOADER_VOID_##_ret)

#define Z_CALL_OFFLOADER_PARAM(_p) __DEBRACKET _p
#define Z_CALL_OFFLOADER_FIELD(_p) __DEBRACKET _p;
#define Z_CALL_OFFLOADER_INIT(_p)                                              \
	.Z_CALL_OFFLOADER_DROP_TYPE _p = Z_CALL_OFFLOADER_DROP_TYPE _p
#define Z_CALL_OFFLOADER_ARG(_p) z_co->Z_CALL_OFFLOADER_DROP_TYPE _p

#define Z_CALL_OFFLOADER_PARAMS(...)                                           \
	COND_CODE_1(IS_EMPTY(__VA_ARGS__), (void),                             \
		    (FOR_EACH(Z_CALL_OFFLOADER_PARAM, (,), __VA_ARGS__)))
#define Z_CALL_OFFLOADER_FIELDS(...)                                           \
	COND_CODE_1(IS_EMPTY(__VA_ARGS__), (),                                 \
		    (FOR_EACH(Z_CALL_OFFLOADER_FIELD, (), __VA_ARGS__)))
#define Z_CALL_OFFLOADER_RET_FIELD(_ret, ...)                                            \
	COND_CODE_1(Z_CALL_OFFLOADER_IS_VOID(_ret),                                      \
		    (COND_CODE_1(IS_EMPTY(__VA_ARGS__), (char z_co_unused;), ())),       \
		    (_ret z_co_ret;))
#define Z_CALL_OFFLOADER_INITS(...)                                            \
	COND_CODE_1(IS_EMPTY(__VA_ARGS__), (),                                 \
		    (= { FOR_EACH(Z_CALL_OFFLOADER_INIT, (,), __VA_ARGS__) }))
#define Z_CALL_OFFLOADER_ARGS(...)                                             \
	COND_CODE_1(IS_EMPTY(__VA_ARGS__), (),                                 \
		    (FOR_EACH(Z_CALL_OFFLOADER_ARG, (,), __VA_ARGS__)))
#define Z_CALL_OFFLOADER_CALL(_ret, _fn, ...)                                  \
	COND_CODE_1(Z_CALL_OFFLOADER_IS_VOID(_ret), (), (z_co->z_co_ret =))    \
	z_call_offloader_##_fn##_offloaded(Z_CALL_OFFLOADER_ARGS(__VA_ARGS__))
/** @endcond */

/**
 * @brief Define a replacement for an existing function that runs on an offloading engine.
 *
 * Defines @c call_offloader_<_fn>() with the signature of @p _fn, together with the static
 * function carrying the body that follows the macro. Callers of the wrapper block while a worker
 * thread of @p _engine runs that body, so only the worker stacks have to accommodate it. The
 * wrapper is declared with the type of @p _fn, so a parameter list or return type that does not
 * match @p _fn is a compile error instead of a silent mismatch between the callers and the
 * wrapper.
 *
 * The body is written as an ordinary function body and refers to the parameters by name:
 *
 * @code
 * CALL_OFFLOADER_WRAP(my_engine, int, thing, (const uint8_t *) input, (size_t) input_len)
 * {
 *         return thing(input, input_len);
 * }
 * @endcode
 *
 * @param _engine Engine to run the body on.
 * @param _ret Return type of @p _fn.
 * @param _fn Existing function the wrapper replaces.
 * @param ... Parameters of @p _fn, each written as the type in parentheses followed by the name,
 *	  as in <tt>(const uint8_t *) input</tt>. Types whose declarator encloses the name, such as
 *	  function pointer and array types, need a typedef. At most 64 parameters.
 *
 * @note The arguments are copied onto the caller's stack for the duration of the call, so a
 *	 pointer parameter still refers to memory owned by the caller.
 * @note @p _fn must not be redirected to its wrapper where this macro is used, or the wrapper is
 *	 declared with its own type and nothing is checked.
 */
#define CALL_OFFLOADER_WRAP(_engine, _ret, _fn, ...)                                     \
	__typeof__(_fn) call_offloader_##_fn;                                            \
	struct z_call_offloader_##_fn##_args {                                           \
		Z_CALL_OFFLOADER_FIELDS(__VA_ARGS__)                                     \
		Z_CALL_OFFLOADER_RET_FIELD(_ret, __VA_ARGS__)                            \
	};                                                                               \
	static _ret z_call_offloader_##_fn##_offloaded(                                  \
		Z_CALL_OFFLOADER_PARAMS(__VA_ARGS__));                                   \
	static void z_call_offloader_##_fn##_work(void *args)                            \
	{                                                                                \
		struct z_call_offloader_##_fn##_args *z_co = args;                       \
		ARG_UNUSED(z_co);                                                        \
                                                                                         \
		Z_CALL_OFFLOADER_CALL(_ret, _fn, __VA_ARGS__);                           \
	}                                                                                \
	_ret call_offloader_##_fn(Z_CALL_OFFLOADER_PARAMS(__VA_ARGS__))                  \
	{                                                                                \
		struct z_call_offloader_##_fn##_args z_co                                \
			Z_CALL_OFFLOADER_INITS(__VA_ARGS__);                             \
                                                                                         \
		call_offloader_dispatch(&_engine, z_call_offloader_##_fn##_work, &z_co); \
                                                                                         \
		COND_CODE_1(Z_CALL_OFFLOADER_IS_VOID(_ret), (), (return z_co.z_co_ret;)) \
	}                                                                                \
	static _ret z_call_offloader_##_fn##_offloaded(Z_CALL_OFFLOADER_PARAMS(__VA_ARGS__))

/** @} */


#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_INCLUDE_CALL_OFFLOADER_CALL_OFFLOADER_H_ */
