/* system.c - system/hardware module for quark_se_ss BSP */

/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * This module provides routines to initialize and support board-level hardware
 * for the Quark SE platform.
 */

#include <kernel.h>
#include "soc.h"
#include <init.h>

/**
 * @brief perform basic hardware initialization
 *
 * RETURNS: N/A
 */
static int quark_se_arc_init(struct device *arg)
{
	ARG_UNUSED(arg);

	_quark_se_ss_ready();

	return 0;
}

SYS_INIT(quark_se_arc_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
