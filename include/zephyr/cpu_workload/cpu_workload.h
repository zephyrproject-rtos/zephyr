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
 * @brief CPU Workload Estimator
 * @defgroup subsys_cpu_workload CPU Workload Estimator
 * @since 4.4
 * @version 0.1.0
 * @ingroup os_services
 * @{
 */

/** CPU workload signal came from runnable backlog scanning. */
#define CPU_WORKLOAD_SOURCE_READY_BACKLOG BIT(0)

/** CPU workload signal used per-thread burst profiles. */
#define CPU_WORKLOAD_SOURCE_THREAD_BURST_PROFILE BIT(1)

/**
 * @brief Ready backlog workload sample for one CPU.
 *
 * The sample reports cycles expected from threads that are runnable at the
 * time of the query. Threads with sufficiently warm per-thread burst profiles
 * contribute their average burst cycles. Threads without a usable profile may
 * contribute a configured fallback value.
 */
struct cpu_workload_ready_backlog_sample {
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
 * @brief Get a CPU ready-backlog workload sample.
 *
 * @param cpu_id The ID of the CPU for which to get the backlog sample.
 * @param sample Pointer to the output sample.
 *
 * @retval 0 If a sample was written.
 * @retval -EINVAL If @p cpu_id or @p sample is invalid.
 * @retval -ENOTSUP If ready-backlog sampling is not enabled.
 */
int cpu_workload_ready_backlog_get(int cpu_id,
					   struct cpu_workload_ready_backlog_sample *sample);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CPU_WORKLOAD_H_ */