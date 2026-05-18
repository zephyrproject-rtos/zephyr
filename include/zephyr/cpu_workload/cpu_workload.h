/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CPU_WORKLOAD_H_
#define ZEPHYR_INCLUDE_CPU_WORKLOAD_H_

#include <stdint.h>

#include <zephyr/sys/util.h>

struct k_work;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CPU Workload
 * @defgroup subsys_cpu_workload CPU Workload
 * @since 4.5
 * @version 0.1.0
 * @ingroup os_services
 * @{
 */

enum cpu_workload_source {
	/** CPU workload signal used per-thread burst profiles. */
	CPU_WORKLOAD_SOURCE_THREAD_BURST_PROFILE = BIT(0),

	/** CPU workload signal came from runnable backlog scanning. */
	CPU_WORKLOAD_SOURCE_READY_BACKLOG = BIT(1),

	/** CPU workload signal came from wakeup/arrival attribution. */
	CPU_WORKLOAD_SOURCE_ARRIVAL = BIT(2),

	/** Arrival was caused by a timeout expiry. */
	CPU_WORKLOAD_SOURCE_ARRIVAL_TIMEOUT = BIT(3),

	/** Arrival was caused by a synchronization object becoming ready. */
	CPU_WORKLOAD_SOURCE_ARRIVAL_SYNC = BIT(4),

	/** Arrival was caused by an explicit thread wakeup. */
	CPU_WORKLOAD_SOURCE_ARRIVAL_EXPLICIT = BIT(5),

	/** CPU workload signal came from a k_work handler profile. */
	CPU_WORKLOAD_SOURCE_WORK_PROFILE = BIT(6),

	/** CPU workload signal came from recent runtime history. */
	CPU_WORKLOAD_SOURCE_RUNTIME_HISTORY = BIT(7),
};

/**
 * @brief Ready backlog workload for one CPU.
 *
 * The backlog reports cycles expected from profiled threads that are runnable
 * at the time of the query.
 */
struct cpu_workload_ready_backlog {
	/** Estimated cycles currently queued as runnable work. */
	uint64_t ready_backlog_cycles;

	/** Bitmask describing which sources contributed to the sample. */
	uint32_t source_mask;

	/** Number of runnable threads considered by the scan. */
	uint16_t runnable_threads;

	/** Number of runnable threads with usable burst profiles. */
	uint16_t profiled_threads;

	/** Confidence in the sample, from 0 to 100. */
	uint8_t confidence;
};

/**
 * @brief CPU wakeup-arrival workload for one CPU.
 *
 * The arrival reports expected cycles from threads that were woken since the
 * previous query. Each attributed arrival contributes the target thread's
 * per-thread burst profile when one is available.
 */
struct cpu_workload_arrival {
	/** Estimated cycles expected from recently attributed arrivals. */
	uint64_t expected_arrival_cycles;

	/** Bitmask describing which sources contributed to the sample. */
	uint32_t source_mask;

	/** Number of arrivals observed since the previous query. */
	uint16_t arrivals;

	/** Number of arrivals with usable burst profiles. */
	uint16_t profiled_arrivals;

	/** Confidence in the sample, from 0 to 100. */
	uint8_t confidence;
};

/**
 * @brief Work item handler workload profile.
 *
 * The profile reports the exponentially weighted moving average (EWMA) of one
 * work item's handler execution cost.
 */
struct cpu_workload_work_profile {
	/** Estimated cycles for the work item's handler. */
	uint32_t handler_cycles;

	/** Bitmask describing which sources contributed to the profile. */
	uint32_t source_mask;

	/** Number of profiled handler executions. */
	uint16_t sample_count;

	/** Confidence in the profile, from 0 to 100. */
	uint8_t confidence;
};

/**
 * @brief Runtime-history workload for one CPU.
 *
 * The history reports CPU runtime-stat deltas observed since the previous
 * history query. The first query establishes the baseline and returns
 * -EAGAIN without reporting a sample.
 */
struct cpu_workload_history {
	/** Non-idle CPU cycles observed since the previous history query. */
	uint64_t non_idle_cycles;

	/** Total CPU runtime-stat cycles elapsed since the previous history query. */
	uint64_t window_cycles;

	/** Runtime-history window duration converted to microseconds. */
	uint32_t window_us;

	/** Non-idle load percentage within the runtime-history window. */
	uint8_t load;

	/** Confidence in the runtime-history sample, from 0 to 100. */
	uint8_t confidence;
};

/**
 * @brief Unified CPU workload estimate for one CPU.
 *
 * The estimate combines recent runtime history with forward-looking workload
 * signals. Runtime history is used as a conservative floor for recurring work,
 * while ready backlog and arrival signals describe work that is already queued
 * or recently arrived.
 */
struct cpu_workload_estimate {
	/** Estimated cycles expected for the next decision window. */
	uint64_t estimated_cycles;

	/** Estimated cycles from currently runnable work. */
	uint64_t ready_backlog_cycles;

	/** Estimated cycles from recently attributed arrivals. */
	uint64_t expected_arrival_cycles;

	/** Recent non-idle runtime-history cycles. */
	uint64_t history_cycles;

	/** Runtime-history window duration in microseconds. */
	uint32_t history_window_us;

	/** Bitmask describing which sources contributed to the estimate. */
	uint32_t source_mask;

	/** Recent runtime-history load percentage. */
	uint8_t history_load;

	/** Confidence in the estimate, from 0 to 100. */
	uint8_t confidence;
};

/**
 * @brief Get CPU ready-backlog workload.
 *
 * @param cpu_id The ID of the CPU for which to get the ready backlog.
 * @param backlog Pointer to the output ready backlog.
 *
 * @retval 0 If the ready backlog was written.
 * @retval -EINVAL If @p cpu_id or @p backlog is invalid.
 * @retval -ENOTSUP If ready-backlog sampling is not enabled.
 */
int cpu_workload_ready_backlog_get(int cpu_id, struct cpu_workload_ready_backlog *backlog);

/**
 * @brief Get CPU wakeup-arrival workload.
 *
 * @param cpu_id The ID of the CPU for which to get the arrival workload.
 * @param arrival Pointer to the output arrival workload.
 *
 * @retval 0 If the arrival workload was written.
 * @retval -EINVAL If @p cpu_id or @p arrival is invalid.
 * @retval -ENOTSUP If arrival attribution is not enabled.
 */
int cpu_workload_arrival_get(int cpu_id, struct cpu_workload_arrival *arrival);

/**
 * @brief Get a work item handler workload profile.
 *
 * @param work Work item.
 * @param profile Pointer to the output profile.
 *
 * @retval 0 If a profile was written.
 * @retval -EINVAL If @p work or @p profile is invalid.
 * @retval -ENOTSUP If work handler profiling is not enabled.
 */
int cpu_workload_work_get(struct k_work *work, struct cpu_workload_work_profile *profile);

/**
 * @brief Get CPU runtime-history workload.
 *
 * @param cpu_id The ID of the CPU for which to get the runtime history.
 * @param history Pointer to the output runtime history.
 *
 * @retval 0 If the runtime history was written.
 * @retval -EAGAIN If this query only established the history baseline.
 * @retval -EINVAL If @p cpu_id or @p history is invalid.
 * @retval -ENOTSUP If runtime-history sampling is not enabled.
 */
int cpu_workload_history_get(int cpu_id, struct cpu_workload_history *history);

/**
 * @brief Get a unified CPU workload estimate.
 *
 * @param cpu_id The ID of the CPU for which to get the estimate.
 * @param estimate Pointer to the output estimate.
 *
 * @retval 0 If an estimate was written.
 * @retval -EINVAL If @p cpu_id or @p estimate is invalid.
 * @retval -ENOTSUP If unified workload estimation is not enabled.
 */
int cpu_workload_estimate_get(int cpu_id, struct cpu_workload_estimate *estimate);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CPU_WORKLOAD_H_ */
