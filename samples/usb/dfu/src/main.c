/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample app for DFU class driver
 *
 * This sample app implements a DFU class driver. It does not perform an actual
 * firmware upgrade instead it allows user to upload a file at a predetermined
 * flash address or to download the content from that flash address.
 */

#include <string.h>
#include <zephyr.h>
#include "usb_dfu.h"
#include <stdio.h>

#ifdef CONFIG_SOC_QUARK_SE_C1000
#define DFU_FLASH_DEVICE "QUARK_FLASH"
/* Unused flash area to test DFU class driver */
#define DFU_FLASH_TEST_ADDR (0x40030000 + 0x10000)
#define DFU_FLASH_PAGE_SIZE (2048)
#define DFU_FLASH_UPLOAD_SIZE (0x6000)
#else
#error "Unsupported board"
#endif

void main(void)
{
	struct device *dev = NULL;

	printf("DFU Test Application\n");

	dev = device_get_binding(DFU_FLASH_DEVICE);
	if (!dev) {
		printf("Flash device not found\n");
		return;
	}

	dfu_start(dev,
		DFU_FLASH_TEST_ADDR,
		DFU_FLASH_PAGE_SIZE,
		DFU_FLASH_UPLOAD_SIZE);

	while (1) {
		/* do nothing */
	}
}
