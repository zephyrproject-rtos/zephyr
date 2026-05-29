/**
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CLEANUP_H_
#define ZEPHYR_INCLUDE_CLEANUP_H_

#include <zephyr/toolchain.h>

/**
 * @file
 * @brief Cleanup helper API interface
 */

/**
 * @defgroup cleanup_interface Cleanup Helper Interface
 * @ingroup os_services
 * @{
 */

#if defined(CONFIG_SCOPE_CLEANUP_HELPERS) || defined(__DOXYGEN__)

/**
 * @brief Define a scoped variable type
 *
 * This macro defines a new cleanup helper that can be used with the <tt>__cleanup</tt>
 * attribute to automatically execute cleanup code when a variable goes out of scope.
 *
 * @param _name Name of the cleanup helper
 * @param _type Type of the variable to be cleaned up
 * @param _exit_fn Cleanup code to execute when variable goes out of scope
 *                 (can reference <tt>_T</tt>)
 * @param _init_fn Initialization expression for the variable
 * @param ...   Initialization arguments (variadic)
 *
 * The macro creates:
 * - An exit function <tt>cleanup_&lt;name&gt;_exit</tt> that executes cleanup code
 * - An init function <tt>cleanup_&lt;name&gt;_init</tt> that initializes the variable
 * - A typedef <tt>cleanup_&lt;name&gt;_t</tt> for the type
 *
 * @kconfig_dep{CONFIG_SCOPE_CLEANUP_HELPERS}
 *
 * Usage:
 * @code{.c}
 * struct foo {
 *     int value;
 * };
 *
 * static inline struct foo *foo_constructor(int value)
 * {
 *     struct foo *ptr = k_malloc(sizeof(struct foo));
 *
 *     if (ptr != NULL) {
 *         ptr->value = value;
 *     }
 *
 *     return ptr;
 * }
 *
 * static inline void foo_destructor(struct foo *ptr)
 * {
 *     k_free(ptr);
 * }
 *
 * // Define the cleanup helper
 * SCOPE_VAR_DEFINE(foo, struct foo *, foo_destructor(_T), foo_constructor(val), int val);
 *
 * static int some_function(void)
 * {
 *     // The scope_var macro declares 'f' with automatic cleanup
 *     scope_var(foo, f)(42);
 *     if (f == NULL) {
 *         return -ENOMEM;
 *     }
 *
 *     // Use f normally - it points to an allocated and initialized struct foo
 *     printk("Value: %d\n", f->value);
 *
 *     // No need to manually free - foo_destructor is called automatically
 *     // when 'f' goes out of scope
 * }
 * @endcode
 */
#define SCOPE_VAR_DEFINE(_name, _type, _exit_fn, _init_fn, ...)                                    \
	static inline void __maybe_unused cleanup_##_name##_exit(_type *p)                         \
	{                                                                                          \
		_type _T = *p;                                                                     \
		_exit_fn;                                                                          \
	}                                                                                          \
	static inline _type __maybe_unused cleanup_##_name##_init(__VA_ARGS__)                     \
	{                                                                                          \
		_type t = _init_fn;                                                                \
		return t;                                                                          \
	}                                                                                          \
	typedef _type cleanup_##_name##_t

/**
 * @brief Define a scoped guard type
 *
 * This macro defines a guard that automatically acquires a lock on initialization
 * and releases it when going out of scope.
 *
 * @param _name Name of the guard (will be prefixed with guard_)
 * @param _type Type of the lock object (typically a pointer)
 * @param _lock Expression to acquire the lock (can reference <tt>_T</tt>)
 * @param _unlock Expression to release the lock (can reference <tt>_T</tt>)
 *
 * @kconfig_dep{CONFIG_SCOPE_CLEANUP_HELPERS}
 *
 * Usage:
 * @code{.c}
 * SCOPE_GUARD_DEFINE(k_mutex, struct k_mutex *,
 *                    (void)k_mutex_lock(_T, K_FOREVER),
 *                    (void)k_mutex_unlock(_T));
 * @endcode
 */
#define SCOPE_GUARD_DEFINE(_name, _type, _lock, _unlock)                                           \
	SCOPE_VAR_DEFINE(                                                                          \
		guard_##_name, _type, if (_T != NULL) { _unlock; }, ({                             \
			_lock;                                                                     \
			_T;                                                                        \
		}),                                                                                \
		_type _T)

/** @cond INTERNAL_HIDDEN */

#define Z_SCOPE_DEFER_DEFINE_VOID(_func)                                                           \
	SCOPE_VAR_DEFINE(defer_##_func, void *, (void)_T; (void)_func(), NULL, void)

#define Z_DEFER_CTX_ARG(idx, t) t arg##idx;
#define Z_DEFER_FUNC_ARG(idx, t) t arg##idx
#define Z_DEFER_INIT_ARG(idx, _) .arg##idx = arg##idx
#define Z_DEFER_EXIT_ARG(idx, _) _T.arg##idx
#define Z_COMMA ,

#define Z_SCOPE_DEFER_DEFINE_N(_func, ...)                                                         \
	struct defer_##_func##_ctx {                                                               \
		FOR_EACH_IDX(Z_DEFER_CTX_ARG, (), __VA_ARGS__)                                     \
	};                                                                                         \
	SCOPE_VAR_DEFINE(defer_##_func, struct defer_##_func##_ctx,                                \
		     (void)_func(FOR_EACH_IDX(Z_DEFER_EXIT_ARG, (Z_COMMA), __VA_ARGS__)),          \
		     {FOR_EACH_IDX(Z_DEFER_INIT_ARG, (Z_COMMA), __VA_ARGS__)},                     \
		     FOR_EACH_IDX(Z_DEFER_FUNC_ARG, (Z_COMMA), __VA_ARGS__))

