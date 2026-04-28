/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hw_init.h"
#include <stdint.h>
#include <esp_cpu.h>
#include <soc/rtc.h>
#include <soc/extmem_reg.h>
#include <esp_rom_sys.h>

#include <hal/cache_hal.h>
#include <hal/mmu_hal.h>

#include <bootloader_clock.h>
#include <bootloader_flash.h>
#include <esp_flash_internal.h>
#include <esp_log.h>

#include <console_init.h>
#include <flash_init.h>
#include <soc_flash_init.h>
#include <soc_init.h>
#include <soc_random.h>

const static char *TAG = "hw_init";

int hardware_init(void)
{
	int err = 0;

	super_wdt_auto_feed();

#ifdef CONFIG_BOOTLOADER_REGION_PROTECTION_ENABLE
	esp_cpu_configure_region_protection();
#endif

	bootloader_clock_configure();

#ifdef CONFIG_ESP_CONSOLE
	/* initialize console, from now on, we can log */
	esp_console_init();
	print_banner();
#endif /* CONFIG_ESP_CONSOLE */

	cache_hal_init();

	mmu_hal_init();

	/* Workaround: normal ROM bootloader exits with DROM0 cache unmasked, but 2nd bootloader
	 * exits with it masked.
	 */
	REG_CLR_BIT(EXTMEM_PRO_ICACHE_CTRL1_REG, EXTMEM_PRO_ICACHE_MASK_DROM0);

	flash_update_id();

	err = bootloader_flash_xmc_startup();
	if (err != 0) {
		ESP_EARLY_LOGE(TAG, "failed when running XMC startup flow, reboot!");
		return err;
	}

	err = read_bootloader_header();
	if (err != 0) {
		return err;
	}

	err = check_bootloader_validity();
	if (err != 0) {
		return err;
	}

	err = init_spi_flash();
	if (err != 0) {
		return err;
	}

	check_wdt_reset();
	config_wdt();

	soc_random_enable();

	return 0;
}
