/*
 * SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its
 * SPDX-FileCopyrightText: affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Performance event provider interface.
 */

#ifndef ZEPHYR_SUBSYS_PROFILING_PERF_EVENT_PROVIDER_H_
#define ZEPHYR_SUBSYS_PROFILING_PERF_EVENT_PROVIDER_H_

#include <stdint.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup profiling_perf_providers Performance event providers
 * @ingroup profiling_perf
 * @{
 */

/** Callback used to report an event supported by a provider. */
typedef int (*perf_event_list_cb_t)(const char *event_name, const char *description,
				    void *user_data);

/**
 * Performance event provider operations.
 *
 * Event IDs are plain provider-local values in the range 0 to UINT16_MAX.
 * The core owns token encoding and passes the ID returned by lookup() unchanged
 * to prepare(), start(), and stop(). Providers do not access token fields.
 */
struct perf_event_provider_api {
	/** Enumerate provider-local event names. */
	int (*list)(void *context, perf_event_list_cb_t callback, void *user_data);
	/** Resolve a provider-local event name. */
	int (*lookup)(void *context, const char *event_name, uint64_t *event_id);
	/** Validate and configure an event without starting it. May be NULL. */
	int (*prepare)(void *context, uint64_t event_id);
	/** Start an event and return its initial counter value. */
	int (*start)(void *context, uint64_t event_id, uint64_t *baseline);
	/** Stop an event and return its final counter value. */
	int (*stop)(void *context, uint64_t event_id, uint64_t *final_count);
};

/** Registered performance event provider. */
struct perf_event_provider {
	/** Prefix used in canonical event names. */
	const char *name;
	/** Human-readable provider description. */
	const char *description;
	/** Provider operations. */
	const struct perf_event_provider_api *api;
	/** Context passed to provider operations. */
	void *context;
};

/**
 * @brief Define and register a performance event provider.
 *
 * @param _name Provider name supplied as an unquoted C identifier.
 * @param _description Human-readable provider description.
 * @param _api Pointer to the provider API.
 * @param _context Provider context, or NULL.
 */
#define PERF_EVENT_PROVIDER_DEFINE(_name, _description, _api, _context)                            \
	static const STRUCT_SECTION_ITERABLE(perf_event_provider, perf_event_provider_##_name) = { \
		.name = STRINGIFY(_name), .description = (_description), .api = (_api),            \
				  .context = (_context),                                           \
	}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_PROFILING_PERF_EVENT_PROVIDER_H_ */
