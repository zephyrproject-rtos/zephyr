/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/drivers/pm_cpu_ops.h>
#include <zephyr/drivers/pm_cpu_ops/psci.h>

/* Zephyr kernel start address. */
extern void __start(void);

static int cmd_reboot_warm(const struct shell *shctx, size_t argc, char **argv)
{
	ARG_UNUSED(shctx);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int ret = 0;

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
	int ret = 0;

	ret = pm_system_reset(SYS_COLD_RESET);
	if (ret != 0) {
		shell_error(shctx, "Failed to perform system cold reset");
	}

	return ret;
}

static int cmd_psci_cpuon(const struct shell *shctx, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	unsigned long cpu_id = 0;
	int result;

	result = 0;
	cpu_id = shell_strtoul(argv[1], 10, &result);
	if (result != 0) {
		shell_error(shctx, "psci: invalid input");
		return result;
	} else if (cpu_id > CONFIG_MP_MAX_NUM_CPUS) {
		shell_error(shctx, "psci: cpu_id out of range");
		return -ERANGE;
	}

	result = pm_cpu_on(cpu_id, (uintptr_t)&__start);
	if (result != 0) {
		shell_error(shctx, "psci: failed to power on cpu core %lu", cpu_id);
	}

	return result;
}

static int cmd_psci_version(const struct shell *shctx, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	uint32_t version = 0U;

	version = psci_version();
	shell_info(shctx, "psci version: %d.%d\n", PSCI_VERSION_MAJOR(version),
		   PSCI_VERSION_MINOR(version));

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_reboot,
	SHELL_CMD_ARG(warm, NULL, "System warm reset. Usage: <psci warm>", cmd_reboot_warm, 1, 0),
	SHELL_CMD_ARG(cold, NULL, "System cold reset. Usage: <psci cold>", cmd_reboot_cold, 1, 0),
	SHELL_CMD_ARG(version, NULL, "Get PSCI version. Usage: <psci version>", cmd_psci_version, 1,
		      0),
	SHELL_CMD_ARG(cpuon, NULL, "Power-up the secondary CPUs. Usage: <psci cpuon <cpuid>>",
		      cmd_psci_cpuon, 2, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(psci, &sub_reboot, "ARM PSCI interface commands", NULL);
