/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Performance counter sessions.
 */

#ifndef ZEPHYR_INCLUDE_PROFILING_PERF_H_
#define ZEPHYR_INCLUDE_PROFILING_PERF_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup profiling_perf Performance counter sessions
 * @ingroup profiling
 * @{
 */

/** Performance event selection and counter snapshots. */
struct perf_event_handle {
	/**
	 * Opaque event identifier initialized by perf_event_lookup().
	 * Applications must not modify it or reuse it across firmware builds.
	 */
	uint64_t token;
	/** Counter value sampled when the measurement session starts. */
	uint64_t baseline;
	/** Counter value sampled when the measurement session stops. */
	uint64_t final_count;
	/** Event-specific stop status. Zero means @c final_count is valid. */
	int status;
};

/** Performance counter session configuration. */
struct perf_stat_config {
	/** Event handles initialized by perf_event_lookup(). */
	struct perf_event_handle *events;
	/** Number of elements in @c events. Must be greater than zero. */
	size_t num_events;
};

/**
 * @brief Resolve a performance event name.
 *
 * @param name NUL-terminated canonical provider.event name.
 * @param handle Storage for the initialized event handle.
 *
 * @retval 0 The event was found.
 * @retval -EINVAL An argument is invalid.
 * @retval -ENOENT The named event is unavailable.
 * @retval -ENOSYS The matched provider is not fully implemented.
 * @retval -EPERM The function was called from user mode.
 * @retval -EOVERFLOW The provider identifier cannot be represented.
 * @retval -ERANGE The provider-local event identifier exceeds UINT16_MAX.
 * @retval -EWOULDBLOCK The function was called from interrupt context.
 * @return Another negative errno value reported by the provider.
 */
int perf_event_lookup(const char *name, struct perf_event_handle *handle);

/**
 * @brief Start a performance counter session.
 *
 * The caller shall keep @p config and its event array valid and unchanged
 * until perf_stat_stop() returns.
 *
 * @param config Session configuration containing at least one event.
 *
 * @retval 0 The session was started.
 * @retval -EINVAL The configuration or an event handle is invalid.
 * @retval -EBUSY Another perf session is active.
 * @retval -ENOSPC Too many events were selected or provider resources are exhausted.
 * @retval -EPERM The function was called from user mode.
 * @retval -EWOULDBLOCK The function was called from interrupt context.
 * @return Another negative errno value reported by a provider.
 */
int perf_stat_start(const struct perf_stat_config *config);

/**
 * @brief Stop a performance counter session.
 *
 * Event-specific failures are stored in each handle's @c status field and do
 * not cause this function itself to fail.
 *
 * @param config Configuration used to start the active session.
 *
 * @retval 0 A completed result was produced.
 * @retval -EINVAL The configuration does not identify the active session.
 * @retval -EPERM The function was called from user mode.
 * @retval -EWOULDBLOCK The function was called from interrupt context.
 * @return Another negative errno value if a coherent result could not be produced.
 */
int perf_stat_stop(const struct perf_stat_config *config);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PROFILING_PERF_H_ */
