/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_CPU_LOAD_H_
#define ZEPHYR_SUBSYS_CPU_LOAD_H_

#include <stdint.h>

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CPU Load Monitoring
 * @defgroup subsys_cpu_load CPU Load
 * @since 4.3
 * @version 0.1.0
 * @ingroup os_services
 *
 * CPU load and runtime-history sample APIs. New Kconfig users should enable
 * :kconfig:option:`CONFIG_CPU_LOAD_METRIC`.
 * @{
 */

/** CPU load sample came from runtime-history counters. */
#define CPU_LOAD_SAMPLE_SOURCE_RUNTIME_HISTORY BIT(0)

/**
 * @brief CPU load sample for one recent accounting window.
 *
 * The sample reports non-idle CPU cycles observed since the previous
 * successful sample for @p cpu_id. Users may treat @p non_idle_cycles
 * as a first-order workload estimate for the next comparable window,
 * with @p confidence indicating how much history has been collected.
 */
struct cpu_load_sample {
	/** Non-idle cycles in the sampled window. */
	uint64_t non_idle_cycles;

	/** Sample window duration in microseconds. */
	uint32_t window_us;

	/** Bitmask describing which sources contributed to the sample. */
	uint32_t source_mask;

	/** Percentage of the sampled window spent executing non-idle work. */
	uint8_t load;

	/** Confidence in the sample, from 0 to 100. */
	uint8_t confidence;
};

/**
 * @brief Get a CPU load sample with cycle and confidence metadata.
 *
 * @param cpu_id The ID of the CPU for which to get the sample.
 * @param sample Pointer to the output sample.
 *
 * @retval 0 If a sample was written.
 * @retval -EINVAL If @p cpu_id or @p sample is invalid.
 * @retval -EAGAIN If the metric has not accumulated a usable window yet.
 * @retval -errno Other errors returned by runtime statistics collection.
 */
int cpu_load_sample_get(int cpu_id, struct cpu_load_sample *sample);

/**
 * @brief Get the CPU load as a percentage.
 *
 * Return the percent that the CPU has spent in the active (non-idle) state between calls to this
 * function.
 *
 * @param cpu_id The ID of the CPU for which to get the load.
 *
 * @return CPU load in percent (0-100) in case of success
 * @retval -errno code in case of failure.
 *
 */
int cpu_load_metric_get(int cpu_id);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_CPU_LOAD_H_ */
