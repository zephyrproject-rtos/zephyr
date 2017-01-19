/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Sample to put the device in USB mass storage mode backed on a 16k RAMDisk. */

#include <zephyr.h>
#include <logging/sys_log.h>

void main(void)
{
	/* Nothing to be done other than the selecting appropriate build
	 * config options. Everything is driven from the USB host side.
	 */
	SYS_LOG_INF("The device is put in USB mass storage mode.\n");
}

