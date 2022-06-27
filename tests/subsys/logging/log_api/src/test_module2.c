/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test module for log api test
 *
 */

#include "test_module.h"
#include <zephyr/logging/log.h>

/* Module disabled by default, will emit logs only when CONFIG_LOG_OVERRIDE_LEVEL. */
LOG_MODULE_REGISTER(test2, 0);

void test_func2(void)
{
	LOG_DBG(TEST_DBG_MSG);
	LOG_ERR(TEST_ERR_MSG);
}
