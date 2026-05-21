/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CPU_WORKLOAD_H_
#define ZEPHYR_INCLUDE_CPU_WORKLOAD_H_

#include <stdint.h>

#include <zephyr/sys/util.h>

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
	/** Recent runtime history contributed to this workload. */
	CPU_WORKLOAD_SOURCE_RUNTIME_HISTORY = BIT(0),
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
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CPU_WORKLOAD_H_ */
