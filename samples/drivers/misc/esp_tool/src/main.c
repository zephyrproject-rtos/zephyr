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
	uint32_t flash_size, target_id;
	uint32_t addr, len;

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

	ret = esp_tool_get_target(esp, &target_id);
	if (ret) {
		printk("Target detection failed!\n");
		return -1;
	}
	printk("Target id %x\n", target_id);

	ret = esp_tool_flash_detect_size(esp, &flash_size);
	if (ret) {
		printk("Could not detected flash size");
		return -1;
	}
	printk("Detected flash size %d\n", flash_size);

//	while (1) {
//		esp_tool_flash_write(esp_dev, tmp, 0x1000);
//	}
//	esp_tool_flash_finish(esp_dev, 0);

	ret = esp_tool_flash_read(esp, 0x0, buf, sizeof(buf));
	if (ret) {
		printk("flash read ret=%d\n", ret);
	}

	hexdump("0x0", buf, sizeof(buf));

//	esp_tool_flash_erase(esp_dev);

//	esp_tool_flash_erase_region(esp_dev, offset, size);

//	esp_tool_tr_rate(esp_dev, old, new);
//	esp_tool_tr_rate_stub(esp_dev, old, new);
//	esp_tool_get_security_info(esp_dev, old, new);

	return 0;
}
