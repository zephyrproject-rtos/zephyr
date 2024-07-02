/*
 * Copyright 2023 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _INSTRUMENTATION_TIMESTAMP_H
#define _INSTRUMENTATION_TIMESTAMP_H

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

#endif /* _INSTRUMENTATION_TIMESTAMP_H */
