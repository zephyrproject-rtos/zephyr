/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pm_cpu_shell, CONFIG_PM_LOG_LEVEL);

/* Supported states info from devicetree (CPU0) */
static const struct pm_state_info residency_info[] =
	PM_STATE_INFO_LIST_FROM_DT_CPU(DT_NODELABEL(cpu0));

static int cmd_cpu_states(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* sanity check for first */
	if (ARRAY_SIZE(residency_info) == 0U) {
		shell_warn(sh, "No pm states");
		return -EINVAL;
	}

	shell_print(sh, "Supported Low Power States:");

	ARRAY_FOR_EACH_PTR(residency_info, state_info) {
		shell_print(sh,
		"  - State: %s, Substate: %d, Residency: %dus, Latency: %dus, PM Device Disabled: %s",
			pm_state_to_str(state_info->state),
			state_info->substate_id,
			state_info->min_residency_us,
			state_info->exit_latency_us,
			state_info->pm_device_disabled ? "Yes" : "No");
	}

	return 0;
}

static int cmd_cpu_available(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* sanity check for first */
	if (ARRAY_SIZE(residency_info) == 0U) {
		shell_warn(sh, "No pm states");
		return -EINVAL;
	}

	bool available;
	bool locked;

	shell_print(sh, "Check whether the low power states of the current core are supported:");

	ARRAY_FOR_EACH_PTR(residency_info, state_info) {
		available = pm_policy_state_is_available(state_info->state,
				state_info->substate_id);
		locked = pm_policy_state_lock_is_active(state_info->state,
				state_info->substate_id);

		shell_print(sh, " - %-16s sub=%-3u avail=%c lock=%c",
				pm_state_to_str(state_info->state),
				state_info->substate_id,
				available ? 'Y' : 'N',
				locked ? 'Y' : 'N');
	}

	return 0;
}

static int cmd_cpu_lock(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	enum pm_state st;
	uint8_t sub;
	int err;

	if (pm_state_from_str(argv[1], &st) < 0) {
		shell_error(sh, "Unknown state: %s", argv[1]);
		return -EINVAL;
	}

	sub = shell_strtoul(argv[2], 0, &err);
	if (err < 0) {
		shell_error(sh, "Unable to parse input (err %d), substate", err);
		return err;
	}

	pm_policy_state_lock_get(st, sub);

	shell_print(sh, "Locked %s sub=%u", argv[1], sub);

	return 0;
}

static int cmd_cpu_unlock(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	enum pm_state st;
	uint8_t sub;
	int err = 0;

	if (pm_state_from_str(argv[1], &st) < 0) {
		shell_error(sh, "Unknown state: %s", argv[1]);
		return -EINVAL;
	}

	sub = shell_strtoul(argv[2], 0, &err);
	if (err < 0) {
		shell_error(sh, "Unable to parse input (err %d), substate", err);
		return err;
	}

	pm_policy_state_lock_put(st, sub);

	shell_print(sh, "Unlocked %s sub=%u", argv[1], sub);

	return 0;
}

static int cmd_cpu_idle(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int err;
	uint32_t ms;

	ms = shell_strtoul(argv[1], 0, &err);
	if (err < 0) {
		shell_error(sh, "Unable to parse input (err %d), times", err);
		return err;
	}

	k_msleep(ms);

	shell_print(sh, "Woke up");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	cpu_cmds,
	SHELL_CMD_ARG(states,    NULL,
		SHELL_HELP("List supported CPU low power states", ""), cmd_cpu_states, 1, 0),
	SHELL_CMD_ARG(available, NULL,
		SHELL_HELP("Show availability/locks for each state", ""), cmd_cpu_available, 1, 0),
	SHELL_CMD_ARG(lock,      NULL,
		SHELL_HELP("Lock a state", "<state> <substate>"), cmd_cpu_lock, 2, 1),
	SHELL_CMD_ARG(unlock,    NULL,
		SHELL_HELP("Unlock a state", "<state> <substate>"), cmd_cpu_unlock, 2, 1),
	SHELL_CMD_ARG(idle,      NULL,
		SHELL_HELP("Sleep current thread to let PM work", "<ms>"), cmd_cpu_idle, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(cpu, &cpu_cmds, "CPU core and power state commands", NULL);
