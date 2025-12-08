/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/misc/esp_tool/esp_tool.h>
#include <zephyr/device.h>

/* local */
//#include "hexdump.h"

static const struct device *esp = DEVICE_DT_GET(DT_INST(0, espressif_esp_tool));
//static uint8_t buf[128];

int main(void)
{
	int ret;
	uint32_t flash_size, chip_id;
	uint32_t offset;
	char *name;

	ret = device_is_ready(esp);
	if (ret == 0) {
		printk("ESP device is not ready (err=%d).\n", ret);
		return -1;
	}
	printk("ESP device is ready (ret=%d).\n", ret);

#if 0
	ret = esp_tool_connect_stub(esp, 0);
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

	ret = esp_tool_get_target_name(esp, &name);
	if (ret) {
		printk("Target name detection failed!\n");
		return -1;
	}
	printk("Target name %s\n", name);

	ret = esp_tool_flash_detect_size(esp, &flash_size);
	if (ret) {
		printk("Could not detected flash size\n");
		return -1;
	}
	printk("Detected flash size is %d MB (%d Bytes)\n",
		flash_size/1024/1024, flash_size);

	if (esp_tool_get_boot_offset(esp, &offset)) {
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
		printk("Boot vector installed\n");
	} else {
		printk("Target chip not bootable!\n");
	}

	printk("Resetting target...");

	ret = esp_tool_reset_target(esp);
	if (ret) {
		printk(" Failed\n");
		return -1;
	}
	printk("OK\n\n");
#endif

	return 0;
}
