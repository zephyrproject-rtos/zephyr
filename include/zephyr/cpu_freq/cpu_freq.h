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
 * @brief Put processor into the given performance state.
 *
 * To be implemented by the SoC. This API abstracts the hardware and SoC specific
 * calls required to change the performance state of the processor.
 *
 * @param state Performance state.
 */
int32_t cpu_freq_pstate_set(struct pstate state);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_CPU_FREQ_H_ */
