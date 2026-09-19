/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_PROFILING_PMU_SAMPLING_PMU_SAMPLING_PRIV_H_
#define SUBSYS_PROFILING_PMU_SAMPLING_PMU_SAMPLING_PRIV_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

void z_pmu_sampling_on_overflow(uintptr_t pc);
bool z_pmu_sampling_is_active(void);
int z_arm64_pmu_sampling_irq_init(void);
void z_arm64_pmu_sampling_set_period(uint32_t period_events);

/*
 * Called on each secondary CPU after it is brought up (SMP only).
 * Programs the PMU counter and enables the overflow IRQ on that CPU so
 * all cores participate in sampling.
 */
void z_arm64_pmu_sampling_cpu_start(uint32_t event, uint32_t period_events);

/*
 * Called on each secondary CPU to stop PMU sampling (SMP only).
 * Disables counter + overflow interrupt on the local CPU.
 */
void z_arm64_pmu_sampling_cpu_stop(void);

#endif /* SUBSYS_PROFILING_PMU_SAMPLING_PMU_SAMPLING_PRIV_H_ */
