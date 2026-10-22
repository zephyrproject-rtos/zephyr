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

#include "test_module.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(test, CONFIG_SAMPLE_MODULE_LOG_LEVEL);

void test_func(void)
{
	LOG_DBG(TEST_DBG_MSG);
	LOG_ERR(TEST_ERR_MSG);
}
