/*
 * Copyright (c) 2021-2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <stdint.h>

#define SPI_FLASH_HWINFO_ID_LEN ((size_t)6)

extern uint32_t flash_w91_get_id(const struct device *dev, uint8_t *flash_id);


ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	ssize_t result = length;
	uint8_t chip_id_val[SPI_FLASH_HWINFO_ID_LEN] = {0};
	const struct device *flash_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

	if (length < SPI_FLASH_HWINFO_ID_LEN) {
		printk("Not enougth buffer size to get the hwinfo (ID).\n\r");
		result = 0;
	} else {
		result = (size_t)flash_w91_get_id(flash_dev, chip_id_val);
	}

	/* Check get device ID operation and store it into the buffer */
	if (result == (size_t)0) {
		memcpy(buffer, chip_id_val, SPI_FLASH_HWINFO_ID_LEN);
		result = SPI_FLASH_HWINFO_ID_LEN;
	} else {
		printk("Flash hw INFO get ID read failed!\n");
		result = 0;
	}

	return result;
}
