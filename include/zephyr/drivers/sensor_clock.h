/*
 * Copyright (c) 2024 Cienet
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_CLOCK_H_
#define ZEPHYR_DRIVERS_SENSOR_CLOCK_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Retrieve the current sensor clock cycles.
 *
 * This function obtains the current cycle count from the selected
 * sensor clock source. The clock source may be the system clock or
 * an external clock, depending on the configuration.
 *
 * @param[out] cycles Pointer to a 64-bit unsigned integer where the
 *                    current clock cycle count will be stored.
 *
 * @return 0 on success, or an error code on failure.
 */
int sensor_clock_get_cycles(uint64_t *cycles);

/**
 * @brief Convert clock cycles to nanoseconds.
 *
 * This function converts clock cycles into nanoseconds based on the
 * clock frequency.
 *
 * @param cycles Clock cycles to convert.
 * @return Time in nanoseconds.
 */
uint64_t sensor_clock_cycles_to_ns(uint64_t cycles);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_CLOCK_H_ */
