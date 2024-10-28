/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fff.h>

/* List of fakes used by this unit tester */
#define SYS_CLOCK_MOCKS_FFF_FAKES_LIST(FAKE)                                                       \
	FAKE(sys_timepoint_calc)                                                                   \
	FAKE(sys_timepoint_timeout)

DECLARE_FAKE_VALUE_FUNC(k_timepoint_t, sys_timepoint_calc, k_timeout_t);
DECLARE_FAKE_VALUE_FUNC(k_timeout_t, sys_timepoint_timeout, k_timepoint_t);
