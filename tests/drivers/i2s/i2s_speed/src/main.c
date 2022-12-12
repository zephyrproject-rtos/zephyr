/*
 * Copyright (c) 2017 comsuisse AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

extern void *test_i2s_speed_configure(void);
extern void *test_i2s_speed_rxtx_configure(void);
ZTEST_SUITE(drivers_i2s_speed, NULL, test_i2s_speed_configure, NULL, NULL, NULL);
ZTEST_SUITE(drivers_i2s_speed_both_rxtx, NULL, test_i2s_speed_rxtx_configure, NULL, NULL, NULL);
