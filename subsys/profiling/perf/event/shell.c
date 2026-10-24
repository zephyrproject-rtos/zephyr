/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "internal.h"

#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/profiling/perf.h>
#include "provider.h"
#include <zephyr/shell/shell.h>

struct perf_shell_selection {
	struct perf_event_handle events[CONFIG_PROFILING_PERF_EVENTS_MAX_EVENTS];
	char names[CONFIG_PROFILING_PERF_EVENTS_MAX_EVENTS][CONFIG_PROFILING_PERF_EVENTS_NAME_MAX];
	size_t count;
};

struct perf_shell_list_context {
	const struct shell *sh;
	const char *provider_name;
};

static K_MUTEX_DEFINE(perf_shell_lock);
static struct perf_shell_selection current_selection;
static struct perf_shell_selection retained_selection;
static struct perf_stat_config current_config;
static bool shell_session_active;
static bool retained_result_valid;

static void perf_shell_result_print_locked(const struct shell *sh)
{
	shell_print(sh, "Perf buffer type: stat, version: 1");
	for (size_t i = 0U; i < retained_selection.count; i++) {
		const struct perf_event_handle *event = &retained_selection.events[i];

		if (event->status == 0) {
			shell_print(sh, "%20" PRIu64 "  %s", event->final_count - event->baseline,
				    retained_selection.names[i]);
		} else {
			shell_print(sh, "%20s  %s (%d)", "error", retained_selection.names[i],
				    event->status);
		}
	}
}

bool z_perf_stat_shell_has_result(void)
{
	bool has_result;

	k_mutex_lock(&perf_shell_lock, K_FOREVER);
	has_result = retained_result_valid;
	k_mutex_unlock(&perf_shell_lock);

	return has_result;
}

int z_perf_stat_shell_print_result(const struct shell *sh, bool clear)
{
	int ret = 0;

	if (sh == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&perf_shell_lock, K_FOREVER);
	if (!retained_result_valid) {
		ret = -ENOENT;
		goto out;
	}

	perf_shell_result_print_locked(sh);
	if (clear) {
		retained_result_valid = false;
		retained_selection.count = 0U;
	}

out:
	k_mutex_unlock(&perf_shell_lock);
	return ret;
}

void z_perf_stat_shell_clear_result(void)
{
	k_mutex_lock(&perf_shell_lock, K_FOREVER);
	retained_result_valid = false;
	retained_selection.count = 0U;
	k_mutex_unlock(&perf_shell_lock);
}

static int perf_shell_list_event(const char *event_name, const char *description, void *user_data)
{
	struct perf_shell_list_context *context = user_data;

	if (event_name == NULL) {
		return -EINVAL;
	}

	shell_print(context->sh, "%s.%s : %s", context->provider_name, event_name,
		    description == NULL ? "-" : description);
	return 0;
}

static int perf_shell_list_provider(const struct perf_event_provider *provider, void *user_data)
{
	const struct shell *sh = user_data;

	shell_print(sh, "%s : %s", provider->name,
		    provider->description == NULL ? "-" : provider->description);
	return 0;
}

static int cmd_perf_list(const struct shell *sh, size_t argc, char **argv)
{
	struct perf_shell_list_context context;
	int ret;

	if (argc == 1U) {
		return z_perf_provider_list(perf_shell_list_provider, (void *)sh);
	}

	context.sh = sh;
	context.provider_name = argv[1];

	ret = z_perf_provider_event_list(argv[1], perf_shell_list_event, &context);
	if (ret == -ENOENT) {
		shell_error(sh, "Provider not found: %s", argv[1]);
	}

	return ret;
}

