/*
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <stdint.h>
#include <stdio.h>
#include <init.h>

static int _init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Enable mcycle and minstret counter */
	__asm__ ("csrsi mcountinhibit, 0x5");
	return 0;
}

SYS_INIT(_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
