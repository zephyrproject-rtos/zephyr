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

/* Define the maximum sleep time*/
#define PM_CPU_SHELL_MAX_IDLE_MS	CONFIG_PM_CPU_SHELL_MAX_IDLE_MS

/* Supported states info from devicetree (CPU0) */
static const struct pm_state_info residency_info[] =
	PM_STATE_INFO_LIST_FROM_DT_CPU(DT_NODELABEL(cpu0));

static int cmd_cpu_states(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* sanity check for first */
	if (ARRAY_SIZE(residency_info) == 0U) {
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
	if (argc < 2) {
		shell_error(sh, "Usage: cpu lock <state> [substate (0..255)]");
	}

	enum pm_state st;
	uint8_t sub;
	int err;

	if (pm_state_from_str(argv[1], &st)) {
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

	if (pm_state_from_str(argv[1], &st)) {
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

	int err;
	uint32_t ms;
	uint64_t sleep_us;
	const struct pm_state_info *state_chosen = NULL;

	ms = shell_strtoul(argv[1], 0, &err);
	if (err < 0) {
		shell_error(sh, "Unable to parse input (err %d), times", err);
		return err;
	}

	sleep_us = (uint64_t)ms * 1000U;

	/* sanity test firstly for input time */
	if (ms == 0 || ms > PM_CPU_SHELL_MAX_IDLE_MS) {
		shell_print(sh, "Invalid sleep time");
		return 0;
	}

	/* Judge which soc low power mode will enter by policy availability and level */
	ARRAY_FOR_EACH_PTR(residency_info, state) {
		if (!pm_policy_state_is_available(state->state, state->substate_id)) {
			continue;
		}

		if (sleep_us < (uint64_t)state->min_residency_us) {
			continue;
		}

		state_chosen = state;
	}

	if (state_chosen == NULL) {
		shell_print(sh, "No eligible SoC low power state, stay in CPU idle");
	} else {
		shell_print(sh,
				"Target SoC LPM: %s sub=%u Sleeping %u ms...",
				pm_state_to_str(state_chosen->state),
				state_chosen->substate_id,
				ms);
	}

	k_msleep(ms);

	shell_print(sh, "Woke up");

	return 0;
}

/* lock all soc level low power mode */
static int pm_cpu_lock_soc_low_power(void)
{
	const struct pm_state_info *states;
	uint8_t cpu_idx = 0;
	uint8_t num_cpu_states = pm_state_cpu_get_all(cpu_idx, &states);

	for (uint8_t i = 0; i < num_cpu_states; i++) {
		if (states[i].state > PM_STATE_ACTIVE) {
			pm_policy_state_lock_get(states[i].state, PM_ALL_SUBSTATES);
		}
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	cpu_cmds,
	SHELL_CMD(states,    NULL, "List supported CPU low power states",
		cmd_cpu_states),
	SHELL_CMD(available, NULL, "Show availability/locks for each state",
		cmd_cpu_available),
	SHELL_CMD(lock,      NULL, "Lock state: cpu lock <state> <sub>",
		cmd_cpu_lock),
	SHELL_CMD(unlock,    NULL, "Unlock state: cpu unlock <state> <sub>",
		cmd_cpu_unlock),
	SHELL_CMD(idle,      NULL, "Sleep to let PM work [ms] with specific low power mode",
		cmd_cpu_idle),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(cpu, &cpu_cmds, "CPU core and power state commands", NULL);

SYS_INIT(pm_cpu_lock_soc_low_power, POST_KERNEL, 0);
