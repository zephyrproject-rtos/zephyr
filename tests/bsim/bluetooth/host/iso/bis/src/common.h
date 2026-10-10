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

#define WAIT_TIME (60e6) /* 60 seconds*/

enum disable_states {
	DISABLE_STATE_BROADCASTER,
	DISABLE_STATE_SYNC_RECEIVER,
	DISABLE_STATE_BOTH,
	DISABLE_STATE_COUNT,
};

void test_init(void);
void test_tick(bs_time_t HW_device_time);

#define SDU_INTERVAL_US 10U * USEC_PER_MSEC /* 10 ms */
