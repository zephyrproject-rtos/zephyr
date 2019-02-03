/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/__assert.h>
#include <zephyr.h>

void assert_post_action(void)
{
	k_panic();
}
