/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fff.h>

/* List of fakes used by this unit tester */
#define KERNEL_FFF_FAKES_LIST(FAKE)         \
		FAKE(k_uptime_ticks)                \

DECLARE_FAKE_VALUE_FUNC(int64_t, k_uptime_ticks);
