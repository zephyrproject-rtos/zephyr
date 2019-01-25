/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/__assert.h>
#include <zephyr.h>

void assert_post_action(void)
{
	if (IS_ENABLED(CONFIG_ARCH_POSIX)) {
		extern void posix_exit(int exit_code);
		posix_exit(1);
	} else {
		k_panic();
	}
}
