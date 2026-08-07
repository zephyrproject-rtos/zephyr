/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief PMU helpers for benchmark applications (not kernel primitives)
 */

#ifndef PMU_HELPERS_H_
#define PMU_HELPERS_H_

#include <zephyr/arch/pmu.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate @a pmu_init() result and sanity-check counters.
 *
 * On success, runs a short workload and requires the cycle counter and event
 * counter 0 (@c PMU_EVT_INST_RETIRED) to advance. If either stays zero (common
 * under QEMU: PMU probes but event counters do not count), returns @c false
 * without failing the suite.
 *
 * @param pmu_init_ret return value from @ref pmu_init()
 * @return @c true if PMU benchmarks should run, @c false if they should all skip
 */
bool pmu_pmuv3_bootstrap(int pmu_init_ret);

/**
 * @brief True only after @ref pmu_pmuv3_bootstrap succeeded on this CPU.
 *
 * Use at the start of each benchmark so partial PMUs (e.g. QEMU) skip even when
 * @ref pmu_num_counters() is non-zero.
 */
bool pmu_ready(void);

/**
 * @brief Configure multiple counters from @a configs and enable those marked enabled.
 *
 * Disables all counters first, then applies @ref pmu_counter_config for each index
 * in [0, @a num_configs) and optionally enables per-entry.
 *
 * @param configs Array of counter/event pairs (index i configures counter i).
 * @param num_configs Number of valid entries in @a configs; must not exceed
 *                    @ref pmu_num_counters().
 *
 * @return 0 on success, or negative errno (e.g. -ENODEV, -EINVAL)
 */
int pmu_configure_counters(const struct pmu_counter_config *configs, uint32_t num_configs);

/**
 * @brief Copy @a src into @a counter_configs and call @ref pmu_configure_counters.
 *
 * @return true on success, false if @a n exceeds @ref PMU_MAX_COUNTERS or
 *         configuration fails
 */
static inline bool pmu_apply_counter_group(struct pmu_counter_config *counter_configs,
					   const struct pmu_counter_config *src, size_t n)
{
	if (n > PMU_MAX_COUNTERS) {
		return false;
	}

	memcpy(counter_configs, src, n * sizeof(struct pmu_counter_config));
	if (pmu_configure_counters(counter_configs, (uint32_t)n) != 0) {
		return false;
	}

	return true;
}

/**
 * @brief Calibrated CPU MHz from @ref pmu_init (benchmark / diag only).
 *
 * Implemented for AArch64 PMU; returns 0 when @c CONFIG_ARM64_PMUV3 is off.
 */
static inline uint32_t pmu_cpu_freq_mhz(void)
{
#if defined(CONFIG_ARM64) && defined(CONFIG_ARM64_PMUV3)
	return arch_pmu_cpu_freq_mhz();
#else
	return 0U;
#endif
}

/**
 * @brief Human-readable PMU event name for logging (benchmark / diag only).
 */
static inline const char *pmu_event_name(uint32_t event)
{
#if defined(CONFIG_ARM64) && defined(CONFIG_ARM64_PMUV3)
	return arch_pmu_event_name(event);
#else
	return "UNKNOWN";
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* PMU_HELPERS_H_ */
