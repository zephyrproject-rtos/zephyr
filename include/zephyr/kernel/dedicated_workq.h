/*
 * Copyright 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Macros for declaring and defining dedicated workqueues.
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_DEDICATED_WORKQ_H_
#define ZEPHYR_INCLUDE_KERNEL_DEDICATED_WORKQ_H_

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 */

#define Z_DEDICATED_WORK_QUEUE_DECLARE_DEDICATED(_prefix)                                          \
	int _prefix##_work_submit(struct k_work *work);                                            \
	int _prefix##_work_schedule(struct k_work_delayable *dwork, k_timeout_t delay);            \
	int _prefix##_work_reschedule(struct k_work_delayable *dwork, k_timeout_t delay)

#define Z_DEDICATED_WORK_QUEUE_DECLARE_SYSTEM(_prefix)                                             \
	static inline int _prefix##_work_submit(struct k_work *work)                               \
	{                                                                                          \
		return k_work_submit(work);                                                        \
	}                                                                                          \
	static inline int _prefix##_work_schedule(struct k_work_delayable *dwork,                  \
						  k_timeout_t delay)                               \
	{                                                                                          \
		return k_work_schedule(dwork, delay);                                              \
	}                                                                                          \
	static inline int _prefix##_work_reschedule(struct k_work_delayable *dwork,                \
						    k_timeout_t delay)                             \
	{                                                                                          \
		return k_work_reschedule(dwork, delay);                                            \
	}

#define Z_DEDICATED_WORK_QUEUE_DEFINE(_prefix, _module)                                            \
	static struct k_work_q _prefix##_work_q;                                                   \
	static K_KERNEL_STACK_DEFINE(_prefix##_work_q_stack,                                       \
				     CONFIG_##_module##_DEDICATED_WORKQUEUE_STACK_SIZE);           \
                                                                                                   \
	int _prefix##_work_submit(struct k_work *work)                                             \
	{                                                                                          \
		return k_work_submit_to_queue(&_prefix##_work_q, work);                            \
	}                                                                                          \
                                                                                                   \
	int _prefix##_work_schedule(struct k_work_delayable *dwork, k_timeout_t delay)             \
	{                                                                                          \
		return k_work_schedule_for_queue(&_prefix##_work_q, dwork, delay);                 \
	}                                                                                          \
                                                                                                   \
	int _prefix##_work_reschedule(struct k_work_delayable *dwork, k_timeout_t delay)           \
	{                                                                                          \
		return k_work_reschedule_for_queue(&_prefix##_work_q, dwork, delay);               \
	}                                                                                          \
                                                                                                   \
	static int _prefix##_work_q_init(void)                                                     \
	{                                                                                          \
		static const struct k_work_queue_config cfg = {                                    \
			.name = STRINGIFY(_prefix) "_workq",                                       \
			};                                                                         \
                                                                                                   \
		k_work_queue_init(&_prefix##_work_q);                                              \
		k_work_queue_start(&_prefix##_work_q, _prefix##_work_q_stack,                      \
				   K_KERNEL_STACK_SIZEOF(_prefix##_work_q_stack),                  \
				   CONFIG_##_module##_DEDICATED_WORKQUEUE_PRIORITY, &cfg);         \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	SYS_INIT(_prefix##_work_q_init, POST_KERNEL, 0)

/**
 * @endcond
 */

/**
 * @brief Declare workqueue helper functions for a subsystem.
 *
 * Declares `<_prefix>_work_submit()`, `<_prefix>_work_schedule()`, and
 * `<_prefix>_work_reschedule()`. When `CONFIG_<_module>_DEDICATED_WORKQUEUE` is
 * enabled, these are declared as external functions backed by a dedicated
 * workqueue; otherwise they are defined as `static inline` wrappers around
 * the system workqueue functions.
 *
 * @param _prefix Lowercase subsystem prefix used for function names.
 * @param _module Uppercase module prefix matching the Kconfig option
 *                `CONFIG_<_module>_DEDICATED_WORKQUEUE`.
 */
#define K_DEDICATED_WORK_QUEUE_DECLARE(_prefix, _module)			\
	COND_CODE_1(CONFIG_##_module##_DEDICATED_WORKQUEUE,			\
		    (Z_DEDICATED_WORK_QUEUE_DECLARE_DEDICATED(_prefix)),	\
		    (Z_DEDICATED_WORK_QUEUE_DECLARE_SYSTEM(_prefix)))

/**
 * @brief Define a dedicated workqueue and helper functions for a subsystem.
 *
 * When `CONFIG_<_module>_DEDICATED_WORKQUEUE` is enabled, defines the static
 * workqueue object, kernel stack sized by
 * `CONFIG_<_module>_DEDICATED_WORKQUEUE_STACK_SIZE`, `POST_KERNEL` startup
 * initialization at priority `CONFIG_<_module>_DEDICATED_WORKQUEUE_PRIORITY`,
 * and the `<_prefix>_work_submit()`, `<_prefix>_work_schedule()`, and
 * `<_prefix>_work_reschedule()` functions. Expands to nothing when
 * `CONFIG_<_module>_DEDICATED_WORKQUEUE` is disabled.
 *
 * @param _prefix Lowercase subsystem prefix used for function and thread names.
 * @param _module Uppercase module prefix matching the Kconfig options
 *                `CONFIG_<_module>_DEDICATED_WORKQUEUE*`.
 */
#define K_DEDICATED_WORK_QUEUE_DEFINE(_prefix, _module)				\
	IF_ENABLED(CONFIG_##_module##_DEDICATED_WORKQUEUE,			\
		   (Z_DEDICATED_WORK_QUEUE_DEFINE(_prefix, _module)))

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_KERNEL_DEDICATED_WORKQ_H_ */
