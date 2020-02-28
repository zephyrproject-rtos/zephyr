/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(hello_world, LOG_LEVEL_INF);

void main(void)
{
	LOG_INF("main()");
	printk("hello world! %s\n", CONFIG_BOARD);
}
