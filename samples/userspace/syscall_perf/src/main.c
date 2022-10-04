/*
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <stdio.h>

#include "main.h"
#include "thread_def.h"

void main(void)
{
	printf("Main Thread started; %s\n", CONFIG_BOARD);

	k_thread_create(&supervisor_thread, supervisor_stack, THREAD_STACKSIZE,
			supervisor_thread_function, NULL, NULL, NULL,
			-1, K_INHERIT_PERMS, K_NO_WAIT);

	k_sleep(K_MSEC(1000));

	k_thread_create(&user_thread, user_stack, THREAD_STACKSIZE,
			user_thread_function, NULL, NULL, NULL,
			-1, K_USER | K_INHERIT_PERMS, K_NO_WAIT);
}
