/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/misc/esp_tool/esp_tool.h>
#include <zephyr/device.h>

#include "hexdump.h"

static const struct device *esp = DEVICE_DT_GET(DT_INST(0, espressif_esp_tool));
static uint8_t buf[512];

int main(void)
{
	int ret;
	uint32_t flash_size, chip_id;
	uint32_t offset;

	ret = device_is_ready(esp);
	if (ret == 0) {
		printk("ESP device is not ready (err=%d).\n", ret);
		return -1;
	}
	printk("ESP device is ready (ret=%d).\n", ret);

	ret = esp_tool_connect(esp);
	if (ret) {
		printk("Could not open esp device\n");
		return -1;
	}
	printk("ESP device connected\n");

	ret = esp_tool_get_target(esp, &chip_id);
	if (ret) {
		printk("Target detection failed!\n");
		return -1;
	}
	printk("Target chip id is %x\n", chip_id);

	ret = esp_tool_flash_detect_size(esp, &flash_size);
	if (ret) {
		printk("Could not detected flash size");
		return -1;
	}
	printk("Detected flash size is %d MB (%d Bytes)\n", flash_size/1024/1024, flash_size);

	if (esp_tool_get_boot_offset(esp, chip_id, &offset)) {
		printk("boot offset failed\n");
		return -1;
	}
	printk("Target chip boot offset is 0x%x\n", offset);

	ret = esp_tool_flash_read(esp, offset, buf, sizeof(buf));
	if (ret) {
		printk("flash read ret=%d\n", ret);
		return -1;
	}

	hexdump("0x0", buf, sizeof(buf));

	if (buf[0] == 0xe9) {
		printk("Boot vector installed");
	} else {
		printk("Target chip not bootable!");
	}

	printk("Resetting target...");

	ret = esp_tool_reset_target(esp);
	if (ret) {
		printk("Target reset failed\n");
		return -1;
	}
	printk("OK\n\n");

	return 0;
}
