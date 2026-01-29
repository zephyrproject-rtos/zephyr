/**
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CLEANUP_H_
#define ZEPHYR_INCLUDE_CLEANUP_H_

#include <zephyr/toolchain.h>

/**
 * @file
 * @brief cleanup class API interface
 */

/**
 * @defgroup cleanup_interface Cleanup Class Interface
 * @ingroup os_services
 * @{
 */

#if defined(CONFIG_CLEANUP_CLASSES) || defined(__DOXYGEN__)

/**
 * @brief Define a cleanup class
 *
 * This macro defines a new cleanup class that can be used with the <tt>__cleanup</tt>
 * attribute to automatically execute cleanup code when a variable goes out of scope.
 *
 * @param _name Name of the cleanup class
 * @param _type Type of the variable to be cleaned up
 * @param _exit Cleanup code to execute when variable goes out of scope (can reference <tt>_T</tt>)
 * @param _init Initialization expression for the variable
 * @param ...   Initialization arguments (variadic)
 *
 * The macro creates:
 * - A destructor function <tt>class_&lt;name&gt;_destructor</tt> that executes cleanup code
 * - A constructor function <tt>class_&lt;name&gt;_constructor</tt> that initializes the variable
 * - A typedef <tt>class_&lt;name&gt;_t</tt> for the type
 *
 * @note The cleanup code can reference <tt>_T</tt> which contains the value of the variable
 *
 * @kconfig_dep{CONFIG_CLEANUP_CLASSES}
 *
 * Example:
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
 * // Define the cleanup class
 * DEFINE_CLASS(foo, struct foo *, foo_destructor(_T), foo_constructor(val), int val);
 *
 * static int some_function(void)
 * {
 *     // The CLASS macro declares 'f' with automatic cleanup
 *     CLASS(foo, f)(42);
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
#define DEFINE_CLASS(_name, _type, _exit, _init, ...)                                              \
	static inline void __maybe_unused class_##_name##_destructor(_type *p)                     \
	{                                                                                          \
		_type _T = *p;                                                                     \
		_exit;                                                                             \
	}                                                                                          \
	static inline _type __maybe_unused class_##_name##_constructor(__VA_ARGS__)                \
	{                                                                                          \
		_type t = _init;                                                                   \
		return t;                                                                          \
	}                                                                                          \
	typedef _type class_##_name##_t

/**
 * @brief Define a guard cleanup class
 *
 * This macro defines a guard that automatically acquires a lock on initialization
 * and releases it when going out of scope.
 *
 * @param _name Name of the guard (will be prefixed with guard_)
 * @param _type Type of the lock object (typically a pointer)
 * @param _lock Expression to acquire the lock (can reference <tt>_T</tt>)
 * @param _unlock Expression to release the lock (can reference <tt>_T</tt>)
 *
 * @kconfig_dep{CONFIG_CLEANUP_CLASSES}
 */
#define DEFINE_GUARD(_name, _type, _lock, _unlock)                                                 \
	DEFINE_CLASS(                                                                              \
		guard_##_name, _type, if (_T != NULL) { _unlock; }, ({                             \
			_lock;                                                                     \
			_T;                                                                        \
		}),                                                                                \
		_type _T)

/** @cond INTERNAL_HIDDEN */

#define Z_DEFINE_DEFER_VOID(_func)                                                                 \
	DEFINE_CLASS(defer_##_func, void *, (void)_T; (void)_func(), NULL, void)

#define Z_DEFER_CTX_ARG(idx, t) t arg##idx;
#define Z_DEFER_FUNC_ARG(idx, t) t arg##idx
#define Z_DEFER_INIT_ARG(idx, _) .arg##idx = arg##idx
#define Z_DEFER_EXIT_ARG(idx, _) _T.arg##idx

#define Z_DEFINE_DEFER_N(_func, ...)                                                               \
	struct defer_##_func##_ctx {                                                               \
		FOR_EACH_IDX(Z_DEFER_CTX_ARG, (), __VA_ARGS__)                                     \
	};                                                                                         \
	DEFINE_CLASS(defer_##_func, struct defer_##_func##_ctx,                                    \
		     (void)_func(FOR_EACH_IDX(Z_DEFER_EXIT_ARG, (,), __VA_ARGS__)),                \
		     {FOR_EACH_IDX(Z_DEFER_INIT_ARG, (,), __VA_ARGS__)},                           \
		     FOR_EACH_IDX(Z_DEFER_FUNC_ARG, (,), __VA_ARGS__))

/** @endcond */

