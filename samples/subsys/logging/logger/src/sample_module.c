/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>

#define LOG_MODULE_NAME foo
#include <logging/log.h>
LOG_MODULE_REGISTER();

const char *sample_module_name_get(void)
{
	return STRINGIFY(LOG_MODULE_NAME);
}

void sample_module_func(void)
{
	LOG_INF("log in test_module %d", 11);
}
