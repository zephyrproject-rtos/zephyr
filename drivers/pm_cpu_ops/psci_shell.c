/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/drivers/pm_cpu_ops.h>

/* Zephyr kernel start address. */
extern void __start(void);

static int cmd_reboot_warm(const struct shell *shctx, size_t argc, char **argv)
{
	ARG_UNUSED(shctx);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int ret;

	ret = pm_system_reset(SYS_WARM_RESET);
	if (ret != 0) {
		shell_error(shctx, "Failed to perform system warm reset");
	}

	return ret;
}

static int cmd_reboot_cold(const struct shell *shctx, size_t argc, char **argv)
{
	ARG_UNUSED(shctx);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int ret;

	ret = pm_system_reset(SYS_COLD_RESET);
	if (ret != 0) {
		shell_error(shctx, "Failed to perform system cold reset");
	}

	return ret;
}

static int cmd_psci_cpuon(const struct shell *shctx, size_t argc, char **argv)
{
	ARG_UNUSED(shctx);
	ARG_UNUSED(argc);
	long cpu_id;
	int result;

	errno = 0;
	cpu_id = strtol(argv[1], NULL, 10);
	if (cpu_id == 0 || cpu_id == LONG_MIN || cpu_id == LONG_MAX) {
		if (errno != 0) {
			shell_error(shctx, "psci: invalid input:%ld", cpu_id);
			return -EINVAL;
		}
	}

	result = pm_cpu_on((unsigned long)cpu_id, (uintptr_t)&__start);

	return result;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_reboot,
	SHELL_CMD_ARG(warm, NULL, "System warm reset. Usage: <psci warm>", cmd_reboot_warm, 1, 0),
	SHELL_CMD_ARG(cold, NULL, "System cold reset. Usage: <psci cold>", cmd_reboot_cold, 1, 0),
	SHELL_CMD_ARG(cpuon, NULL, "Power-up the secondary CPU. Usage: <psci cpuon <cpuid>>",
		      cmd_psci_cpuon, 2, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(psci, &sub_reboot, "ARM PSCI interface commands", NULL);
