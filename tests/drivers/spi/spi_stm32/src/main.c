/*
 * Copyright (c) 2023 Graphcore Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>

ZTEST(spi_stm32, test_one)
{
	zassert_true(1);
}

ZTEST_SUITE(spi_stm32, NULL, NULL, NULL, NULL, NULL);