/**
 * @brief Define a defer cleanup class
 *
 * This macro defines a defer that executes a cleanup function when the variable
 * goes out of scope. Unlike guards, defers don't acquire a resource on initialization.
 *
 * @param _func The function to call at cleanup
 * @param ...   The argument types to pass to the cleanup function
 *
 * @kconfig_dep{CONFIG_CLEANUP_CLASSES}
 */
#define DEFINE_DEFER(_func, ...)                                                                   \
	COND_CODE_0(NUM_VA_ARGS(__VA_ARGS__),                                                      \
		    (Z_DEFINE_DEFER_VOID(_func)),                                                  \
		    (Z_DEFINE_DEFER_N(_func, __VA_ARGS__)))

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Helper marcro to generate a unique identifier with prefix
 * @param _prefix Prefix for the unique identifier
 * @internal
 */
#define Z_UNIQUE_ID(_prefix) _CONCAT(_CONCAT(Z_UNIQUE_ID_, _prefix), __COUNTER__)

/** @endcond */

/**
 * @brief Declare a variable with automatic cleanup
 *
 * This macro declares a variable with automatic cleanup using a previously defined
 * cleanup class. The variable will be automatically cleaned up when it goes out of scope.
 *
 * The variable is initialized by calling the constructor function defined in @ref DEFINE_CLASS.
 * Use @ref CLASS_INIT if you want to initialize the variable with a direct expression instead
 * of calling the constructor function.
 *
 * @param _name Name of the cleanup class (defined by @ref DEFINE_CLASS)
 * @param _var Name of the variable to declare
 *
 * @kconfig_dep{CONFIG_CLEANUP_CLASSES}
 *
 * Usage: @code{.c} CLASS(my_class, my_var)(init_args); @endcode
 *
 * @see CLASS_INIT for direct initialization without calling the constructor
 */
#define CLASS(_name, _var)                                                                         \
	class_##_name##_t _var __cleanup(class_##_name##_destructor) = class_##_name##_constructor

/**
 * @brief Declare a variable with automatic cleanup using direct initialization
 *
 * This macro declares a variable with automatic cleanup using a previously defined
 * cleanup class. The variable will be automatically cleaned up when it goes out of scope.
 *
 * Unlike @ref CLASS, this macro accepts a direct initialization expression instead of
 * calling the constructor function defined in @ref DEFINE_CLASS. Use this when you need
 * to bypass the constructor and initialize the variable directly (e.g., with a struct
 * initializer, a different function, or a literal value).
 *
 * @param _name Name of the cleanup class (defined by @ref DEFINE_CLASS)
 * @param _var Name of the variable to declare
 * @param _init_expr Direct initialization expression for the variable (e.g., <tt>{0}</tt>,
 *                   <tt>NULL</tt>, or a function call)
 *
 * @kconfig_dep{CONFIG_CLEANUP_CLASSES}
 *
 * Usage: @code{.c} CLASS_INIT(my_class, my_var, {0}); @endcode
 *
 * @see CLASS for initialization using the constructor function
 */
#define CLASS_INIT(_name, _var, _init_expr)                                                        \
	class_##_name##_t _var __cleanup(class_##_name##_destructor) = (_init_expr)

/**
 * @brief Use a guard with automatic variable name
 *
 * This macro creates a guard variable with an automatically generated unique name.
 * The guard will acquire the lock on initialization and release it when going out of scope.
 *
 * @param _name Name of the guard type (defined by @ref DEFINE_GUARD or
 *              @ref DEFINE_CLASS with <tt>guard_</tt> prefix)
 *
 * @kconfig_dep{CONFIG_CLEANUP_CLASSES}
 *
 * Usage: @code{.c} GUARD(k_mutex)(&my_mutex); @endcode
 */
#define GUARD(_name) CLASS(guard_##_name, Z_UNIQUE_ID(guard))

/**
 * @brief Use a defer with automatic variable name
 *
 * This macro creates a defer variable with an automatically generated unique name.
 * The defer will execute its cleanup action when going out of scope.
 *
 * @param _name Name of the guard type (defined by @ref DEFINE_DEFER or
 *              @ref DEFINE_CLASS with <tt>defer_</tt> prefix)
 *
 * @kconfig_dep{CONFIG_CLEANUP_CLASSES}
 *
 * Usage: @code{.c} DEFER(k_mutex_unlock)(&my_mutex); @endcode
 */
#define DEFER(_name) CLASS(defer_##_name, Z_UNIQUE_ID(defer))

#endif /* defined(CONFIG_CLEANUP_CLASSES) || defined(__DOXYGEN__) */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_CLEANUP_H_ */
