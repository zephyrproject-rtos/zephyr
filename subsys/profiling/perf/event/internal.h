/*
 * SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its
 * SPDX-FileCopyrightText: affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_PROFILING_PERF_EVENT_INTERNAL_H_
#define ZEPHYR_SUBSYS_PROFILING_PERF_EVENT_INTERNAL_H_

#include <stdbool.h>
#include <errno.h>
#include "provider.h"
#include <zephyr/sys/util.h>

struct shell;

/** Callback used to report a registered performance event provider. */
typedef int (*z_perf_provider_list_cb_t)(const struct perf_event_provider *provider,
					 void *user_data);

enum perf_session_type {
	PERF_SESSION_NONE,
	PERF_SESSION_RECORD,
	PERF_SESSION_STAT,
};

#if defined(CONFIG_PROFILING_PERF_EVENTS)
int z_perf_session_claim(enum perf_session_type type);
void z_perf_session_release(enum perf_session_type type);
bool z_perf_session_is_active(void);
int z_perf_provider_list(z_perf_provider_list_cb_t callback, void *user_data);
int z_perf_provider_event_list(const char *provider_name, perf_event_list_cb_t callback,
			       void *user_data);
#else
static inline int z_perf_session_claim(enum perf_session_type type)
{
	ARG_UNUSED(type);

	return 0;
}

static inline void z_perf_session_release(enum perf_session_type type)
{
	ARG_UNUSED(type);
}

static inline bool z_perf_session_is_active(void)
{
	return false;
}
#endif

#if defined(CONFIG_PROFILING_PERF_EVENTS_SHELL)
/** Return whether a completed stat session result is retained. */
bool z_perf_stat_shell_has_result(void);

/** Print the retained stat result to @p sh and discard it when @p clear is true. */
int z_perf_stat_shell_print_result(const struct shell *sh, bool clear);

/** Discard the retained stat result without affecting an active session. */
void z_perf_stat_shell_clear_result(void);
#else
static inline bool z_perf_stat_shell_has_result(void)
{
	return false;
}

static inline int z_perf_stat_shell_print_result(const struct shell *sh, bool clear)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(clear);

	return -ENOENT;
}

static inline void z_perf_stat_shell_clear_result(void)
{
}
#endif

#endif /* ZEPHYR_SUBSYS_PROFILING_PERF_EVENT_INTERNAL_H_ */
