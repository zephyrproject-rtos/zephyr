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

#if defined(CONFIG_THREAD_ANALYZER_USE_PRINTK)
#define THREAD_ANALYZER_PRINT(...) printk(__VA_ARGS__)
#define THREAD_ANALYZER_FMT(str)   str "\n"
#define THREAD_ANALYZER_VSTR(str)  (str)
#else
#define THREAD_ANALYZER_PRINT(...) LOG_INF(__VA_ARGS__)
#define THREAD_ANALYZER_FMT(str)   str
#define THREAD_ANALYZER_VSTR(str)  str
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

#ifdef CONFIG_THREAD_ANALYZER_PRIV_STACK_USAGE
	if (info->priv_stack_size > 0) {
		pcnt = (info->priv_stack_used * 100U) / info->priv_stack_size;

		THREAD_ANALYZER_PRINT(
			THREAD_ANALYZER_FMT(
				" %-20s: PRIV_STACK: unused %zu usage %zu / %zu (%zu %%)"),
			" ", info->priv_stack_size - info->priv_stack_used, info->priv_stack_used,
			info->priv_stack_size, pcnt);
	}
#endif

#ifdef CONFIG_SCHED_THREAD_USAGE
	THREAD_ANALYZER_PRINT(
		THREAD_ANALYZER_FMT(" %-20s: Total CPU cycles used: %llu"),
		" ", info->usage.total_cycles);

#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
	THREAD_ANALYZER_PRINT(
		THREAD_ANALYZER_FMT(
			" %-20s: Current Frame: %llu;"
			" Longest Frame: %llu; Average Frame: %llu"),
		" ", info->usage.current_cycles, info->usage.peak_cycles,
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

struct ta_cb_user_data {
	thread_analyzer_cb cb;
	unsigned int cpu;
};

static void thread_analyze_cb(const struct k_thread *cthread, void *user_data)
{
	struct k_thread *thread = (struct k_thread *)cthread;
#ifdef CONFIG_THREAD_RUNTIME_STATS
	k_thread_runtime_stats_t rt_stats_all;
#endif
	size_t size = thread->stack_info.size;
	struct ta_cb_user_data *ud = user_data;
	thread_analyzer_cb cb = ud->cb;
	unsigned int cpu = ud->cpu;
	struct thread_analyzer_info info;
	char hexname[PTR_STR_MAXLEN + 1];
	const char *name;
	size_t unused;
	int err;
	int ret;



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

#ifdef CONFIG_THREAD_ANALYZER_PRIV_STACK_USAGE
	ret = arch_thread_priv_stack_space_get(cthread, &size, &unused);
	if (ret == 0) {
		info.priv_stack_size = size;
		info.priv_stack_used = size - unused;
	} else {
		info.priv_stack_size = 0;
	}
#endif

#ifdef CONFIG_THREAD_RUNTIME_STATS
	ret = 0;

	if (k_thread_runtime_stats_get(thread, &info.usage) != 0) {
		ret++;
	}

	if (IS_ENABLED(CONFIG_THREAD_ANALYZER_AUTO_SEPARATE_CORES)) {
		if (k_thread_runtime_stats_cpu_get(cpu, &rt_stats_all) != 0) {
			ret++;
		}
	} else {
		if (k_thread_runtime_stats_all_get(&rt_stats_all) != 0) {
			ret++;
		}
	}

	if (ret == 0) {
		info.utilization = (info.usage.execution_cycles * 100U) /
			rt_stats_all.execution_cycles;
	}
#endif

	ARG_UNUSED(ret);

	cb(&info);
}

K_KERNEL_STACK_ARRAY_DECLARE(z_interrupt_stacks, CONFIG_MP_MAX_NUM_CPUS,
			     CONFIG_ISR_STACK_SIZE);

static void isr_stack(int core)
{
	const uint8_t *buf = K_KERNEL_STACK_BUFFER(z_interrupt_stacks[core]);
	size_t size = K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[core]);
	size_t unused;
	int err;

	err = z_stack_space_get(buf, size, &unused);
	if (err == 0) {
		THREAD_ANALYZER_PRINT(
			THREAD_ANALYZER_FMT(
				" %s%-17d: STACK: unused %zu usage %zu / %zu (%zu %%)"),
			THREAD_ANALYZER_VSTR("ISR"), core, unused,
			size - unused, size, (100 * (size - unused)) / size);
	}
}

