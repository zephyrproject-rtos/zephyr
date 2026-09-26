/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/sys/util.h>

#define STATS_MAX_THREADS 48

struct stats_ctx {
	int cpu_filter; /* -1 = all CPUs */
	bool delta_mode;
	unsigned int interval_ms;
	unsigned int repeat_count;
};

struct stats_thread_row {
	struct k_thread *thread;
	uint64_t exec_cycles;
	unsigned int cpu_pct;
	unsigned int stack_pct;
	int8_t prio;
#ifdef CONFIG_SMP
	uint8_t cpu;
#endif
	char state[24];
	char name[16];
};

struct stats_cpu_snap {
	uint64_t exec_cycles;
	uint64_t busy_cycles;
	uint64_t idle_cycles;
};

static struct stats_ctx stats_cfg = {
	.cpu_filter = -1,
};

static struct stats_cpu_snap stats_cpu_prev[CONFIG_MP_MAX_NUM_CPUS];
static struct stats_thread_row stats_thread_prev[STATS_MAX_THREADS];
static size_t stats_thread_prev_count;
static uint64_t stats_interval_cycles;

static unsigned int stats_pct(uint64_t part, uint64_t whole)
{
	if (whole == 0U) {
		return 0U;
	}

	return (unsigned int)((part * 100U) / whole);
}

static bool stats_cpu_have_baseline(unsigned int cpu)
{
	return stats_cpu_prev[cpu].exec_cycles != 0U;
}

static void stats_print_cpus_header(const struct shell *sh, bool delta)
{
	if (delta) {
		shell_print(sh, "CPU  busy%% idle%%  (interval)");
	} else {
		shell_print(sh, "CPU  busy%% idle%%  busy_cycles   idle_cycles");
	}
}

static int stats_get_denom_cycles(int cpu_filter, k_thread_runtime_stats_t *denom)
{
	if (cpu_filter >= 0) {
		return k_thread_runtime_stats_cpu_get((int)cpu_filter, denom);
	}

	return k_thread_runtime_stats_all_get(denom);
}

static void stats_format_name(struct k_thread *thread, char *buf, size_t len)
{
	const char *tname = k_thread_name_get(thread);

	if (tname != NULL && tname[0] != '\0') {
		strncpy(buf, tname, len - 1U);
		buf[len - 1U] = '\0';
		return;
	}

	snprintk(buf, len, "0x%08x", (unsigned int)(uintptr_t)thread);
}

static int stats_show_cpus(const struct shell *sh, bool delta)
{
	k_thread_runtime_stats_t st;
	unsigned int num_cpus = IS_ENABLED(CONFIG_SMP) ? arch_num_cpus() : 1U;
	unsigned int cpu_start = 0U;
	unsigned int cpu_end = num_cpus;
	uint64_t interval_sum = 0U;

	stats_print_cpus_header(sh, delta);

	if (stats_cfg.cpu_filter >= 0) {
		cpu_start = (unsigned int)stats_cfg.cpu_filter;
		cpu_end = cpu_start + 1U;
	}

	for (unsigned int cpu = cpu_start; cpu < cpu_end; cpu++) {
		uint64_t busy;
		uint64_t idle;
		uint64_t exec;
		unsigned int busy_pct;
		unsigned int idle_pct;

		if (k_thread_runtime_stats_cpu_get((int)cpu, &st) != 0) {
			shell_print(sh, "%3u  (stats unavailable)", cpu);
			continue;
		}

		busy = st.total_cycles;
		idle = st.idle_cycles;
		exec = st.execution_cycles;

		if (delta && stats_cpu_have_baseline(cpu)) {
			uint64_t busy_d = busy - stats_cpu_prev[cpu].busy_cycles;
			uint64_t idle_d = idle - stats_cpu_prev[cpu].idle_cycles;
			uint64_t window = busy_d + idle_d;

			interval_sum += window;
			busy_pct = stats_pct(busy_d, window);
			idle_pct = stats_pct(idle_d, window);
			shell_print(sh, "%3u  %4u   %4u", cpu, busy_pct, idle_pct);
		} else if (delta) {
			shell_print(sh, "%3u     (baseline)", cpu);
		} else {
			busy_pct = stats_pct(busy, exec);
			idle_pct = stats_pct(idle, exec);
			shell_print(sh, "%3u  %4u   %4u   %10llu %10llu", cpu, busy_pct, idle_pct,
				    (unsigned long long)busy, (unsigned long long)idle);
		}

		stats_cpu_prev[cpu].exec_cycles = exec;
		stats_cpu_prev[cpu].busy_cycles = busy;
		stats_cpu_prev[cpu].idle_cycles = idle;
	}

	stats_interval_cycles = interval_sum;

	return 0;
}

