/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_CPU_FREQ_POLICY_H_
#define ZEPHYR_SUBSYS_CPU_FREQ_POLICY_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CPU Frequency Policy Interface - to be implemented by each policy.
 */

#include <zephyr/types.h>
#include <zephyr/cpu_freq/pstate.h>

/**
 * @brief Get the next P-state from CPU Frequency Policy
 *
 * This function is implemented by a CPU Freq policy selected via Kconfig,
 * for example 'CPU_FREQ_POLICY_ON_DEMAND'.
 *
 * @param pstate Pointer to the P-state struct where the next P-state is returned.
 *
 * @retval 0 In case of success, nonzero in case of failure.
 */
int cpu_freq_policy_select_pstate(const struct pstate **pstate);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_CPU_FREQ_POLICY_H_ */
