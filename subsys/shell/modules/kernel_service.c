/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/printk.h>
#include <shell/shell.h>
#include <init.h>
#include <debug/object_tracing.h>
#include <misc/reboot.h>
#include <misc/stack.h>
#include <string.h>

#define SHELL_KERNEL "kernel"

static int shell_cmd_version(int argc, char *argv[])
{
	u32_t version = sys_kernel_version_get();

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	printk("Zephyr version %d.%d.%d\n",
	       SYS_KERNEL_VER_MAJOR(version),
	       SYS_KERNEL_VER_MINOR(version),
	       SYS_KERNEL_VER_PATCHLEVEL(version));
	return 0;
}

static int shell_cmd_uptime(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	printk("uptime: %u ms\n", k_uptime_get_32());

	return 0;
}

static int shell_cmd_cycles(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	printk("cycles: %u hw cycles\n", k_cycle_get_32());

	return 0;
}

#if defined(CONFIG_OBJECT_TRACING) && defined(CONFIG_THREAD_MONITOR)
static void shell_tdata_dump(const struct k_thread *thread, void *user_data)
{
	printk("%s%p:   options: 0x%x priority: %d\n",
		(thread == k_current_get()) ? "*" : " ",
		thread,
		thread->base.user_options,
		thread->base.prio);
}

static int shell_cmd_threads(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	printk("Threads:\n");
	k_thread_foreach(shell_tdata_dump, NULL);

	return 0;
}
#endif


#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_MONITOR) \
				&& defined(CONFIG_THREAD_STACK_INFO)
static void shell_stack_dump(const struct k_thread *thread, void *user_data)
{
	stack_analyze((char *)user_data, (char *)thread->stack_info.start,
						thread->stack_info.size);
}

static int shell_cmd_stack(int argc, char *argv[])
{
	k_thread_foreach(shell_stack_dump, "Shell");
	return 0;
}
#endif

#if defined(CONFIG_REBOOT)
static int shell_cmd_reboot(int argc, char *argv[])
{
	int type;

	if (argc != 2) {
		return -EINVAL;
	}

	if (!strcmp(argv[1], "warm")) {
		type = SYS_REBOOT_WARM;
	} else if (!strcmp(argv[1], "cold")) {
		type = SYS_REBOOT_COLD;
	} else {
		return -EINVAL;
	}

	sys_reboot(type);
	return 0;
}
#endif

struct shell_cmd kernel_commands[] = {
	{ "version", shell_cmd_version, "show kernel version" },
	{ "uptime", shell_cmd_uptime, "show system uptime in milliseconds" },
	{ "cycles", shell_cmd_cycles, "show system hardware cycles" },
#if defined(CONFIG_OBJECT_TRACING) && defined(CONFIG_THREAD_MONITOR)
	{ "threads", shell_cmd_threads, "show running threads" },
#endif
#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_MONITOR) \
				&& defined(CONFIG_THREAD_STACK_INFO)
	{ "stacks", shell_cmd_stack, "show system stacks" },
#endif
#if defined(CONFIG_REBOOT)
	{ "reboot", shell_cmd_reboot, "<warm cold>" },
#endif
	{ NULL, NULL, NULL }
};


SHELL_REGISTER(SHELL_KERNEL, kernel_commands);
