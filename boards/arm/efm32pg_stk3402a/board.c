/*
 * Copyright (c) 2017 Christian Taedcke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <board.h>

static int efm32pg_stk3402a_init(struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

/* needs to be done after GPIO driver init */
SYS_INIT(efm32pg_stk3402a_init, PRE_KERNEL_1, CONFIG_BOARD_INIT_PRIORITY);
