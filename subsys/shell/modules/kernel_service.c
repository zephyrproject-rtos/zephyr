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

static int cmd_kernel_version(const struct shell *shell,
			      size_t argc, char **argv)
{
	u32_t version = sys_kernel_version_get();

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

#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_MONITOR) \
				&& defined(CONFIG_THREAD_STACK_INFO)
static void shell_tdata_dump(const struct k_thread *cthread, void *user_data)
{
	struct k_thread *thread = (struct k_thread *)cthread;
	const struct shell *shell = (const struct shell *)user_data;
	unsigned int pcnt, unused = 0U;
	unsigned int size = thread->stack_info.size;
	const char *tname;

	unused = stack_unused_space_get((char *)thread->stack_info.start, size);

	/* Calculate the real size reserved for the stack */
	pcnt = ((size - unused) * 100U) / size;

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
	shell_print(shell, "\tstack size %u, unused %u, usage %u / %u (%u %%)\n",
		      size, unused, size - unused, size, pcnt);

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
	unsigned int pcnt, unused = 0U;
	unsigned int size = thread->stack_info.size;
	const char *tname;

	tname = k_thread_name_get((struct k_thread *)thread);
	unused = stack_unused_space_get((char *)thread->stack_info.start, size);

	/* Calculate the real size reserved for the stack */
	pcnt = ((size - unused) * 100U) / size;

	shell_print((const struct shell *)user_data,
		"0x%08X %-10s (real size %u):\tunused %u\tusage %u / %u (%u %%)",
		      (u32_t)thread,
		      tname ? tname : "NA",
		      size, unused, size - unused, size, pcnt);
}

static int cmd_kernel_stacks(const struct shell *shell,
			     size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	k_thread_foreach(shell_stack_dump, (void *)shell);
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
#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_MONITOR) \
				&& defined(CONFIG_THREAD_STACK_INFO)
	SHELL_CMD(stacks, NULL, "List threads stack usage.", cmd_kernel_stacks),
	SHELL_CMD(threads, NULL, "List kernel threads.", cmd_kernel_threads),
#endif
	SHELL_CMD(uptime, NULL, "Kernel uptime.", cmd_kernel_uptime),
	SHELL_CMD(version, NULL, "Kernel version.", cmd_kernel_version),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(kernel, &sub_kernel, "Kernel commands", NULL);
