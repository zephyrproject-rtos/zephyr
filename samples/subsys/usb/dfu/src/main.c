/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2017 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Sample app for USB DFU class driver. */

#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(main);

void main(void)
{
	/* Nothing to be done other than the selecting appropriate build
	 * config options. Use dfu-util to update the device.
	 */
	LOG_INF("This device supports USB DFU class.\n");
}
