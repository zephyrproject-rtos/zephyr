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
static size_t residency_info_count = DT_NUM_CPU_POWER_STATES(DT_NODELABEL(cpu0));

/* Enabled cpu idle forced as true, only allow cpu enter soc level pm with 'cpu idle *ms' command */
static bool pm_cpu_forced_idle = true;

bool pm_cpu_shell_forced_idle(void)
{
	return pm_cpu_forced_idle;
}

static const char *pm_state_to_str(enum pm_state state)
{
	switch (state) {
	case PM_STATE_ACTIVE:
		return "active";
	case PM_STATE_RUNTIME_IDLE:
		return "runtime-idle";
	case PM_STATE_SUSPEND_TO_IDLE:
		return "suspend-to-idle";
	case PM_STATE_STANDBY:
		return "standby";
	case PM_STATE_SUSPEND_TO_RAM:
		return "suspend-to-ram";
	case PM_STATE_SUSPEND_TO_DISK:
		return "suspend-to-disk";
	case PM_STATE_SOFT_OFF:
		return "soft-off";
	default:
		return "UNKNOWN";
	}
}

static int str_to_pm_state(const char *name, enum pm_state *out)
{
	if (strcmp(name, "runtime-idle") == 0) {
		*out = PM_STATE_RUNTIME_IDLE;
		return 0;
	} else if (strcmp(name, "suspend-to-idle") == 0) {
		*out = PM_STATE_SUSPEND_TO_IDLE;
		return 0;
	} else if (strcmp(name, "standby") == 0) {
		*out = PM_STATE_STANDBY;
		return 0;
	} else if (strcmp(name, "suspend-to-ram") == 0) {
		*out = PM_STATE_SUSPEND_TO_RAM;
		return 0;
	} else if (strcmp(name, "suspend-to-disk") == 0) {
		*out = PM_STATE_SUSPEND_TO_DISK;
		return 0;
	} else if (strcmp(name, "soft-off") == 0) {
		*out = PM_STATE_SOFT_OFF;
		return 0;
	} else {
		return -EINVAL;
	}

}

static int cmd_cpu_states(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct pm_state_info *state_info;

	/* sanity check for first */
	if (residency_info_count == 0) {
		return -EINVAL;
	}

	shell_print(sh, "Supported Low Power States:");

	for (state_info = residency_info;
			state_info < (residency_info + ARRAY_SIZE(residency_info)); state_info++) {
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

	const struct pm_state_info *state_info;
	bool available;
	bool locked;

	shell_print(sh, "Check whether the low power states of the current core are supported:");

	for (state_info = residency_info;
			state_info < (residency_info + ARRAY_SIZE(residency_info)); state_info++) {
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
	if (argc < 2) {
		shell_error(sh, "Usage: cpu lock <state> [substate (0..255)]");
		return -EINVAL;
	}

	enum pm_state st;
	uint8_t sub;
	int err = 0;

	if (str_to_pm_state(argv[1], &st)) {
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
	if (argc < 2) {
		shell_error(sh, "Usage: pm unlock <state> [substate (0..255)]");
		return -EINVAL;
	}

	enum pm_state st;
	uint8_t sub;
	int err = 0;

	if (str_to_pm_state(argv[1], &st)) {
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
	if (argc < 2) {
		shell_error(sh, "Usage: cpu idle times(ms)");
		return -EINVAL;
	}

	int err = 0;
	uint32_t ms = shell_strtoul(argv[1], 0, &err);

	if (err < 0) {
		shell_error(sh, "Unable to parse input (err %d), times", err);
		return err;
	}

	pm_cpu_forced_idle = false;
	k_msleep(ms);
	pm_cpu_forced_idle = true;

	shell_print(sh, "Woke up");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	cpu_cmds,
	SHELL_CMD(states,    NULL, "List supported CPU low power states",	cmd_cpu_states),
	SHELL_CMD(available, NULL, "Show availability/locks for each state",	cmd_cpu_available),
	SHELL_CMD(lock,      NULL, "Lock state: cpu lock <state> <sub>",	cmd_cpu_lock),
	SHELL_CMD(unlock,    NULL, "Unlock state: cpu unlock <state> <sub>",	cmd_cpu_unlock),
	SHELL_CMD(idle,      NULL, "Sleep to let PM work [ms])",		cmd_cpu_idle),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(cpu, &cpu_cmds, "CPU core and power state commands", NULL);
