/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Default PMU counter groups for AArch64 PMU benchmarks
 *
 * Used by the unified @c pmu application (kernel, cache, branch).
 * Event encodings are Arm architectural
 * (PMUv3 @c pmu_evt_t). Other architectures should supply their own header.
 */

#ifndef PMU_ARM64_CONFIGS_H_
#define PMU_ARM64_CONFIGS_H_

#include <zephyr/arch/pmu.h>

#if defined(CONFIG_ARM64)

/** Context switch: instruction flow + cache pressure from stack switching. */
static const struct pmu_counter_config pmu_bench_ctx_switch_counters[] = {
	{.event = PMU_EVT_INST_RETIRED, .enabled = true},
	{.event = PMU_EVT_EXC_TAKEN, .enabled = true},
	{.event = PMU_EVT_L1D_CACHE_REFILL, .enabled = true},
	{.event = PMU_EVT_BR_MIS_PRED, .enabled = true},
};

/** irq_offload: exception entry/return path. */
static const struct pmu_counter_config pmu_bench_irq_offload_counters[] = {
	{.event = PMU_EVT_INST_RETIRED, .enabled = true},
	{.event = PMU_EVT_EXC_TAKEN, .enabled = true},
	{.event = PMU_EVT_EXC_RETURN, .enabled = true},
	{.event = PMU_EVT_BR_MIS_PRED, .enabled = true},
};

/** Kernel API calls (supervisor-mode fast path; not user-mode syscall trap). */
static const struct pmu_counter_config pmu_bench_kernel_api_counters[] = {
	{.event = PMU_EVT_INST_RETIRED, .enabled = true},
	{.event = PMU_EVT_EXC_TAKEN, .enabled = true},
	{.event = PMU_EVT_EXC_RETURN, .enabled = true},
	{.event = PMU_EVT_CPU_CYCLES, .enabled = true},
};

/** L1/L2 cache behaviour. */
static const struct pmu_counter_config pmu_perf_cache_counters[] = {
	{.event = PMU_EVT_L1D_CACHE, .enabled = true},
	{.event = PMU_EVT_L1D_CACHE_REFILL, .enabled = true},
	{.event = PMU_EVT_L2D_CACHE, .enabled = true},
	{.event = PMU_EVT_L2D_CACHE_REFILL, .enabled = true},
};

/** Branch prediction. */
static const struct pmu_counter_config pmu_perf_branch_counters[] = {
	{.event = PMU_EVT_BR_PRED, .enabled = true},
	{.event = PMU_EVT_BR_MIS_PRED, .enabled = true},
	{.event = PMU_EVT_INST_RETIRED, .enabled = true},
	{.event = PMU_EVT_INST_SPEC, .enabled = true},
};

#else /* CONFIG_ARM64 */

#error "pmu_arm64_configs.h requires CONFIG_ARM64 (or add an arch-specific config header)"

#endif /* CONFIG_ARM64 */

#endif /* PMU_ARM64_CONFIGS_H_ */
