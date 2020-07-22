/*
 * Copyright (c) 2020 RISE Research Institutes of Sweden <www.ri.se>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stdlib.h>
#include <sys/printk.h>
#include <kernel.h>
#include <linker/linker-defs.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(app_main);

#define STACK_SIZE 512
#define PRIORITY 0

void dummy(void);

K_THREAD_DEFINE(tid0, STACK_SIZE, dummy, NULL, NULL, NULL, PRIORITY, K_USER, 0);
K_THREAD_DEFINE(tid1, STACK_SIZE, dummy, NULL, NULL, NULL, PRIORITY, K_USER, 0);
K_THREAD_DEFINE(tid2, STACK_SIZE, dummy, NULL, NULL, NULL, PRIORITY, K_USER, 0);
K_THREAD_DEFINE(tid3, STACK_SIZE, dummy, NULL, NULL, NULL, PRIORITY, K_USER, 0);

void dummy(void)
{
	if (arch_is_user_context()) {
		printk("Hello from user context! ");
	} else {
		printk("Hello from machine context! ");
	}

	const char *str = "Hej";
	int err_arg;
	size_t nlen = arch_user_string_nlen(str, 4, &err_arg);

	printk("%s is %d characters long.\n", str, nlen);

	k_yield();
}

void main(void)
{
	k_mem_domain_remove_thread(tid0);
	k_yield();

	//k_mem_domain_remove_thread(_current);

	if (arch_is_user_context()) {
		printk("Hello from user context! ");
	} else {
		printk("Hello from machine context! ");
	}

	const char *str = "Fika";
	int err_arg;
	size_t nlen = arch_user_string_nlen(str, 4, &err_arg);

	printk("%s is %d characters long.\n", str, nlen);

	LOG_INF("SUCCESS");
}
