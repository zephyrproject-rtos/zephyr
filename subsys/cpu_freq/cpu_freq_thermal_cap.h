/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_CPU_FREQ_THERMAL_CAP_H_
#define ZEPHYR_SUBSYS_CPU_FREQ_THERMAL_CAP_H_

#include <zephyr/cpu_freq/pstate.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Apply the cached thermal cap to a requested P-state.
 *
 * The CPU frequency policy selects the requested P-state first. This helper
 * constrains that request to the highest-performance P-state currently allowed
 * by the thermal cap.
 *
 * @param state Requested P-state selected by the active CPU frequency policy.
 *
 * @return P-state to pass to the CPU frequency driver, or NULL when @p state is NULL.
 */
const struct pstate *cpu_freq_thermal_cap_apply(const struct pstate *state);

/**
 * @brief Sample the configured thermal sensor and update the cached cap.
 *
 * This function may call blocking sensor APIs and must not be called from the
 * CPU frequency timer path. Repeated sample failures may apply the configured
 * fail-safe P-state.
 *
 * @retval 0 Thermal cap state updated successfully.
 * @retval -errno Thermal sensor sampling failed.
 */
int cpu_freq_thermal_cap_sample(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_CPU_FREQ_THERMAL_CAP_H_ */
