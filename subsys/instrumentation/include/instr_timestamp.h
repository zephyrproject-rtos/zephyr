/*
 * Copyright 2023 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INSTRUMENTATION_TIMESTAMP_H_
#define ZEPHYR_INCLUDE_INSTRUMENTATION_TIMESTAMP_H_

#include <inttypes.h>

#include <zephyr/timing/timing.h>

/**
 * @brief Initialize timestamp
 *
 */
int instr_timestamp_init(void);

/**
 * @brief Get current timestamp in nanoseconds
 *
 */
uint64_t instr_timestamp_ns(void);

#endif /* ZEPHYR_INCLUDE_INSTRUMENTATION_TIMESTAMP_H_ */
