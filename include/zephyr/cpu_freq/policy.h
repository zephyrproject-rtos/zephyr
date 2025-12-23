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
 * @brief CPU Frequency Scaling Policy
 * @defgroup subsys_cpu_freq_policy CPU Frequency Policy
 * @since 4.3
 * @version 0.1.0
 * @ingroup subsys_cpu_freq
 * @{
 */

/**
 * @brief Get the next P-state from CPU frequency scaling policy
 *
 * This function is implemented by a CPU frequency scaling policy selected via
 * Kconfig. The caller must ensure that the current CPU does not change. If
 * called from an ISR or a single CPU system, this restriction is automatically
 * met. If called from a thread on an SMP system, either interrupts or the
 * scheduler must be disabled to ensure the current CPU does not change.
 *
 * @param pstate_out Pointer to the P-state struct where the next P-state is returned.
 *
 * @return 0 in case of success, nonzero in case of failure.
 */
int cpu_freq_policy_select_pstate(const struct pstate **pstate_out);

/**
 * @brief Reset data structures used by CPU frequency scaling policy
 *
 * This function is implemented by a CPU frequency scaling policy selected via
 * Kconfig. It resets any data structured needed by the policy. It is
 * automatically invoked by the CPU frequency scaling subsystem and should not
 * be called directly by application code.
 */
void cpu_freq_policy_reset(void);

/**
 * @brief Set the CPU frequency scaling P-state according to policy
 *
 * This function is implemented by a CPU frequency scaling policy selected via
 * Kconfig. It accepts a P-state previously obtained by a call to
 * :c:func:`cpu_freq_policy_select_pstate` and attempts to apply it according
 * to the policy. The policy may choose to apply the selected P-state, or
 * apply a different P-state, or ignore the request altogether. This function
 * is automatically invoked by the CPU frequency scaling subsystem and should
 * not be called directly by application code.
 *
 * @param state Pointer to a P-state
 *
 * @return Pointer to the applied P-state, or NULL if nothing applied
 */
const struct pstate *cpu_freq_policy_pstate_set(const struct pstate *state);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_CPU_FREQ_POLICY_H_ */