/** @endcond */

/**
 * @brief Define a scoped defer type
 *
 * This macro defines a defer that executes a cleanup function when the variable
 * goes out of scope. Unlike guards, defers don't acquire a resource on initialization.
 *
 * @param _func The function to call at cleanup
 * @param ...   The argument types to pass to the cleanup function
 *
 * @kconfig_dep{CONFIG_SCOPE_CLEANUP_HELPERS}
 *
 * Usage:
 * @code{.c}
 * SCOPE_DEFER_DEFINE(k_free, void *);
 * @endcode
 */
#define SCOPE_DEFER_DEFINE(_func, ...)                                                             \
	COND_CODE_0(NUM_VA_ARGS(__VA_ARGS__),                                                      \
		    (Z_SCOPE_DEFER_DEFINE_VOID(_func)),                                            \
		    (Z_SCOPE_DEFER_DEFINE_N(_func, __VA_ARGS__)))

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Helper macro to generate a unique identifier with prefix
 * @param _prefix Prefix for the unique identifier
 * @internal
 */
#define Z_UNIQUE_ID(_prefix) _CONCAT(_CONCAT(Z_UNIQUE_ID_, _prefix), __COUNTER__)

/** @endcond */

/**
 * @brief Declare a variable with automatic cleanup
 *
 * This macro declares a variable with automatic cleanup using a previously defined
 * cleanup helper. The variable will be automatically cleaned up when it goes out of scope.
 *
 * The variable is initialized by calling the init function defined in @ref SCOPE_VAR_DEFINE.
 * Use @ref scope_var_init if you want to initialize the variable with a direct expression instead
 * of calling the init function.
 *
 * @param _name Name of the cleanup helper (defined by @ref SCOPE_VAR_DEFINE)
 * @param _var Name of the variable to declare
 *
 * @kconfig_dep{CONFIG_SCOPE_CLEANUP_HELPERS}
 *
 * Usage: @code{.c} scope_var(some_type, my_var)(init_args); @endcode
 *
 * @see scope_var_init for direct initialization without calling the init function
 */
#define scope_var(_name, _var)                                                                     \
	cleanup_##_name##_t _var __cleanup(cleanup_##_name##_exit) = cleanup_##_name##_init

/**
 * @brief Declare a variable with automatic cleanup using direct initialization
 *
 * This macro declares a variable with automatic cleanup using a previously defined
 * cleanup helper. The variable will be automatically cleaned up when it goes out of scope.
 *
 * Unlike @ref scope_var, this macro accepts a direct initialization expression instead of
 * calling the init function defined in @ref SCOPE_VAR_DEFINE. Use this when you need
 * to bypass the init function and initialize the variable directly (e.g., with a struct
 * initializer, a different function, or a literal value).
 *
 * @param _name Name of the cleanup helper (defined by @ref SCOPE_VAR_DEFINE)
 * @param _var Name of the variable to declare
 * @param _init_expr Direct initialization expression for the variable (e.g., <tt>{0}</tt>,
 *                   <tt>NULL</tt>, or a function call)
 *
 * @kconfig_dep{CONFIG_SCOPE_CLEANUP_HELPERS}
 *
 * Usage: @code{.c} scope_var_init(some_type, my_var, {0}); @endcode
 *
 * @see scope_var for initialization using the init function
 */
#define scope_var_init(_name, _var, _init_expr)                                                    \
	cleanup_##_name##_t _var __cleanup(cleanup_##_name##_exit) = (_init_expr)

/**
 * @brief Acquire a scoped guard lock
 *
 * This macro creates a guard variable with an automatically generated unique name.
 * The guard will acquire the lock on initialization and release it when going out of scope.
 *
 * @param _name Name of the guard type (defined by @ref SCOPE_GUARD_DEFINE or
 *              @ref SCOPE_VAR_DEFINE with <tt>guard_</tt> prefix)
 *
 * @kconfig_dep{CONFIG_SCOPE_CLEANUP_HELPERS}
 *
 * Usage: @code{.c} scope_guard(k_mutex)(&my_mutex); @endcode
 */
#define scope_guard(_name) scope_var(guard_##_name, Z_UNIQUE_ID(guard))

/**
 * @brief Register a scoped deferred call
 *
 * This macro creates a defer variable with an automatically generated unique name.
 * The defer will execute its cleanup action when going out of scope.
 *
 * @param _name Name of the defer type (defined by @ref SCOPE_DEFER_DEFINE or
 *              @ref SCOPE_VAR_DEFINE with <tt>defer_</tt> prefix)
 *
 * @kconfig_dep{CONFIG_SCOPE_CLEANUP_HELPERS}
 *
 * Usage: @code{.c} scope_defer(k_mutex_unlock)(&my_mutex); @endcode
 */
#define scope_defer(_name) scope_var(defer_##_name, Z_UNIQUE_ID(defer))

/**
 * @}
 */

#endif /* defined(CONFIG_SCOPE_CLEANUP_HELPERS) || defined(__DOXYGEN__) */

#endif /* ZEPHYR_INCLUDE_CLEANUP_H_ */
