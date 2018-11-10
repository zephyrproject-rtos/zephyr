/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <flash_map.h>
#include <misc/printk.h>
#include <zephyr.h>
#include <string.h>

void main(void)
{
	const struct flash_area *flash_area;

	if (flash_area_open(FLASH_AREA_EXTERNAL_ID, &flash_area)) {
		printk("flash_area_open() fail");
	} else {
		u32_t buf[64];

		if (flash_area_erase(flash_area, 0, KB(8))) {
			printk("flash_area_erase() fail");
			return;
		}

		memset(buf, 0x5a, sizeof(buf));
		if (flash_area_write(flash_area, 0, buf, sizeof(buf))) {
			printk("flash_area_write() fail");
			return;
		}

		memset(buf, 0, sizeof(buf));
		if (flash_area_read(flash_area, 0, buf, sizeof(buf))) {
			printk("flash_area_read() fail");
			return;
		}

		flash_area_close(flash_area);
	}
}
