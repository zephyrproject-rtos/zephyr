/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_CPU_FREQ_H_
#define ZEPHYR_SUBSYS_CPU_FREQ_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <zephyr/cpu_freq/pstate.h>

/**
 * @brief Dynamic CPU Frequency Scaling
 * @defgroup subsys_cpu_freq CPU Frequency (CPUFreq)
 * @since 4.3
 * @version 0.1.0
 * @ingroup os_services
 * @{
 */

/**
 * @brief Request processor set the given performance state.
 *
 * To be implemented by the SoC. This API abstracts the hardware and SoC specific calls required to
 * change the performance state of the processor.
 *
 * @note It is not guaranteed that the performance state will be set immediately, or at all.
 *
 * @param state Pointer to performance state.
 *
 *
 * @retval 0 if request received successfully, -errno in case of failure.
 */
int cpu_freq_pstate_set(const struct pstate *state);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_CPU_FREQ_H_ */
