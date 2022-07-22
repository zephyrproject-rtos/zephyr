/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_CPU_CORTEX_M)
  #error test can only run on Cortex-M MCUs
#endif

#include <ztest.h>

ZTEST_SUITE(arm_ramfunc, NULL, NULL, NULL, NULL, NULL);
