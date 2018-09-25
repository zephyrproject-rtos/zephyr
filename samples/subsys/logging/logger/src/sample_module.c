/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <logging/log.h>
#include "sample_module.h"

LOG_MODULE_REGISTER(MODULE_NAME, CONFIG_SAMPLE_MODULE_LOG_LEVEL);

const char *sample_module_name_get(void)
{
	return STRINGIFY(MODULE_NAME);
}

void sample_module_func(void)
{
	LOG_INF("log in test_module %d", 11);
}
