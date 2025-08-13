/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPU_LOAD_H_
#define CPU_LOAD_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the CPU load as a percentage.
 *
 * Return the percent that the CPU has spent in the active (non-idle) state
 * between calls to this function.
 *
 * @param load Pointer to the integer where the CPU load as a percent (0-100) is returned.
 * @param cpu_id The ID of the CPU for which to get the load.
 *
 * @retval 0 In case of success, nonzero in case of failure.
 */
int cpu_load_get(uint32_t *load, int cpu_id);

#ifdef __cplusplus
}
#endif

#endif /* CPU_LOAD_H_ */
