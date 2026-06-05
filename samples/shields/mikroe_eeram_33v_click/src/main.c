/*
 * Copyright (c) 2025 Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(eeram_click, CONFIG_EEPROM_LOG_LEVEL);

/*
 * Get the EERAM device via the eeprom-0 alias.
 * This alias is provided by the mikroe_eeram_33v_click shield overlay.
 *
 * A build error here means the shield was not specified with --shield
 * or the board does not define a mikrobus_i2c node.
 */
static const struct device *const eeram_dev = DEVICE_DT_GET(DT_ALIAS(eeprom_0));

#define TEST_OFFSET   0
#define TEST_MAGIC    0xEE4716U

struct eeram_data {
	uint32_t magic;
	uint32_t boot_count;
	uint8_t  payload[24];
};

int main(void)
{
#if CONFIG_DCACHE
	struct eeram_data data __aligned(CONFIG_DCACHE_LINE_SIZE);
	struct eeram_data verify __aligned(CONFIG_DCACHE_LINE_SIZE);
#else
	struct eeram_data data __aligned(32);
	struct eeram_data verify __aligned(32);
#endif
	int ret;

	if (!device_is_ready(eeram_dev)) {
		LOG_ERR("EERAM device not ready");
		return -ENODEV;
	}

	printk("EERAM 3.3V Click sample - Microchip 47L16\n");
	printk("==========================================\n");
	printk("EERAM size (from dt): %zu bytes\n", eeprom_get_size(eeram_dev));

	printk("\n--- EEPROM Driver Test ---\n");

	/* Fill test data with pattern */
	data.magic = TEST_MAGIC;
	data.boot_count = 0x12345678;
	for (size_t i = 0; i < sizeof(data.payload); i++) {
		data.payload[i] = (uint8_t)(i & 0xFF);
	}

	printk("Writing %zu bytes to EERAM...\n", sizeof(data));
	ret = eeprom_write(eeram_dev, TEST_OFFSET, &data, sizeof(data));
	if (ret < 0) {
		LOG_ERR("Write failed: %d", ret);
		return ret;
	}

	printk("Reading back %zu bytes...\n", sizeof(verify));
	ret = eeprom_read(eeram_dev, TEST_OFFSET, &verify, sizeof(verify));
	if (ret < 0) {
		LOG_ERR("Read failed: %d", ret);
		return ret;
	}

	if (memcmp(&data, &verify, sizeof(data)) != 0) {
		LOG_ERR("Data mismatch after read-back");
		return -EIO;
	}

	printk("EEPROM driver test PASSED\n");
	return 0;
}
