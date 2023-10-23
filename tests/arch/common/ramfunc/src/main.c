/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#if !defined(CONFIG_ARCH_HAS_RAMFUNC_SUPPORT)
  #error test can only run on Cortex-M MCUs and RISC-V
#endif

ZTEST_SUITE(ramfunc, NULL, NULL, NULL, NULL, NULL);
