/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_CPU_CORTEX_M)
  #error project can only run on Cortex-M
#endif

#include <ztest.h>

ZTEST_SUITE(vector_table, NULL, NULL, NULL, NULL, NULL);
