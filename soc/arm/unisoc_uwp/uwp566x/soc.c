/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>

#include <uwp_hal.h>

extern void uwp_cache_init(void);
extern void uwp_glb_init(void);

static int unisoc_uwp_init(struct device *arg)
{
	ARG_UNUSED(arg);

	return 0;
}

SYS_INIT(unisoc_uwp_init, PRE_KERNEL_1, 0);