struct stats_collect_ud {
	struct stats_thread_row *rows;
	size_t count;
	size_t max;
	k_thread_runtime_stats_t denom;
	int cpu_filter;
	bool truncated;
};

static void stats_collect_thread_cb(const struct k_thread *thread, void *user_data)
{
	struct stats_collect_ud *ud = user_data;
	struct stats_thread_row *row;
	k_thread_runtime_stats_t th_stats;
	size_t unused;
	size_t stack_size = thread->stack_info.size;
	char state_str[24];
	unsigned int stack_pct = 0U;

	if (ud->count >= ud->max) {
		ud->truncated = true;
		return;
	}

#ifdef CONFIG_SMP
	if (ud->cpu_filter >= 0 && (int)thread->base.cpu != ud->cpu_filter) {
		return;
	}
#endif

	if (k_thread_runtime_stats_get((struct k_thread *)thread, &th_stats) != 0) {
		return;
	}

	row = &ud->rows[ud->count];
	row->thread = (struct k_thread *)thread;
	row->exec_cycles = th_stats.execution_cycles;
	row->cpu_pct = stats_pct(th_stats.execution_cycles, ud->denom.execution_cycles);
	row->prio = thread->base.prio;
#ifdef CONFIG_SMP
	row->cpu = thread->base.cpu;
#endif
	stats_format_name((struct k_thread *)thread, row->name, sizeof(row->name));
	(void)k_thread_state_str((struct k_thread *)thread, state_str, sizeof(state_str));
	strncpy(row->state, state_str, sizeof(row->state) - 1U);
	row->state[sizeof(row->state) - 1U] = '\0';

	if (k_thread_stack_space_get((struct k_thread *)thread, &unused) == 0 && stack_size > 0U) {
		stack_pct = stats_pct(stack_size - unused, stack_size);
	}
	row->stack_pct = stack_pct;

	ud->count++;
}

static void stats_sort_rows(struct stats_thread_row *rows, size_t n)
{
	for (size_t i = 0; i + 1U < n; i++) {
		for (size_t j = i + 1U; j < n; j++) {
			if (rows[j].cpu_pct > rows[i].cpu_pct) {
				struct stats_thread_row tmp = rows[i];

				rows[i] = rows[j];
				rows[j] = tmp;
			}
		}
	}
}

static unsigned int stats_thread_delta_pct(struct k_thread *thread, uint64_t exec_cycles)
{
	uint64_t exec_d = exec_cycles;
	size_t i;

	for (i = 0; i < stats_thread_prev_count; i++) {
		if (stats_thread_prev[i].thread == thread) {
			exec_d = exec_cycles - stats_thread_prev[i].exec_cycles;
			break;
		}
	}

	return stats_pct(exec_d, stats_interval_cycles);
}

static void stats_save_thread_prev(struct stats_thread_row *rows, size_t n)
{
	stats_thread_prev_count = MIN(n, ARRAY_SIZE(stats_thread_prev));

	for (size_t i = 0; i < stats_thread_prev_count; i++) {
		stats_thread_prev[i] = rows[i];
	}
}

