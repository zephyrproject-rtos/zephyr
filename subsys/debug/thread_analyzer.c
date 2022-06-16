/*
 * Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief Thread analyzer implementation
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/debug/thread_analyzer.h>
#include <zephyr/debug/stack.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

LOG_MODULE_REGISTER(thread_analyzer, CONFIG_THREAD_ANALYZER_LOG_LEVEL);

#if IS_ENABLED(CONFIG_THREAD_ANALYZER_USE_PRINTK)
#define THREAD_ANALYZER_PRINT(...) printk(__VA_ARGS__)
#define THREAD_ANALYZER_FMT(str)   str "\n"
#define THREAD_ANALYZER_VSTR(str)  (str)
#else
#define THREAD_ANALYZER_PRINT(...) LOG_INF(__VA_ARGS__)
#define THREAD_ANALYZER_FMT(str)   str
#define THREAD_ANALYZER_VSTR(str)  log_strdup(str)
#endif

/* @brief Maximum length of the pointer when converted to string
 *
 * Pointer is converted to string in hexadecimal form.
 * It would use 2 hex digits for every single byte of the pointer
 * but some implementations adds 0x prefix when used with %p format option.
 */
#define PTR_STR_MAXLEN (sizeof(void *) * 2 + 2)

static void thread_print_cb(struct thread_analyzer_info *info)
{
	size_t pcnt = (info->stack_used * 100U) / info->stack_size;
#ifdef CONFIG_THREAD_RUNTIME_STATS
	THREAD_ANALYZER_PRINT(
		THREAD_ANALYZER_FMT(
			" %-20s: STACK: unused %zu usage %zu / %zu (%zu %%); CPU: %u %%"),
		THREAD_ANALYZER_VSTR(info->name),
		info->stack_size - info->stack_used, info->stack_used,
		info->stack_size, pcnt,
		info->utilization);

#ifdef CONFIG_SCHED_THREAD_USAGE
	THREAD_ANALYZER_PRINT(
		THREAD_ANALYZER_FMT("      : Total CPU cycles used: %llu"),
		info->usage.total_cycles);

#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
	THREAD_ANALYZER_PRINT(
		THREAD_ANALYZER_FMT(
			"         - Current Frame: %llu; Longest Frame: %llu; Average Frame: %llu"),
		info->usage.current_cycles, info->usage.peak_cycles,
		info->usage.average_cycles);
#endif
#endif
#else
	THREAD_ANALYZER_PRINT(
		THREAD_ANALYZER_FMT(
			" %-20s: unused %zu usage %zu / %zu (%zu %%)"),
		THREAD_ANALYZER_VSTR(info->name),
		info->stack_size - info->stack_used, info->stack_used,
		info->stack_size, pcnt);
#endif
}

static void thread_analyze_cb(const struct k_thread *cthread, void *user_data)
{
	struct k_thread *thread = (struct k_thread *)cthread;
#ifdef CONFIG_THREAD_RUNTIME_STATS
	k_thread_runtime_stats_t rt_stats_all;
	int ret;
#endif
	size_t size = thread->stack_info.size;
	thread_analyzer_cb cb = user_data;
	struct thread_analyzer_info info;
	char hexname[PTR_STR_MAXLEN + 1];
	const char *name;
	size_t unused;
	int err;



	name = k_thread_name_get((k_tid_t)thread);
	if (!name || name[0] == '\0') {
		name = hexname;
		snprintk(hexname, sizeof(hexname), "%p", (void *)thread);
	}

	err = k_thread_stack_space_get(thread, &unused);
	if (err) {
		THREAD_ANALYZER_PRINT(
			THREAD_ANALYZER_FMT(
				" %-20s: unable to get stack space (%d)"),
			name, err);

		unused = 0;
	}

	info.name = name;
	info.stack_size = size;
	info.stack_used = size - unused;

#ifdef CONFIG_THREAD_RUNTIME_STATS
	ret = 0;

	if (k_thread_runtime_stats_get(thread, &info.usage) != 0) {
		ret++;
	}

	if (k_thread_runtime_stats_all_get(&rt_stats_all) != 0) {
		ret++;
	}
	if (ret == 0) {
		info.utilization = (info.usage.execution_cycles * 100U) /
			rt_stats_all.execution_cycles;
	}
#endif
	cb(&info);
}

K_KERNEL_STACK_ARRAY_DECLARE(z_interrupt_stacks, CONFIG_MP_NUM_CPUS,
			     CONFIG_ISR_STACK_SIZE);

static void isr_stacks(void)
{
	for (int i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		const uint8_t *buf = Z_KERNEL_STACK_BUFFER(z_interrupt_stacks[i]);
		size_t size = K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[i]);
		size_t unused;
		int err;

		err = z_stack_space_get(buf, size, &unused);
		if (err == 0) {
			THREAD_ANALYZER_PRINT(
				THREAD_ANALYZER_FMT(
					" %s%-17d: STACK: unused %zu usage %zu / %zu (%zu %%)"),
					THREAD_ANALYZER_VSTR("ISR"), i, unused,
					size - unused, size, (100 * (size - unused)) / size);
		}
	}
}

void thread_analyzer_run(thread_analyzer_cb cb)
{
	if (IS_ENABLED(CONFIG_THREAD_ANALYZER_RUN_UNLOCKED)) {
		k_thread_foreach_unlocked(thread_analyze_cb, cb);
	} else {
		k_thread_foreach(thread_analyze_cb, cb);
	}

	if (IS_ENABLED(CONFIG_THREAD_ANALYZER_ISR_STACK_USAGE)) {
		isr_stacks();
	}
}

void thread_analyzer_print(void)
{
	THREAD_ANALYZER_PRINT(THREAD_ANALYZER_FMT("Thread analyze:"));
	thread_analyzer_run(thread_print_cb);
}

#if IS_ENABLED(CONFIG_THREAD_ANALYZER_AUTO)

void thread_analyzer_auto(void)
{
	for (;;) {
		thread_analyzer_print();
		k_sleep(K_SECONDS(CONFIG_THREAD_ANALYZER_AUTO_INTERVAL));
	}
}

K_THREAD_DEFINE(thread_analyzer,
		CONFIG_THREAD_ANALYZER_AUTO_STACK_SIZE,
		thread_analyzer_auto,
		NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO,
		0, 0);

#endif
