/*
 * Common functions and helpers for ISO broadcast tests
 *
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/sys/util_macro.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "bstests.h"

#define WAIT_TIME (30e6) /* 30 seconds*/

void test_init(void);
void test_tick(bs_time_t HW_device_time);

/* Generate 1 KiB of mock data going 0x00, 0x01, ..., 0xff, 0x00, 0x01, ..., 0xff, etc */
#define ISO_DATA_GEN(_i, _) _i
static const uint8_t mock_iso_data[] = {
	LISTIFY(255, ISO_DATA_GEN, (,)),
		LISTIFY(255, ISO_DATA_GEN, (,)),
			LISTIFY(255, ISO_DATA_GEN, (,)),
				LISTIFY(255, ISO_DATA_GEN, (,)),
};
