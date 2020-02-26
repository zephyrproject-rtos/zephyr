/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>

void test_flash(void);

LOG_MODULE_REGISTER(main);

/* This semaphore is used to serialize the UART prints dumped by various
 * modules. This prevents mixing of UART prints across modules. This
 * semaphore starts off "available".
 */
K_SEM_DEFINE(thread_sem, 1, 1);

void main(void)
{
	LOG_INF("Sample app running on: %s Intel S1000 CRB\n", CONFIG_ARCH);

	test_flash();
}
