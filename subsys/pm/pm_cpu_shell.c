/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/pm/pm.h>
#include <stdlib.h>
#include "pm_cpu_shell.h"

static const struct pm_state_info residency_info[] =
	PM_STATE_INFO_LIST_FROM_DT_CPU(DT_NODELABEL(cpu0));
static size_t residency_info_count = DT_NUM_CPU_POWER_STATES(DT_NODELABEL(cpu0));

/* List to track suspended threads, only resume what suspended threads */
static sys_slist_t suspended_threads;
/* record shell thread */
static const struct k_thread *shell_thread;
static bool pm_cpu_forced = true;

bool pm_cpu_shell_forced_idle(void)
{
	return pm_cpu_forced;
}

static void pm_suspend_threads_cb(const struct k_thread *thread, void *user_data)
{
	char *name;
	struct thread_event *thread_node;

	thread_node = malloc(sizeof(struct thread_event));

	for (thread = _kernel.threads; thread != NULL; thread = thread->next_thread) {
		name = (char *)k_thread_name_get((struct k_thread *)thread);
		if (strcmp(name, "shell_uart")) {
			/* save current shell thread to suspend in the last sequency */
			shell_thread = thread;

			thread_node->thread = thread;
			sys_slist_append(&suspended_threads, &thread_node->node);
		} else if ((strcmp(name, "main") != 0) && (strcmp(name, "idle") != 0)) {
			thread_node->thread = thread;
			sys_slist_append(&suspended_threads, &thread_node->node);
			k_thread_suspend((k_tid_t)thread);
		} else {
			/* skip main and idle thread and free thread_node */
			free(thread_node);
		}
	}
}

static void pm_suspend_threads(void)
{
	sys_slist_init(&suspended_threads);
	k_thread_foreach(pm_suspend_threads_cb, NULL);

	/* suspend shell thread in the last sequence */
	if (shell_thread) {
		k_thread_suspend((k_tid_t)shell_thread);
	}
}

void pm_resume_threads(void)
{
	sys_snode_t *node;
	struct thread_event *thread_node;

	while ((node = sys_slist_get(&suspended_threads)) != NULL) {
		thread_node = CONTAINER_OF(node, struct thread_event, node);
		k_thread_resume((k_tid_t)thread_node->thread);
		free(thread_node);
	}

	/* restore pm_cpu_forced into true that don't allow cpu run into soc level low power states
	 * in cpu idle, only can enter in cpu shell command
	 */
	pm_cpu_forced = true;
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

static bool pm_state_supported(enum pm_state state)
{
	/* sanity check for first */
	if (residency_info_count == 0) {
		return false;
	}

	for (int i = 0; i < residency_info_count; i++) {
		if (residency_info[i].state == state) {
			return true;
		}
	}

	return false;
}

static int cmd_cpu_lps_info(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* sanity check for first */
	if (residency_info_count == 0) {
		return -EINVAL;
	}

	shell_print(sh, "Supported Low Power States:");

	for (int i = 0; i < residency_info_count; i++) {
		shell_print(sh,
		"  - State: %s, Substate: %d, Residency: %dus, Latency: %dus, PM Device Disabled: %s",
			pm_state_to_str(residency_info[i].state),
			residency_info[i].substate_id,
			residency_info[i].min_residency_us,
			residency_info[i].exit_latency_us,
			residency_info[i].pm_device_disabled ? "Yes" : "No");
	}

	return 0;
}

static int cmd_cpu_lps(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(sh, "Usage: cpu lps <state>");
		return -EINVAL;
	}

	const char *state_str = argv[1];
	const struct pm_state_info *info;
	enum pm_state state;

	if (strcmp(state_str, "runtime-idle") == 0) {
		state = PM_STATE_RUNTIME_IDLE;
	} else if (strcmp(state_str, "suspend-to-idle") == 0) {
		state = PM_STATE_SUSPEND_TO_IDLE;
	} else if (strcmp(state_str, "standby") == 0) {
		state = PM_STATE_STANDBY;
	} else if (strcmp(state_str, "soft-off") == 0) {
		state = PM_STATE_SOFT_OFF;
	} else if (strcmp(state_str, "suspend-to-ram") == 0) {
		state = PM_STATE_SUSPEND_TO_RAM;
	} else {
		shell_error(sh, "Unsupported state: %s", state_str);
		return -EINVAL;
	}

	if (!pm_state_supported(state)) {
		shell_error(sh, "The SoC does not support low power state: %s", state_str);
		return -EINVAL;
	}

	pm_cpu_forced = false;
	info = pm_state_get(0, state, 0);
	if (!info) {
		shell_error(sh, "Failed to get state info for: %s", state_str);
		return -EINVAL;
	}
	pm_state_force(0, info);
	/* suspend user thread to let cpu into idle task context */
	pm_suspend_threads();

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	cpu_cmds,
	SHELL_CMD(lps_info, NULL,
		"List supported low power states info by 'cpu lps' command", cmd_cpu_lps_info),
	SHELL_CMD(lps, NULL,
		"Enter specific low power state by cpu lps specific_low_power_mode'", cmd_cpu_lps),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(cpu, &cpu_cmds, "CPU core and power state commands", NULL);
