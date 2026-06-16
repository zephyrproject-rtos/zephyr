/*
 * Common functions and helpers for ISO broadcast tests
 *
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/sys/clock.h>

#include "bs_types.h"

#define WAIT_TIME (30e6) /* 30 seconds*/

void test_init(void);
void test_tick(bs_time_t HW_device_time);

#define SDU_INTERVAL_US 10U * USEC_PER_MSEC /* 10 ms */
