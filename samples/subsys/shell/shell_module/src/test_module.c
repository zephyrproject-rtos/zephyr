/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(app_test);

void foo(void)
{
	LOG_INF("info message");
	LOG_WRN("warning message");
	LOG_ERR("err message");
}
