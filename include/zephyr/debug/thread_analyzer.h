/*
 * Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __STACK_SIZE_ANALYZER_H
#define __STACK_SIZE_ANALYZER_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup thread_analyzer Thread analyzer
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
};

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
 *  a given callback on every thread found.
 *
 *  @param cb The callback function handler
 */
void thread_analyzer_run(thread_analyzer_cb cb);

/** @brief Run the thread analyzer and print stack size statistics.
 *
 *  This function runs the thread analyzer and prints the output in standard
 *  form.
 */
void thread_analyzer_print(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __STACK_SIZE_ANALYZER_H */
