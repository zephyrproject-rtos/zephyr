/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Demo: stay running; user types shell command to trigger fault (good for repeatable UDP runs).
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/debug/coredump.h>

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	ARG_UNUSED(pEsf);

	printk("%s is expected; reason = %u; halting ...\n", __func__, reason);
	k_fatal_halt(reason);
}

static int cmd_coredump_crash(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Triggering null deref for coredump (UDP + UART)...");
	uint32_t *p = NULL;

	*p = 0;
	return 0;
}

SHELL_CMD_REGISTER(coredump_crash, NULL, "Trigger coredump via fault", cmd_coredump_crash);

int main(void)
{
	printk("demo_shell: %s — open shell, run: coredump_crash\n", CONFIG_BOARD);
	return 0;
}
