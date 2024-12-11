/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_shell.h"

#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/kernel.h>

#ifdef CONFIG_THREAD_RUNTIME_STATS
static void rt_stats_dump(const struct shell *sh, struct k_thread *thread)
{
	k_thread_runtime_stats_t rt_stats_thread;
	k_thread_runtime_stats_t rt_stats_all;
	int ret = 0;
	unsigned int pcnt;

	if (k_thread_runtime_stats_get(thread, &rt_stats_thread) != 0) {
		ret++;
	}

	if (k_thread_runtime_stats_all_get(&rt_stats_all) != 0) {
		ret++;
	}

	if (ret == 0) {
		pcnt = (rt_stats_thread.execution_cycles * 100U) /
		       rt_stats_all.execution_cycles;

		/*
		 * z_prf() does not support %llu by default unless
		 * CONFIG_MINIMAL_LIBC_LL_PRINTF=y. So do conditional
		 * compilation to avoid blindly enabling this kconfig
		 * so it won't increase RAM/ROM usage too much on 32-bit
		 * targets.
		 */
		shell_print(sh, "\tTotal execution cycles: %u (%u %%)",
			    (uint32_t)rt_stats_thread.execution_cycles,
			    pcnt);
#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
		shell_print(sh, "\tCurrent execution cycles: %u",
			    (uint32_t)rt_stats_thread.current_cycles);
		shell_print(sh, "\tPeak execution cycles: %u",
			    (uint32_t)rt_stats_thread.peak_cycles);
		shell_print(sh, "\tAverage execution cycles: %u",
			    (uint32_t)rt_stats_thread.average_cycles);
#endif /* CONFIG_SCHED_THREAD_USAGE_ANALYSIS */
	} else {
		shell_print(sh, "\tTotal execution cycles: ? (? %%)");
#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
		shell_print(sh, "\tCurrent execution cycles: ?");
		shell_print(sh, "\tPeak execution cycles: ?");
		shell_print(sh, "\tAverage execution cycles: ?");
#endif /* CONFIG_SCHED_THREAD_USAGE_ANALYSIS */
	}
}
#endif /* CONFIG_THREAD_RUNTIME_STATS */

static void shell_tdata_dump(const struct k_thread *cthread, void *user_data)
{
	struct k_thread *thread = (struct k_thread *)cthread;
	const struct shell *sh = (const struct shell *)user_data;
	unsigned int pcnt;
	size_t unused;
	size_t size = thread->stack_info.size;
	const char *tname;
	int ret;
	char state_str[32];

	tname = k_thread_name_get(thread);

	shell_print(sh, "%s%p %-10s",
		    (thread == k_current_get()) ? "*" : " ",
		    thread,
		    tname ? tname : "NA");
	/* Cannot use lld as it's less portable. */
	shell_print(sh, "\toptions: 0x%x, priority: %d timeout: %" PRId64,
		    thread->base.user_options,
		    thread->base.prio,
		    (int64_t)thread->base.timeout.dticks);
	shell_print(sh, "\tstate: %s, entry: %p",
		    k_thread_state_str(thread, state_str, sizeof(state_str)),
		    thread->entry.pEntry);

#ifdef CONFIG_SCHED_CPU_MASK
	shell_print(sh, "\tcpu_mask: 0x%x", thread->base.cpu_mask);
#endif /* CONFIG_SCHED_CPU_MASK */

	IF_ENABLED(CONFIG_THREAD_RUNTIME_STATS, (rt_stats_dump(sh, thread)));

	ret = k_thread_stack_space_get(thread, &unused);
	if (ret) {
		shell_print(sh,
			    "Unable to determine unused stack size (%d)\n",
			    ret);
	} else {
		/* Calculate the real size reserved for the stack */
		pcnt = ((size - unused) * 100U) / size;

		shell_print(sh,
			    "\tstack size %zu, unused %zu, usage %zu / %zu (%u %%)\n",
			    size, unused, size - unused, size, pcnt);
	}

}

static int cmd_kernel_thread_list(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Scheduler: %u since last call", sys_clock_elapsed());
	shell_print(sh, "Threads:");

	/*
	 * Use the unlocked version as the callback itself might call
	 * arch_irq_unlock.
	 */
	k_thread_foreach_unlocked(shell_tdata_dump, (void *)sh);

	return 0;
}

KERNEL_THREAD_CMD_ADD(list, NULL, "List kernel threads.", cmd_kernel_thread_list);
