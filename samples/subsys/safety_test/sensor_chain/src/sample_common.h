/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SAFETY_TEST_SENSOR_CHAIN_SAMPLE_COMMON_H_
#define SAFETY_TEST_SENSOR_CHAIN_SAMPLE_COMMON_H_

#include <stdbool.h>
#include <stdint.h>

/** True once any test has failed. Set by the failure hook, never cleared. */
bool sample_fault_latched(void);

/** Temperature samples held in the scratch block. */
#define SAMPLE_HISTORY_LEN 16U

/** Record a reading, in millidegrees Celsius. */
void sample_history_append(int32_t temp_mc);

/** Readings recorded since the last reset. */
uint32_t sample_history_count(void);

/** Discard recorded readings. */
void sample_history_reset(void);

/** Most recent temperature in millidegrees Celsius, or 0 before the first read. */
int32_t sample_sensor_last_mc(void);

#endif /* SAFETY_TEST_SENSOR_CHAIN_SAMPLE_COMMON_H_ */
