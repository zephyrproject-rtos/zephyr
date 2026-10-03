/*
 * Copyright (c) 2026 Nerijus Bendžiūnas
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_shell.h"

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#define TOP_DEFAULT_REFRESH_INTERVAL_SECONDS 3
#define TOP_MIN_REFRESH_INTERVAL_SECONDS     1
#define TOP_MAX_REFRESH_INTERVAL_SECONDS     3600

#define TOP_MAX_THREADS CONFIG_KERNEL_THREAD_SHELL_TOP_MAX_THREADS

#define TOP_VT100_CLEAR_SCREEN_AND_CURSOR_HOME "\x1b[2J\x1b[H"

#define ASCII_END_OF_TEXT_CTRL_C 0x03

#define TOP_STACK_USAGE_UNKNOWN UINT8_MAX

struct top_thread_snapshot {
	struct k_thread *thread;
	uint64_t execution_cycles;
};

struct top_thread_row {
	struct k_thread *thread;
	uint64_t execution_cycles;
	uint64_t delta_cycles;
	int priority;
	char state[16];
	char name[24];
#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO)
	uint8_t stack_usage_percent;
#endif
};

static struct top_context {
	struct k_work_delayable work;
	const struct shell *sh;
	atomic_t active;
	uint32_t interval_seconds;
	uint64_t previous_all_cycles;
	uint64_t previous_busy_cycles;
	struct top_thread_snapshot snapshots[TOP_MAX_THREADS];
	size_t snapshot_count;
	struct top_thread_row rows[TOP_MAX_THREADS];
	size_t row_count;
	size_t thread_count;
	size_t hidden_count;
} top_context;

static void top_stop(struct top_context *context)
{
	atomic_set(&context->active, 0);
	k_work_cancel_delayable(&context->work);
	shell_set_bypass(context->sh, NULL, NULL);
}

static void top_collect_cb(const struct k_thread *cthread, void *user_data)
{
	struct k_thread *thread = (struct k_thread *)cthread;
	struct top_context *context = user_data;
	k_thread_runtime_stats_t stats;
	struct top_thread_row *row;
	const char *name;

	context->thread_count++;

	if (context->row_count >= ARRAY_SIZE(context->rows)) {
		context->hidden_count++;
		return;
	}

	if (k_thread_runtime_stats_get(thread, &stats) != 0) {
		context->hidden_count++;
		return;
	}

	row = &context->rows[context->row_count++];
	row->thread = thread;
	row->execution_cycles = stats.execution_cycles;
	row->priority = thread->base.prio;
	k_thread_state_str(thread, row->state, sizeof(row->state));

	name = k_thread_name_get(thread);
	if (name != NULL && name[0] != '\0') {
		strncpy(row->name, name, sizeof(row->name) - 1);
		row->name[sizeof(row->name) - 1] = '\0';
	} else {
		snprintk(row->name, sizeof(row->name), "%p", (void *)thread);
	}

	/* First refresh has no prior snapshot, so delta is since-boot, like top's first frame. */
	row->delta_cycles = stats.execution_cycles;
	for (size_t i = 0; i < context->snapshot_count; i++) {
		if (context->snapshots[i].thread != thread) {
			continue;
		}
		/*
		 * A dead thread's memory may be reused by a new thread with a
		 * smaller cycle count; treat a backwards jump as a new thread.
		 */
		if (stats.execution_cycles >= context->snapshots[i].execution_cycles) {
			row->delta_cycles =
				stats.execution_cycles - context->snapshots[i].execution_cycles;
		}
		break;
	}

#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO)
	size_t unused_stack;
	size_t stack_size = thread->stack_info.size;

	if ((k_thread_stack_space_get(thread, &unused_stack) == 0) && (stack_size > 0)) {
		row->stack_usage_percent = ((stack_size - unused_stack) * 100U) / stack_size;
	} else {
		row->stack_usage_percent = TOP_STACK_USAGE_UNKNOWN;
	}
#endif /* CONFIG_INIT_STACKS && CONFIG_THREAD_STACK_INFO */
}

static void top_sort_rows(struct top_context *context)
{
	/* Table is small; insertion sort keeps it descending by delta cycles. */
	for (size_t i = 1; i < context->row_count; i++) {
		struct top_thread_row key = context->rows[i];
		size_t j = i;

		while ((j > 0) && (context->rows[j - 1].delta_cycles < key.delta_cycles)) {
			context->rows[j] = context->rows[j - 1];
			j--;
		}
		context->rows[j] = key;
	}
}

static void top_permille_split(uint64_t part, uint64_t whole, uint32_t *integer_part,
			       uint32_t *tenths)
{
	uint64_t permille = 0;

	if (whole > 0) {
		permille = MIN((part * 1000U) / whole, 1000U);
	}

	*integer_part = (uint32_t)(permille / 10U);
	*tenths = (uint32_t)(permille % 10U);
}