static int cmd_perf_stat_start(const struct shell *sh, size_t argc, char **argv)
{
	size_t event_count;
	int ret;

	if (argc < 3U || ((argc - 1U) % 2U) != 0U) {
		return -EINVAL;
	}
	event_count = (argc - 1U) / 2U;
	if (event_count > ARRAY_SIZE(current_selection.events)) {
		return -ENOSPC;
	}

	k_mutex_lock(&perf_shell_lock, K_FOREVER);
	if (shell_session_active || z_perf_session_is_active()) {
		shell_warn(sh, "Perf is running");
		ret = -EBUSY;
		goto out;
	}

	for (size_t i = 0U; i < event_count; i++) {
		const char *option = argv[1U + (2U * i)];
		const char *name = argv[2U + (2U * i)];
		size_t name_len;

		if (strcmp(option, "-e") != 0) {
			ret = -EINVAL;
			goto out;
		}
		name_len = strlen(name);
		if (name_len >= sizeof(current_selection.names[i])) {
			ret = -ENAMETOOLONG;
			goto out;
		}

		ret = perf_event_lookup(name, &current_selection.events[i]);
		if (ret != 0) {
			shell_error(sh, "Event not found: %s", name);
			goto out;
		}
		memcpy(current_selection.names[i], name, name_len + 1U);
	}

	current_selection.count = event_count;
	current_config.events = current_selection.events;
	current_config.num_events = current_selection.count;
	ret = perf_stat_start(&current_config);
	if (ret == 0) {
		shell_session_active = true;
		retained_result_valid = false;
		retained_selection.count = 0U;
		shell_print(sh, "Perf stat started");
	} else if (ret == -EBUSY) {
		shell_warn(sh, "Perf is running");
	}

out:
	k_mutex_unlock(&perf_shell_lock);
	return ret;
}

static int cmd_perf_stat_stop(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	k_mutex_lock(&perf_shell_lock, K_FOREVER);
	if (!shell_session_active) {
		ret = -EINVAL;
		goto out;
	}

	ret = perf_stat_stop(&current_config);
	if (ret != 0) {
		goto out;
	}

	shell_session_active = false;
	retained_selection = current_selection;
	retained_result_valid = true;
	perf_shell_result_print_locked(sh);

out:
	k_mutex_unlock(&perf_shell_lock);
	return ret;
}

#if !defined(CONFIG_PROFILING_PERF)
static int cmd_perf_print(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (z_perf_session_is_active()) {
		shell_warn(sh, "Perf is running");
		return -EINPROGRESS;
	}
	if (!z_perf_stat_shell_has_result()) {
		shell_print(sh, "Perf buffer empty");
		return 0;
	}

	return z_perf_stat_shell_print_result(sh, true);
}

static int cmd_perf_clear(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (z_perf_session_is_active()) {
		shell_warn(sh, "Perf is running");
		return -EINPROGRESS;
	}

	z_perf_stat_shell_clear_result();
	shell_print(sh, "Perf buffer cleared");
	return 0;
}

static int cmd_perf_info(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (z_perf_session_is_active()) {
		shell_print(sh, "Perf is running");
	}
	shell_print(sh, "Perf buffer: %s", z_perf_stat_shell_has_result() ? "stat" : "empty");
	return 0;
}
#endif

#define PERF_STAT_START_OPTIONAL_ARGS (2 * (CONFIG_PROFILING_PERF_EVENTS_MAX_EVENTS - 1))

SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_perf_stat,
			       SHELL_CMD_ARG(start, NULL,
					     "Start a stat session: start -e <provider.event> ...",
					     cmd_perf_stat_start, 3, PERF_STAT_START_OPTIONAL_ARGS),
			       SHELL_CMD_ARG(stop, NULL, "Stop the active stat session",
					     cmd_perf_stat_stop, 1, 0),
			       SHELL_SUBCMD_SET_END);

#if !defined(CONFIG_PROFILING_PERF)
SHELL_SUBCMD_SET_CREATE(m_sub_perf, (perf));
SHELL_SUBCMD_ADD((perf), printbuf, NULL, "Print the perf buffer", cmd_perf_print, 0, 0);
SHELL_SUBCMD_ADD((perf), clear, NULL, "Clear the perf buffer", cmd_perf_clear, 0, 0);
SHELL_SUBCMD_ADD((perf), info, NULL, "Print the perf info", cmd_perf_info, 0, 0);
SHELL_CMD_ARG_REGISTER(perf, &m_sub_perf, "Lightweight profiler", NULL, 0, 0);
#endif

SHELL_SUBCMD_ADD((perf), list, NULL, "List providers or provider events", cmd_perf_list, 1, 1);
SHELL_SUBCMD_ADD((perf), stat, &m_sub_perf_stat, "Control counter sessions", NULL, 0, 0);
