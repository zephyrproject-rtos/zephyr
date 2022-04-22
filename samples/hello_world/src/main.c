/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

void main(void)
{
	LOG_INF("test string that should be removed. %d", 0xDDDDDDDD);
	LOG_INF("test string that should be also removed.");
}
