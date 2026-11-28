/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Performance event provider interface.
 */

#ifndef ZEPHYR_INCLUDE_PROFILING_PERF_PROVIDER_H_
#define ZEPHYR_INCLUDE_PROFILING_PERF_PROVIDER_H_

#include <stdint.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup profiling_perf_providers Performance event providers
 * @ingroup profiling_perf
 * @since 4.6
 * @version 0.1.0
 *
 * @note This API is experimental and may change in future releases.
 *
 * @{
 */

/**
 * @brief Report an event supported by a provider.
 *
 * The event name and description are valid for the duration of the callback.
 *
 * @param event_name NUL-terminated event name without the provider prefix.
 * @param description NUL-terminated event description, or NULL.
 * @param user_data Opaque pointer passed to list(), forwarded unchanged.
 *
 * @retval 0 Continue enumeration.
 * @return A nonzero value to stop enumeration. The provider returns this value unchanged.
 */
typedef int (*perf_event_list_cb_t)(const char *event_name, const char *description,
				    void *user_data);

/**
 * @brief Performance event provider operations.
 *
 * Event IDs are plain provider-local values in the range 0 to UINT16_MAX.
 * The core owns token encoding and passes the ID returned by lookup() unchanged
 * to prepare(), start(), and stop(). Providers do not access token fields.
 *
 * The core invokes operations synchronously in the requesting supervisor thread,
 * never in interrupt context. A mutex shared by all providers is held throughout
 * each operation and its enumeration callbacks, serializing calls from different
 * threads. Successive calls may come from different threads or CPUs.
 *
 * Providers may delegate work to another thread, but must wait for that work to
 * complete before returning. Enumeration callbacks must run synchronously in the
 * thread that called list().
 */
struct perf_event_provider_api {
	/**
	 * @brief Enumerate provider-local event names.
	 *
	 * Stop at the first nonzero callback return value and return it unchanged.
	 *
	 * @param context Provider context supplied at registration, or NULL.
	 * @param callback Function to call for each supported event.
	 * @param user_data Opaque pointer to pass unchanged to @p callback.
	 *
	 * @retval 0 Enumeration completed successfully.
	 * @return A nonzero value returned by @p callback, or a negative errno value
	 *         if enumeration fails.
	 */
	int (*list)(void *context, perf_event_list_cb_t callback, void *user_data);
	/**
	 * @brief Resolve a provider-local event name.
	 *
	 * @param context Provider context supplied at registration, or NULL.
	 * @param event_name NUL-terminated event name without the provider prefix.
	 * @param[out] event_id Storage for the provider-local ID. Valid on success.
	 *
	 * @retval 0 The event was found and @p event_id was set.
	 * @retval -ENOENT The named event was not found.
	 * @return Another negative errno value on failure.
	 */
	int (*lookup)(void *context, const char *event_name, uint64_t *event_id);
	/**
	 * @brief Validate and configure an event without starting it.
	 *
	 * May be NULL. The core prepares all selected events before starting any.
	 * On success, stop() releases any prepared state, including when another
	 * event fails preparation and this event never reaches start(). A failed
	 * prepare() must release its own partial state before returning.
	 *
	 * @param context Provider context supplied at registration, or NULL.
	 * @param event_id Provider-local event ID returned by lookup().
	 *
	 * @retval 0 The event was prepared successfully.
	 * @return A negative errno value on failure.
	 */
	int (*prepare)(void *context, uint64_t event_id);
	/**
	 * @brief Start an event and return its initial counter value.
	 *
	 * On failure, undo any partial activation before returning. Successfully
	 * prepared state is subsequently released by stop(). Without prepare(),
	 * a failed start() must release all resources itself.
	 *
	 * @param context Provider context supplied at registration, or NULL.
	 * @param event_id Provider-local event ID returned by lookup().
	 * @param[out] baseline Storage for the initial counter value. Valid on success.
	 *
	 * @retval 0 The event was started and @p baseline was set.
	 * @return A negative errno value on failure.
	 */
	int (*start)(void *context, uint64_t event_id, uint64_t *baseline);
	/**
	 * @brief Stop an event and release its prepared state.
	 *
	 * Called exactly once for each successfully prepared or started event.
	 * Must also accept a prepared event whose start() was never called or
	 * failed. Set final_count to zero when no measurement was started.
	 * During startup rollback the core discards final_count and the return
	 * value. Otherwise, store the final counter value and return its status.
	 * Release resources even when the final snapshot cannot be obtained.
	 *
	 * @param context Provider context supplied at registration, or NULL.
	 * @param event_id Provider-local event ID returned by lookup().
	 * @param[out] final_count Storage for the final counter value, or zero if counting
	 *                         never started. Valid on success.
	 *
	 * @retval 0 Event resources were released and @p final_count is valid.
	 * @return A negative errno value on failure, stored in the event's status
	 *         during a normal stop.
	 */
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

#endif /* ZEPHYR_INCLUDE_PROFILING_PERF_PROVIDER_H_ */
