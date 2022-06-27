/*
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <stdio.h>
#define USER_STACKSIZE	2048

struct k_thread user_thread;
K_THREAD_STACK_DEFINE(user_stack, USER_STACKSIZE);

static void user_function(void *p1, void *p2, void *p3)
{
	printf("Hello World from UserSpace! %s\n", CONFIG_BOARD);
}


void main(void)
{
	k_thread_create(&user_thread, user_stack, USER_STACKSIZE,
			user_function, NULL, NULL, NULL,
			-1, K_USER, K_MSEC(0));
}
