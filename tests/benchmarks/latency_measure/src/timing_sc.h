/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _LATENCY_MEASURE_TIMING_SC_H
#define _LATENCY_MEASURE_TIMING_SC_H

#include <zephyr/kernel.h>
#include <zephyr/timing/timing.h>
#include <stdint.h>

__syscall timing_t timing_timestamp_get(void);

void     timestamp_overhead_init(uint32_t num_iterations);
uint64_t timestamp_overhead_adjustment(uint32_t options1, uint32_t options2);

#include <zephyr/syscalls/timing_sc.h>

#endif
