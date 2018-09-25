/*
 * Copyright (c) 2018 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test module for log core test
 *
 */

#include <logging/log.h>

LOG_MODULE_DECLARE(test);

void test_func(void)
{
	LOG_ERR("test");
}