static int stats_show_threads_table(const struct shell *sh, bool delta)
{
	/* Static to avoid a 3+ KB stack allocation that overflows the shell
	 * thread stack, especially with LOG_MODE_IMMEDIATE which adds cbprintf
	 * processing overhead inline.  Shell commands are single-threaded so
	 * re-entrancy is not a concern.
	 */
	static struct stats_thread_row rows[STATS_MAX_THREADS];
	struct stats_collect_ud ud = {
		.rows = rows,
		.count = 0,
		.max = ARRAY_SIZE(rows),
		.cpu_filter = stats_cfg.cpu_filter,
		.truncated = false,
	};

	if (stats_get_denom_cycles(stats_cfg.cpu_filter, &ud.denom) != 0) {
		shell_error(sh, "runtime stats unavailable (enable THREAD_RUNTIME_STATS)");
		return -ENOTSUP;
	}

	k_thread_foreach_unlocked(stats_collect_thread_cb, &ud);

	if (delta && stats_interval_cycles == 0U) {
		stats_save_thread_prev(rows, ud.count);
		return 0;
	}

	for (size_t i = 0; i < ud.count; i++) {
		if (delta) {
			rows[i].cpu_pct = stats_thread_delta_pct(rows[i].thread,
								  rows[i].exec_cycles);
		}
	}

	stats_sort_rows(rows, ud.count);

	if (delta) {
#ifdef CONFIG_SMP
		shell_print(sh, "CUR CPU NAME           PRIO STATE      dCPU STK%%");
#else
		shell_print(sh, "CUR NAME           PRIO STATE      dCPU STK%%");
#endif
	} else {
#ifdef CONFIG_SMP
		shell_print(sh, "CUR CPU NAME           PRIO STATE      CPU%% STK%%");
#else
		shell_print(sh, "CUR NAME           PRIO STATE      CPU%% STK%%");
#endif
	}

	for (size_t i = 0; i < ud.count; i++) {
		struct stats_thread_row *row = &rows[i];
		const char *cur = (row->thread == k_current_get()) ? "*" : " ";

#ifdef CONFIG_SMP
		shell_print(sh, "%s %3u %-16s %4d %-12s %4u %4u", cur, row->cpu, row->name,
			    row->prio, row->state, row->cpu_pct, row->stack_pct);
#else
		shell_print(sh, "%s   %-16s %4d %-12s %4u %4u", cur, row->name, row->prio,
			    row->state, row->cpu_pct, row->stack_pct);
#endif
	}

	if (ud.truncated) {
		shell_warn(sh, "table truncated at %u threads", STATS_MAX_THREADS);
	}

	stats_save_thread_prev(rows, ud.count);

	return 0;
}

static int stats_parse_common_opts(const struct shell *sh, size_t argc, char **argv)
{
	stats_cfg.cpu_filter = -1;
	stats_cfg.delta_mode = false;
	stats_cfg.interval_ms = 0U;
	stats_cfg.repeat_count = 1U;

	for (size_t i = 1; i < argc; i++) {
		int err = 0;

		if (strcmp(argv[i], "-c") == 0) {
			unsigned int num_cpus = IS_ENABLED(CONFIG_SMP) ? arch_num_cpus() : 1U;

			if (i + 1U >= argc) {
				shell_error(sh, "-c requires CPU index");
				return -EINVAL;
			}
			stats_cfg.cpu_filter = (int)shell_strtol(argv[i + 1U], 10, &err);
			if (err != 0 || stats_cfg.cpu_filter < 0 ||
			    stats_cfg.cpu_filter >= (int)num_cpus) {
				shell_error(sh, "invalid CPU index");
				return -EINVAL;
			}
			i++;
			continue;
		}

		if (strcmp(argv[i], "-d") == 0) {
			stats_cfg.delta_mode = true;
			continue;
		}

		if (strcmp(argv[i], "-i") == 0) {
			if (i + 1U >= argc) {
				shell_error(sh, "-i requires interval in ms");
				return -EINVAL;
			}
			stats_cfg.interval_ms = (unsigned int)shell_strtoul(argv[i + 1U], 10, &err);
			if (err != 0 || stats_cfg.interval_ms == 0U) {
				shell_error(sh, "invalid interval");
				return -EINVAL;
			}
			i++;
			continue;
		}

		if (strcmp(argv[i], "-n") == 0) {
			if (i + 1U >= argc) {
				shell_error(sh, "-n requires repeat count");
				return -EINVAL;
			}
			stats_cfg.repeat_count =
				(unsigned int)shell_strtoul(argv[i + 1U], 10, &err);
			if (err != 0 || stats_cfg.repeat_count == 0U) {
				shell_error(sh, "invalid repeat count");
				return -EINVAL;
			}
			i++;
			continue;
		}

		shell_error(sh, "unknown option: %s", argv[i]);
		return -EINVAL;
	}

	return 0;
}