static void isr_stacks(void)
{
	unsigned int num_cpus = arch_num_cpus();

	for (int i = 0; i < num_cpus; i++) {
		isr_stack(i);
	}
}

void thread_analyzer_run(thread_analyzer_cb cb, unsigned int cpu)
{
	struct ta_cb_user_data ud = { .cb = cb, .cpu = cpu };

	if (IS_ENABLED(CONFIG_THREAD_ANALYZER_RUN_UNLOCKED)) {
		if (IS_ENABLED(CONFIG_THREAD_ANALYZER_AUTO_SEPARATE_CORES)) {
			k_thread_foreach_unlocked_filter_by_cpu(cpu, thread_analyze_cb, &ud);
		} else {
			k_thread_foreach_unlocked(thread_analyze_cb, &ud);
		}
	} else {
		if (IS_ENABLED(CONFIG_THREAD_ANALYZER_AUTO_SEPARATE_CORES)) {
			k_thread_foreach_filter_by_cpu(cpu, thread_analyze_cb, &ud);
		} else {
			k_thread_foreach(thread_analyze_cb, &ud);
		}
	}

	if (IS_ENABLED(CONFIG_THREAD_ANALYZER_ISR_STACK_USAGE)) {
		if (IS_ENABLED(CONFIG_THREAD_ANALYZER_AUTO_SEPARATE_CORES)) {
			isr_stack(cpu);
		} else {
			isr_stacks();
		}
	}
}

void thread_analyzer_print(unsigned int cpu)
{
#if IS_ENABLED(CONFIG_THREAD_ANALYZER_AUTO_SEPARATE_CORES)
	THREAD_ANALYZER_PRINT(THREAD_ANALYZER_FMT("Thread analyze core %u:"),
						  cpu);
#else
	THREAD_ANALYZER_PRINT(THREAD_ANALYZER_FMT("Thread analyze:"));
#endif
	thread_analyzer_run(thread_print_cb, cpu);
}

#if defined(CONFIG_THREAD_ANALYZER_AUTO)

void thread_analyzer_auto(void *a, void *b, void *c)
{
	unsigned int cpu = IS_ENABLED(CONFIG_THREAD_ANALYZER_AUTO_SEPARATE_CORES) ?
		(unsigned int)(uintptr_t) a : 0;

	for (;;) {
		thread_analyzer_print(cpu);
		k_sleep(K_SECONDS(CONFIG_THREAD_ANALYZER_AUTO_INTERVAL));
	}
}

#if IS_ENABLED(CONFIG_THREAD_ANALYZER_AUTO_SEPARATE_CORES)

static K_THREAD_STACK_ARRAY_DEFINE(analyzer_thread_stacks, CONFIG_MP_MAX_NUM_CPUS,
				   CONFIG_THREAD_ANALYZER_AUTO_STACK_SIZE);
static struct k_thread analyzer_thread[CONFIG_MP_MAX_NUM_CPUS];

static int thread_analyzer_init(void)
{
	uint16_t i;

	for (i = 0; i < ARRAY_SIZE(analyzer_thread); i++) {
		char name[24];
		k_tid_t tid = NULL;
		int ret;

		tid = k_thread_create(&analyzer_thread[i], analyzer_thread_stacks[i],
				      CONFIG_THREAD_ANALYZER_AUTO_STACK_SIZE,
				      thread_analyzer_auto,
				      (void *) (uint32_t) i, NULL, NULL,
				      K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_FOREVER);
		if (!tid) {
			LOG_ERR("k_thread_create() failed for core %u", i);
			continue;
		}
		ret = k_thread_cpu_pin(tid, i);
		if (ret < 0) {
			LOG_ERR("Pinning thread to code core %u", i);
			k_thread_abort(tid);
			continue;
		}
		snprintf(name, sizeof(name), "core %u thread analyzer", i);
		ret = k_thread_name_set(tid, name);
		if (ret < 0) {
			LOG_INF("k_thread_name_set failed: %d for %u", ret, i);
		}

		k_thread_start(tid);
		LOG_DBG("Thread %p for core %u started", tid, i);
	}

	return 0;
}

SYS_INIT(thread_analyzer_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#else

K_THREAD_DEFINE(thread_analyzer,
		CONFIG_THREAD_ANALYZER_AUTO_STACK_SIZE,
		thread_analyzer_auto,
		NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO,
		0, 0);

#endif /* CONFIG_THREAD_ANALYZER_AUTO_SEPARATE_CORES */

#endif /* CONFIG_THREAD_ANALYZER_AUTO */