static void top_screen_print(struct top_context *context, uint64_t all_delta, uint64_t busy_delta)
{
	const struct shell *sh = context->sh;
	int64_t uptime_ms = k_uptime_get();
	uint32_t busy_percent;
	uint32_t busy_tenths;
	uint32_t idle_percent;
	uint32_t idle_tenths;

	top_permille_split(busy_delta, all_delta, &busy_percent, &busy_tenths);
	top_permille_split(all_delta - MIN(busy_delta, all_delta), all_delta, &idle_percent,
			   &idle_tenths);

	shell_fprintf(sh, SHELL_NORMAL, TOP_VT100_CLEAR_SCREEN_AND_CURSOR_HOME);
	shell_print(sh, "top - uptime %lld.%03lld s, refresh %u s, press 'q' to quit",
		    uptime_ms / MSEC_PER_SEC, uptime_ms % MSEC_PER_SEC, context->interval_seconds);
	shell_print(sh, "Threads: %zu total, CPU: %u.%u%% busy, %u.%u%% idle",
		    context->thread_count, busy_percent, busy_tenths, idle_percent, idle_tenths);
	shell_print(sh, "");

#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO)
	shell_print(sh, " %%CPU  PRIO  STATE       STACK%%  NAME");
#else
	shell_print(sh, " %%CPU  PRIO  STATE       NAME");
#endif /* CONFIG_INIT_STACKS && CONFIG_THREAD_STACK_INFO */

	for (size_t i = 0; i < context->row_count; i++) {
		struct top_thread_row *row = &context->rows[i];
		uint32_t cpu_percent;
		uint32_t cpu_tenths;

		top_permille_split(row->delta_cycles, all_delta, &cpu_percent, &cpu_tenths);

#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO)
		char stack_usage[8];

		if (row->stack_usage_percent == TOP_STACK_USAGE_UNKNOWN) {
			snprintk(stack_usage, sizeof(stack_usage), "?");
		} else {
			snprintk(stack_usage, sizeof(stack_usage), "%u%%",
				 row->stack_usage_percent);
		}
		shell_print(sh, "%3u.%u  %4d  %-10.10s  %6s  %s", cpu_percent, cpu_tenths,
			    row->priority, row->state, stack_usage, row->name);
#else
		shell_print(sh, "%3u.%u  %4d  %-10.10s  %s", cpu_percent, cpu_tenths, row->priority,
			    row->state, row->name);
#endif /* CONFIG_INIT_STACKS && CONFIG_THREAD_STACK_INFO */
	}

	if (context->hidden_count > 0) {
		shell_print(sh, "(%zu more threads not shown, see %s)", context->hidden_count,
			    "CONFIG_KERNEL_THREAD_SHELL_TOP_MAX_THREADS");
	}
}

static void top_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct top_context *context = CONTAINER_OF(dwork, struct top_context, work);
	k_thread_runtime_stats_t all_stats;
	uint64_t all_delta;
	uint64_t busy_delta;

	if (atomic_get(&context->active) == 0) {
		return;
	}

	if (k_thread_runtime_stats_all_get(&all_stats) != 0) {
		shell_error(context->sh, "Failed to get CPU statistics");
		top_stop(context);
		return;
	}

	context->row_count = 0;
	context->thread_count = 0;
	context->hidden_count = 0;
	k_thread_foreach_unlocked(top_collect_cb, context);

	all_delta = all_stats.execution_cycles - context->previous_all_cycles;
	busy_delta = all_stats.total_cycles - context->previous_busy_cycles;

	top_sort_rows(context);
	top_screen_print(context, all_delta, busy_delta);

	for (size_t i = 0; i < context->row_count; i++) {
		context->snapshots[i].thread = context->rows[i].thread;
		context->snapshots[i].execution_cycles = context->rows[i].execution_cycles;
	}
	context->snapshot_count = context->row_count;
	context->previous_all_cycles = all_stats.execution_cycles;
	context->previous_busy_cycles = all_stats.total_cycles;

	if (atomic_get(&context->active) == 1) {
		k_work_reschedule(&context->work, K_SECONDS(context->interval_seconds));
	}
}

static void top_bypass_cb(const struct shell *sh, uint8_t *data, size_t len, void *user_data)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(user_data);

	for (size_t i = 0; i < len; i++) {
		if ((data[i] == 'q') || (data[i] == 'Q') || (data[i] == ASCII_END_OF_TEXT_CTRL_C)) {
			top_stop(&top_context);
			break;
		}
	}
}

static int cmd_kernel_thread_top(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t interval_seconds = TOP_DEFAULT_REFRESH_INTERVAL_SECONDS;

	if (argc >= 2) {
		uint64_t value;
		int err = 0;

		if ((strcmp(argv[1], "-d") != 0) || (argc != 3)) {
			shell_error(sh, "Usage: top [-d <seconds>]");
			return -EINVAL;
		}

		value = shell_strtoul(argv[2], 10, &err);
		if ((err != 0) || (value < TOP_MIN_REFRESH_INTERVAL_SECONDS) ||
		    (value > TOP_MAX_REFRESH_INTERVAL_SECONDS)) {
			shell_error(sh, "Invalid interval: %s (allowed: %u-%u seconds)", argv[2],
				    TOP_MIN_REFRESH_INTERVAL_SECONDS,
				    TOP_MAX_REFRESH_INTERVAL_SECONDS);
			return -EINVAL;
		}
		interval_seconds = (uint32_t)value;
	}

	if (atomic_get(&top_context.active) == 1) {
		shell_error(sh, "top is already running");
		return -EBUSY;
	}

	top_context.sh = sh;
	top_context.interval_seconds = interval_seconds;
	top_context.previous_all_cycles = 0;
	top_context.previous_busy_cycles = 0;
	top_context.snapshot_count = 0;

	k_work_init_delayable(&top_context.work, top_work_handler);
	atomic_set(&top_context.active, 1);
	shell_set_bypass(sh, top_bypass_cb, NULL);
	k_work_reschedule(&top_context.work, K_NO_WAIT);

	return 0;
}

KERNEL_THREAD_CMD_ARG_ADD(top, NULL,
			  "Show per-thread CPU usage, refreshed periodically.\n"
			  "Usage: top [-d <seconds>] (default 3), press 'q' to quit.",
			  cmd_kernel_thread_top, 1, 2);
