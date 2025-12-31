/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

static int lpc845_soc_init(void)
{
	/* Minimal init - chip runs at default frequency
	 */
	return 0;
}

SYS_INIT(lpc845_soc_init, PRE_KERNEL_1, 0);
