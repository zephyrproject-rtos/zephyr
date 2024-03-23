/*
 * Copyright (c) 2023 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#include <esp_spi_flash.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_memory_mapped, CONFIG_LOG_DEFAULT_LEVEL);

int main(void)
{
	uint8_t buffer[32];
	const struct device *flash_device;
	const void *mem_ptr;
	spi_flash_mmap_handle_t handle;
	off_t address = FIXED_PARTITION_OFFSET(scratch_partition);
	size_t size = FIXED_PARTITION_SIZE(scratch_partition);

	flash_device = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
	if (!device_is_ready(flash_device)) {
		printk("%s: device not ready.\n", flash_device->name);
		return 0;
	}

	/* map selected region */
	spi_flash_mmap(address, size, SPI_FLASH_MMAP_DATA, &mem_ptr, &handle);
	LOG_INF("memory-mapped pointer address: %p", mem_ptr);

	/* erase and read flash */
	flash_erase(flash_device, address, size);
	LOG_HEXDUMP_INF(mem_ptr, 32, "flash read using memory-mapped pointer");

	LOG_INF("writing 32-bytes data using flash API");
	memset(buffer, 0, sizeof(buffer));
	for (int k = 0; k < 32; k++) {
		buffer[k] = k;
	}
	flash_write(flash_device, address, buffer, 32);

	/* read using flash API */
	memset(buffer, 0, sizeof(buffer));
	flash_read(flash_device, address, buffer, 32);
	LOG_HEXDUMP_INF(buffer, 32, "flash read using flash API");

	LOG_HEXDUMP_INF(mem_ptr, 32, "flash read using memory-mapped pointer");

	if (memcmp(buffer, mem_ptr, 32) == 0) {
		LOG_INF("memory-mapped reading matches flash API read");
	} else {
		LOG_ERR("memory-mapped reading does not match flash API read");
	}

	/* unmap mapped region */
	spi_flash_munmap(handle);

	return 0;
}
