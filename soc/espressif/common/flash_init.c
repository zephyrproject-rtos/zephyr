/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "flash_init.h"

#include <stdbool.h>
#include <bootloader_flash_priv.h>
#include <hal/efuse_ll.h>
#include <hal/efuse_hal.h>
#if CONFIG_SOC_SERIES_ESP32S3
#include <esp32s3/opi_flash_private.h>
#endif

bool flash_is_octal_mode_enabled(void)
{
#if SOC_SPI_MEM_SUPPORT_OPI_MODE
	return efuse_ll_get_flash_type();
#else
	return false;
#endif
}

int spi_flash_init_chip_state(void)
{
#if SOC_SPI_MEM_SUPPORT_OPI_MODE
	if (flash_is_octal_mode_enabled()) {
		return esp_opiflash_init(rom_spiflash_legacy_data->chip.device_id);
	}
#endif
#if CONFIG_SOC_SERIES_ESP32S3
	/* Currently, only esp32s3 allows high performance mode. */
	return spi_flash_enable_high_performance_mode();
#else
	return 0;
#endif /* CONFIG_SOC_SERIES_ESP32S3 */
}
