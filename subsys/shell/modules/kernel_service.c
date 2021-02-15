/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/printk.h>
#include <shell/shell.h>
#include <init.h>
#include <debug/object_tracing.h>
#include <power/reboot.h>
#include <debug/stack.h>
#include <string.h>
#include <device.h>
#include <drivers/timer/system_timer.h>
#include <kernel.h>

static int cmd_kernel_version(const struct shell *shell,
			      size_t argc, char **argv)
{
	uint32_t version = sys_kernel_version_get();

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "Zephyr version %d.%d.%d",
		      SYS_KERNEL_VER_MAJOR(version),
		      SYS_KERNEL_VER_MINOR(version),
		      SYS_KERNEL_VER_PATCHLEVEL(version));
	return 0;
}

static int cmd_kernel_uptime(const struct shell *shell,
			     size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "Uptime: %u ms", k_uptime_get_32());
	return 0;
}

static int cmd_kernel_cycles(const struct shell *shell,
			      size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "cycles: %u hw cycles", k_cycle_get_32());
	return 0;
}

#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO) && \
	defined(CONFIG_THREAD_MONITOR)
static void shell_tdata_dump(const struct k_thread *cthread, void *user_data)
{
	struct k_thread *thread = (struct k_thread *)cthread;
	const struct shell *shell = (const struct shell *)user_data;
	unsigned int pcnt;
	size_t unused;
	size_t size = thread->stack_info.size;
	const char *tname;
	int ret;

#ifdef CONFIG_THREAD_RUNTIME_STATS
	k_thread_runtime_stats_t rt_stats_thread;
	k_thread_runtime_stats_t rt_stats_all;
#endif

	tname = k_thread_name_get(thread);

	shell_print(shell, "%s%p %-10s",
		      (thread == k_current_get()) ? "*" : " ",
		      thread,
		      tname ? tname : "NA");
	shell_print(shell, "\toptions: 0x%x, priority: %d timeout: %d",
		      thread->base.user_options,
		      thread->base.prio,
		      thread->base.timeout.dticks);
	shell_print(shell, "\tstate: %s", k_thread_state_str(thread));

#ifdef CONFIG_THREAD_RUNTIME_STATS
	ret = 0;

	if (k_thread_runtime_stats_get(thread, &rt_stats_thread) != 0) {
		ret++;
	};

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
#ifdef CONFIG_64BIT
		shell_print(shell, "\tTotal execution cycles: %llu (%u %%)",
			    rt_stats_thread.execution_cycles,
			    pcnt);
#else
		shell_print(shell, "\tTotal execution cycles: %lu (%u %%)",
			    (uint32_t)rt_stats_thread.execution_cycles,
			    pcnt);
#endif
	} else {
		shell_print(shell, "\tTotal execution cycles: ? (? %%)");
	}
#endif

	ret = k_thread_stack_space_get(thread, &unused);
	if (ret) {
		shell_print(shell,
			    "Unable to determine unused stack size (%d)\n",
			    ret);
	} else {
		/* Calculate the real size reserved for the stack */
		pcnt = ((size - unused) * 100U) / size;

		shell_print(shell,
			    "\tstack size %zu, unused %zu, usage %zu / %zu (%u %%)\n",
			    size, unused, size - unused, size, pcnt);
	}

}

static int cmd_kernel_threads(const struct shell *shell,
			      size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "Scheduler: %u since last call", z_clock_elapsed());
	shell_print(shell, "Threads:");
	k_thread_foreach(shell_tdata_dump, (void *)shell);
	return 0;
}

static void shell_stack_dump(const struct k_thread *thread, void *user_data)
{
	const struct shell *shell = (const struct shell *)user_data;
	unsigned int pcnt;
	size_t unused;
	size_t size = thread->stack_info.size;
	const char *tname;
	int ret;

	ret = k_thread_stack_space_get(thread, &unused);
	if (ret) {
		shell_print(shell,
			    "Unable to determine unused stack size (%d)\n",
			    ret);
		return;
	}

	tname = k_thread_name_get((struct k_thread *)thread);

	/* Calculate the real size reserved for the stack */
	pcnt = ((size - unused) * 100U) / size;

	shell_print((const struct shell *)user_data,
		"%p %-10s (real size %u):\tunused %u\tusage %u / %u (%u %%)",
		      thread,
		      tname ? tname : "NA",
		      size, unused, size - unused, size, pcnt);
}

extern K_KERNEL_STACK_ARRAY_DEFINE(z_interrupt_stacks, CONFIG_MP_NUM_CPUS,
				   CONFIG_ISR_STACK_SIZE);

static int cmd_kernel_stacks(const struct shell *shell,
			     size_t argc, char **argv)
{
	uint8_t *buf;
	size_t size, unused;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	k_thread_foreach(shell_stack_dump, (void *)shell);

	/* Placeholder logic for interrupt stack until we have better
	 * kernel support, including dumping arch-specific exception-related
	 * stack buffers.
	 */
	for (int i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		buf = Z_KERNEL_STACK_BUFFER(z_interrupt_stacks[i]);
		size = K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[i]);

		unused = 0;
		for (size_t i = 0; i < size; i++) {
			if (buf[i] == 0xAAU) {
				unused++;
			} else {
				break;
			}
		}

		shell_print(shell,
			"%p IRQ %02d     (real size %zu):\tunused %zu\tusage %zu / %zu (%zu %%)",
			      &z_interrupt_stacks[i], i, size, unused,
			      size - unused, size,
			      ((size - unused) * 100U) / size);
	}

	return 0;
}
#endif

#if defined(CONFIG_REBOOT)
static int cmd_kernel_reboot_warm(const struct shell *shell,
				  size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	sys_reboot(SYS_REBOOT_WARM);
	return 0;
}

static int cmd_kernel_reboot_cold(const struct shell *shell,
				  size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	sys_reboot(SYS_REBOOT_COLD);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_kernel_reboot,
	SHELL_CMD(cold, NULL, "Cold reboot.", cmd_kernel_reboot_cold),
	SHELL_CMD(warm, NULL, "Warm reboot.", cmd_kernel_reboot_warm),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(sub_kernel,
	SHELL_CMD(cycles, NULL, "Kernel cycles.", cmd_kernel_cycles),
#if defined(CONFIG_REBOOT)
	SHELL_CMD(reboot, &sub_kernel_reboot, "Reboot.", NULL),
#endif
#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO) && \
		defined(CONFIG_THREAD_MONITOR)
	SHELL_CMD(stacks, NULL, "List threads stack usage.", cmd_kernel_stacks),
	SHELL_CMD(threads, NULL, "List kernel threads.", cmd_kernel_threads),
#endif
	SHELL_CMD(uptime, NULL, "Kernel uptime.", cmd_kernel_uptime),
	SHELL_CMD(version, NULL, "Kernel version.", cmd_kernel_version),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(kernel, &sub_kernel, "Kernel commands", NULL);
