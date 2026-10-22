/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_CPU_CORTEX_M)
#error test can only run on Cortex-M MCUs
#endif

#include <zephyr/ztest.h>

ZTEST_SUITE(arm_sw_vector_relay, NULL, NULL, NULL, NULL, NULL);
