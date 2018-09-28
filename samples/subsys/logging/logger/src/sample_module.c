/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <logging/log.h>

#define LOG_MODULE_NAME sample_module
#define LOG_LEVEL CONFIG_SAMPLE_MODULE_LOG_LEVEL
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

const char *sample_module_name_get(void)
{
	return STRINGIFY(LOG_MODULE_NAME);
}

void sample_module_func(void)
{
	LOG_INF("log in test_module %d", 11);
}
