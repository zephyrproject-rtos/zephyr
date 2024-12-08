/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_shell.h"

#include <zephyr/kernel.h>

static int cmd_kernel_thread_mask_clear(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int rc, err = 0;
	struct k_thread *thread;

	thread = UINT_TO_POINTER(shell_strtoull(argv[1], 16, &err));
	if (err != 0) {
		shell_error(sh, "Unable to parse thread ID %s (err %d)", argv[1], err);
		return err;
	}

	if (!z_thread_is_valid(thread)) {
		shell_error(sh, "Invalid thread id %p", (void *)thread);
		return -EINVAL;
	}

	rc = k_thread_cpu_mask_clear(thread);
	if (rc != 0) {
		shell_error(sh, "Failed - %d", rc);
	} else {
		shell_print(sh, "%p %s cpu_mask: 0x%x", (void *)thread, thread->name,
			    thread->base.cpu_mask);
	}

	return rc;
}

static int cmd_kernel_thread_mask_enable_all(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int rc, err = 0;
	struct k_thread *thread;

	thread = UINT_TO_POINTER(shell_strtoull(argv[1], 16, &err));
	if (err != 0) {
		shell_error(sh, "Unable to parse thread ID %s (err %d)", argv[1], err);
		return err;
	}

	if (!z_thread_is_valid(thread)) {
		shell_error(sh, "Invalid thread id %p", (void *)thread);
		return -EINVAL;
	}

	rc = k_thread_cpu_mask_enable_all(thread);
	if (rc != 0) {
		shell_error(sh, "Failed - %d", rc);
	} else {
		shell_print(sh, "%p %s cpu_mask: 0x%x", (void *)thread, thread->name,
			    thread->base.cpu_mask);
	}

	return rc;
}

static int cmd_kernel_thread_mask_enable(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int rc, cpu, err = 0;
	struct k_thread *thread;

	thread = UINT_TO_POINTER(shell_strtoull(argv[1], 16, &err));
	if (err != 0) {
		shell_error(sh, "Unable to parse thread ID %s (err %d)", argv[1], err);
		return err;
	}

	if (!z_thread_is_valid(thread)) {
		shell_error(sh, "Invalid thread id %p", (void *)thread);
		return -EINVAL;
	}

	cpu = (int)shell_strtol(argv[2], 10, &err);
	if (err != 0) {
		shell_error(sh, "Unable to parse CPU ID %s (err %d)", argv[2], err);
		return err;
	}

	rc = k_thread_cpu_mask_enable(thread, cpu);
	if (rc != 0) {
		shell_error(sh, "Failed - %d", rc);
	} else {
		shell_print(sh, "%p %s cpu_mask: 0x%x", (void *)thread, thread->name,
			    thread->base.cpu_mask);
	}

	return rc;
}

static int cmd_kernel_thread_mask_disable(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int rc, cpu, err = 0;
	struct k_thread *thread;

	thread = UINT_TO_POINTER(shell_strtoull(argv[1], 16, &err));
	if (err != 0) {
		shell_error(sh, "Unable to parse thread ID %s (err %d)", argv[1], err);
		return err;
	}

	if (!z_thread_is_valid(thread)) {
		shell_error(sh, "Invalid thread id %p", (void *)thread);
		return -EINVAL;
	}

	cpu = (int)shell_strtol(argv[2], 10, &err);
	if (err != 0) {
		shell_error(sh, "Unable to parse CPU ID %s (err %d)", argv[2], err);
		return err;
	}

	rc = k_thread_cpu_mask_disable(thread, cpu);
	if (rc != 0) {
		shell_error(sh, "Failed - %d", rc);
	} else {
		shell_print(sh, "%p %s cpu_mask: 0x%x", (void *)thread, thread->name,
			    thread->base.cpu_mask);
	}

	return rc;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_kernel_thread_mask,
	SHELL_CMD_ARG(clear, NULL,
		      "Sets all CPU enable masks to zero.\n"
		      "Usage: kernel thread mask clear <thread ID>",
		      cmd_kernel_thread_mask_clear, 2, 0),
	SHELL_CMD_ARG(enable_all, NULL,
		      "Sets all CPU enable masks to one.\n"
		      "Usage: kernel thread mask enable_all <thread ID>",
		      cmd_kernel_thread_mask_enable_all, 2, 0),
	SHELL_CMD_ARG(enable, NULL,
		      "Enable thread to run on specified CPU.\n"
		      "Usage: kernel thread mask enable <thread ID> <CPU ID>",
		      cmd_kernel_thread_mask_enable, 3, 0),
	SHELL_CMD_ARG(disable, NULL,
		      "Prevent thread to run on specified CPU.\n"
		      "Usage: kernel thread mask disable <thread ID> <CPU ID>",
		      cmd_kernel_thread_mask_disable, 3, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

KERNEL_THREAD_CMD_ARG_ADD(mask, &sub_kernel_thread_mask, "Configure thread CPU mask affinity.",
			  NULL, 2, 0);
