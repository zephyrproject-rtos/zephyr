/*
 * Copyright 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Macros for declaring and defining subsystem-specific workqueues.
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_SUBSYSTEM_WORKQ_H_
#define ZEPHYR_INCLUDE_KERNEL_SUBSYSTEM_WORKQ_H_

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 */

#define Z_SUBSYSTEM_WORK_QUEUE_DECLARE_DEDICATED(subsys)                                           \
	int subsys##_work_submit(struct k_work *work);                                             \
	int subsys##_work_schedule(struct k_work_delayable *dwork, k_timeout_t delay);             \
	int subsys##_work_reschedule(struct k_work_delayable *dwork, k_timeout_t delay)

#define Z_SUBSYSTEM_WORK_QUEUE_DECLARE_SYSTEM(subsys)                                              \
	static inline int subsys##_work_submit(struct k_work *work)                                \
	{                                                                                          \
		return k_work_submit(work);                                                        \
	}                                                                                          \
	static inline int subsys##_work_schedule(struct k_work_delayable *dwork,                   \
						 k_timeout_t delay)                                \
	{                                                                                          \
		return k_work_schedule(dwork, delay);                                              \
	}                                                                                          \
	static inline int subsys##_work_reschedule(struct k_work_delayable *dwork,                 \
						   k_timeout_t delay)                              \
	{                                                                                          \
		return k_work_reschedule(dwork, delay);                                            \
	}

#define Z_SUBSYSTEM_WORK_QUEUE_DEFINE(subsys, module)                                              \
	static struct k_work_q subsys##_work_q;                                                    \
	static K_KERNEL_STACK_DEFINE(subsys##_work_q_stack,                                        \
				     CONFIG_##module##_DEDICATED_WORKQUEUE_STACK_SIZE);            \
                                                                                                   \
	int subsys##_work_submit(struct k_work *work)                                              \
	{                                                                                          \
		return k_work_submit_to_queue(&subsys##_work_q, work);                             \
	}                                                                                          \
                                                                                                   \
	int subsys##_work_schedule(struct k_work_delayable *dwork, k_timeout_t delay)              \
	{                                                                                          \
		return k_work_schedule_for_queue(&subsys##_work_q, dwork, delay);                  \
	}                                                                                          \
                                                                                                   \
	int subsys##_work_reschedule(struct k_work_delayable *dwork, k_timeout_t delay)            \
	{                                                                                          \
		return k_work_reschedule_for_queue(&subsys##_work_q, dwork, delay);                \
	}                                                                                          \
                                                                                                   \
	static int subsys##_work_q_init(void)                                                      \
	{                                                                                          \
		static const struct k_work_queue_config cfg = {                                    \
			.name = STRINGIFY(subsys) "_workq",                                        \
			};                                                                         \
                                                                                                   \
		k_work_queue_init(&subsys##_work_q);                                               \
		k_work_queue_start(&subsys##_work_q, subsys##_work_q_stack,                        \
				   K_KERNEL_STACK_SIZEOF(subsys##_work_q_stack),                   \
				   CONFIG_##module##_DEDICATED_WORKQUEUE_PRIORITY, &cfg);          \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	SYS_INIT(subsys##_work_q_init, POST_KERNEL, 0)

/**
 * @endcond
 */

/**
 * @brief Declare workqueue helper functions for a subsystem.
 *
 * Declares `<subsys>_work_submit()`, `<subsys>_work_schedule()`, and
 * `<subsys>_work_reschedule()`. When `CONFIG_<module>_DEDICATED_WORKQUEUE` is
 * enabled, these are declared as external functions backed by a dedicated
 * workqueue; otherwise they are defined as `static inline` wrappers around
 * the system workqueue functions.
 *
 * @param subsys Lowercase subsystem prefix used for function names.
 * @param module Uppercase module prefix matching the Kconfig option
 *               `CONFIG_<module>_DEDICATED_WORKQUEUE`.
 */
#define K_SUBSYSTEM_WORK_QUEUE_DECLARE(subsys, module)				\
	COND_CODE_1(CONFIG_##module##_DEDICATED_WORKQUEUE,			\
		    (Z_SUBSYSTEM_WORK_QUEUE_DECLARE_DEDICATED(subsys)),		\
		    (Z_SUBSYSTEM_WORK_QUEUE_DECLARE_SYSTEM(subsys)))

/**
 * @brief Define a dedicated workqueue and helper functions for a subsystem.
 *
 * When `CONFIG_<module>_DEDICATED_WORKQUEUE` is enabled, defines the static
 * workqueue object, kernel stack sized by
 * `CONFIG_<module>_DEDICATED_WORKQUEUE_STACK_SIZE`, `POST_KERNEL` startup
 * initialization at priority `CONFIG_<module>_DEDICATED_WORKQUEUE_PRIORITY`,
 * and the `<subsys>_work_submit()`, `<subsys>_work_schedule()`, and
 * `<subsys>_work_reschedule()` functions. Expands to nothing when
 * `CONFIG_<module>_DEDICATED_WORKQUEUE` is disabled.
 *
 * @param subsys Lowercase subsystem prefix used for function and thread names.
 * @param module Uppercase module prefix matching the Kconfig options
 *               `CONFIG_<module>_DEDICATED_WORKQUEUE*`.
 */
#define K_SUBSYSTEM_WORK_QUEUE_DEFINE(subsys, module)				\
	IF_ENABLED(CONFIG_##module##_DEDICATED_WORKQUEUE,			\
		   (Z_SUBSYSTEM_WORK_QUEUE_DEFINE(subsys, module)))

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_KERNEL_SUBSYSTEM_WORKQ_H_ */
