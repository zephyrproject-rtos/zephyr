/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Default PMU counter groups for AArch64 PMU benchmarks
 *
 * Used by @c pmu_benchmarks (kernel operations) and @c cpu_performance
 * (cache / branch / memory). Event encodings are Arm architectural
 * (PMUv3 @c pmu_evt_t). Other architectures should supply their own header.
 */

#ifndef PMU_BENCHMARK_ARM64_CONFIGS_H_
#define PMU_BENCHMARK_ARM64_CONFIGS_H_

#include <zephyr/pmu.h>

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

/** Thread create/suspend: memory-intensive paths. */
static const struct pmu_counter_config pmu_bench_thread_ops_counters[] = {
	{.event = PMU_EVT_INST_RETIRED, .enabled = true},
	{.event = PMU_EVT_MEM_ACCESS, .enabled = true},
	{.event = PMU_EVT_L1D_CACHE_REFILL, .enabled = true},
	{.event = PMU_EVT_BR_MIS_PRED, .enabled = true},
};

/** Kernel API calls (supervisor-mode fast path; not user-mode syscall trap). */
static const struct pmu_counter_config pmu_bench_kernel_api_counters[] = {
	{.event = PMU_EVT_INST_RETIRED, .enabled = true},
	{.event = PMU_EVT_EXC_TAKEN, .enabled = true},
	{.event = PMU_EVT_EXC_RETURN, .enabled = true},
	{.event = PMU_EVT_CPU_CYCLES, .enabled = true},
};

/** cpu_performance: L1/L2 cache behaviour. */
static const struct pmu_counter_config pmu_perf_cache_counters[] = {
	{.event = PMU_EVT_L1D_CACHE, .enabled = true},
	{.event = PMU_EVT_L1D_CACHE_REFILL, .enabled = true},
	{.event = PMU_EVT_L2D_CACHE, .enabled = true},
	{.event = PMU_EVT_L2D_CACHE_REFILL, .enabled = true},
};

/** cpu_performance: branch prediction. */
static const struct pmu_counter_config pmu_perf_branch_counters[] = {
	{.event = PMU_EVT_BR_PRED, .enabled = true},
	{.event = PMU_EVT_BR_MIS_PRED, .enabled = true},
	{.event = PMU_EVT_INST_RETIRED, .enabled = true},
	{.event = PMU_EVT_INST_SPEC, .enabled = true},
};

/** cpu_performance: memory access / bus / IPC. */
static const struct pmu_counter_config pmu_perf_memory_counters[] = {
	{.event = PMU_EVT_MEM_ACCESS, .enabled = true},
	{.event = PMU_EVT_BUS_ACCESS, .enabled = true},
	{.event = PMU_EVT_INST_RETIRED, .enabled = true},
	{.event = PMU_EVT_CPU_CYCLES, .enabled = true},
};

#else /* CONFIG_ARM64 */

#error "pmu_benchmark_arm64_configs.h requires CONFIG_ARM64 (or add an arch-specific config header)"

#endif /* CONFIG_ARM64 */

#endif /* PMU_BENCHMARK_ARM64_CONFIGS_H_ */
