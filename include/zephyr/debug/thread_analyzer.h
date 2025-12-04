/*
 * Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __STACK_SIZE_ANALYZER_H
#define __STACK_SIZE_ANALYZER_H

#include <stddef.h>
#include <zephyr/kernel/thread.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup thread_analyzer Thread analyzer
 *  @ingroup debug
 *  @brief Module for analyzing threads
 *
 *  This module implements functions and the configuration that simplifies
 *  thread analysis.
 *  @{
 */

struct thread_analyzer_info {
	/** The name of the thread or stringified address of the thread handle
	 * if name is not set.
	 */
	const char *name;
	/** The total size of the stack*/
	size_t stack_size;
	/** Stack size in used */
	size_t stack_used;

#ifdef CONFIG_THREAD_RUNTIME_STATS
	unsigned int utilization;
#ifdef CONFIG_SCHED_THREAD_USAGE
	k_thread_runtime_stats_t  usage;
#endif
#endif

#ifdef CONFIG_THREAD_ANALYZER_STACK_SAFETY
	uint32_t stack_safety;
#endif

#ifdef CONFIG_THREAD_ANALYZER_PRIV_STACK_USAGE
	/** Total size of privileged stack */
	size_t priv_stack_size;

	/** Privileged stack size in used */
	size_t priv_stack_used;
#endif
};

/** Stack safety issue codes */

/* No stack safety issues detected */
#define THREAD_ANALYZE_STACK_SAFETY_NO_ISSUES 0

/* Unused stack space is below the defined threshold */
#define THREAD_ANALYZE_STACK_SAFETY_THRESHOLD_EXCEEDED 1

/* No unused stack space is left */
#define THREAD_ANALYZE_STACK_SAFETY_AT_LIMIT 2

/* Stack overflow detected */
#define THREAD_ANALYZE_STACK_SAFETY_OVERFLOW 3

/** @brief Thread analyzer stack safety callback function
 *
 *  Stack safety callback function.
 *
 *  @param thread Pointer to the thread being analyzed.
 *  @param unused_space Amount of unused stack space.
 *  @param stack_issue Pointer to variable to store stack safety issue code
 */
typedef void (*thread_analyzer_stack_safety_handler)(struct k_thread *thread,
						     size_t unused_space,
						     uint32_t *stack_issue);

/** @brief Change the thread analyzer stack safety callback function
 *
 *  This function changes the thread analyzer's stack safety handler. This
 *  allows an application to customize behavior when a thread's unused stack
 *  drops below its configured threshold.
 *
 *  @param handler Function pointer to the new handler (NULL for default)
 */
void thread_analyzer_stack_safety_handler_set(thread_analyzer_stack_safety_handler handler);



/** @brief Thread analyzer stack size callback function
 *
 *  Callback function with thread analysis information.
 *
 *  @param info Thread analysis information.
 */
typedef void (*thread_analyzer_cb)(struct thread_analyzer_info *info);

/** @brief Run the thread analyzer and provide information to the callback
 *
 *  This function analyzes the current state for all threads and calls
 *  a given callback on every thread found. In the special case when Kconfig
 *  option THREAD_ANALYZER_AUTO_SEPARATE_CORES is set, the function analyzes
 *  only the threads running on the specified cpu.
 *
 *  @param cb The callback function handler
 *  @param cpu cpu to analyze, ignored if THREAD_ANALYZER_AUTO_SEPARATE_CORES=n
 */
void thread_analyzer_run(thread_analyzer_cb cb, unsigned int cpu);

/** @brief Run the thread analyzer and print stack size statistics.
 *
 *  This function runs the thread analyzer and prints the output in
 *  standard form. In the special case when Kconfig option
 *  THREAD_ANALYZER_AUTO_SEPARATE_CORES is set, the function analyzes
 *  only the threads running on the specified cpu.
 *
 *  @param cpu cpu to analyze, ignored if THREAD_ANALYZER_AUTO_SEPARATE_CORES=n
 */
void thread_analyzer_print(unsigned int cpu);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __STACK_SIZE_ANALYZER_H */