static int cmd_stats_cpus(const struct shell *sh, size_t argc, char **argv)
{
	int ret = stats_parse_common_opts(sh, argc, argv);

	if (ret != 0) {
		return ret;
	}

	if (stats_cfg.interval_ms == 0U) {
		return stats_show_cpus(sh, stats_cfg.delta_mode);
	}

	for (unsigned int n = 0; n < stats_cfg.repeat_count; n++) {
		if (n > 0U) {
			k_msleep(stats_cfg.interval_ms);
		}
		shell_print(sh, "--- sample %u ---", n + 1U);
		stats_show_cpus(sh, n > 0U);
	}

	return 0;
}

static int cmd_stats_threads(const struct shell *sh, size_t argc, char **argv)
{
	int ret = stats_parse_common_opts(sh, argc, argv);

	if (ret != 0) {
		return ret;
	}

	if (stats_cfg.interval_ms == 0U) {
		return stats_show_threads_table(sh, stats_cfg.delta_mode);
	}

	for (unsigned int n = 0; n < stats_cfg.repeat_count; n++) {
		if (n > 0U) {
			k_msleep(stats_cfg.interval_ms);
		}
		shell_print(sh, "--- sample %u ---", n + 1U);
		(void)stats_show_cpus(sh, n > 0U);
		stats_show_threads_table(sh, n > 0U);
	}

	return 0;
}

static int cmd_stats_top(const struct shell *sh, size_t argc, char **argv)
{
	int ret = stats_parse_common_opts(sh, argc, argv);

	if (ret != 0) {
		return ret;
	}

	if (stats_cfg.interval_ms == 0U) {
		stats_cfg.interval_ms = 1000U;
	}

	if (stats_cfg.repeat_count == 1U) {
		stats_cfg.repeat_count = 0U; /* 0 = run until interrupted */
	}

	for (unsigned int n = 0;; n++) {
		const bool use_delta = (n > 0U);

		shell_print(sh, "=== sched top %u%s ===", n + 1U,
			    use_delta ? "" : " (baseline)");
		stats_show_cpus(sh, use_delta);
		stats_show_threads_table(sh, use_delta);

		if (stats_cfg.repeat_count != 0U && (n + 1U) >= stats_cfg.repeat_count) {
			break;
		}

		k_msleep(stats_cfg.interval_ms);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_sched,
	SHELL_CMD_ARG(cpus, NULL,
		      "Per-CPU busy/idle summary [-c cpu] [-d] [-i ms] [-n count]",
		      cmd_stats_cpus, 1, 8),
	SHELL_CMD_ARG(threads, NULL,
		      "Thread CPU/stack table [-c cpu] [-d] [-i ms] [-n count]",
		      cmd_stats_threads, 1, 8),
	SHELL_CMD_ARG(top, NULL,
		      "Live CPU + thread view [-c cpu] [-d] [-i ms] [-n count] (default -i 1000)",
		      cmd_stats_top, 1, 8),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(sched, &sub_sched,
		   "Scheduler CPU/thread stats (requires THREAD_RUNTIME_STATS)", NULL);
