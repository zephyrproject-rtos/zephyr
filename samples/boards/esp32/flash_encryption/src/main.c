/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#include <esp_spi_flash.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_encryption, CONFIG_LOG_DEFAULT_LEVEL);

#if !defined(CONFIG_SOC_ESP32)
#error Flash encryption feature is only available for ESP32 SOC yet.
#endif

void main(void)
{
	uint8_t buffer[32];
	const struct device *flash_device;
	off_t address = FLASH_AREA_OFFSET(storage);

	flash_device = device_get_binding(DT_LABEL(DT_CHOSEN(zephyr_flash_controller)));

	if (flash_device) {
		LOG_INF("Found flash controller %s\n\r",
			DT_LABEL(DT_CHOSEN(zephyr_flash_controller)));
	} else {
		LOG_INF("Flash controller not available\n\r");
		return;
	}

	for (int k = 0; k < 32; k++) {
		buffer[k] = k;
	}
	LOG_HEXDUMP_INF(buffer, sizeof(buffer), "WRITE BUFFER CONTENT");

	/* erase flash content */
	flash_erase(flash_device, address, 4096);

	/* this writes encrypted data into flash */
	flash_write(flash_device, address, &buffer, sizeof(buffer));

	/* read flash content without decrypting content */
	memset(buffer, 0, sizeof(buffer));
	spi_flash_read(address, &buffer, sizeof(buffer));
	LOG_HEXDUMP_INF(buffer, sizeof(buffer), "FLASH RAW DATA (Encrypted)");

	/* read flash content and decrypt */
	memset(buffer, 0, sizeof(buffer));
	flash_read(flash_device, address, &buffer, sizeof(buffer));
	LOG_HEXDUMP_INF(buffer, sizeof(buffer), "FLASH DECRYPTED DATA");
}
