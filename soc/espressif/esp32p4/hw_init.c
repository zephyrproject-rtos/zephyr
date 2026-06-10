/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hw_init.h"
#include <stdint.h>
#include <esp_cpu.h>
#include <soc/rtc.h>
#include <esp_rom_sys.h>

#include <hal/cache_hal.h>
#include <hal/mmu_hal.h>
#include <hal/mmu_ll.h>
#include <hal/psram_ctrlr_ll.h>

#include <bootloader_clock.h>
#include <bootloader_flash.h>
#include <esp_flash_internal.h>
#include <esp_log.h>
#include <esp_private/esp_clk_tree_common.h>

#include <console_init.h>
#include <flash_init.h>
#include <soc_flash_init.h>
#include <soc_init.h>

int hardware_init(void)
{
	int err = 0;

	soc_hw_init();
	ana_reset_config();
	super_wdt_auto_feed();
	esp_clk_tree_initialize();
	bootloader_clock_configure();

	/* The ROM bootloader leaves PSRAM module clocks gated. Enable them
	 * before cache/MMU init so the cache controller can drive the PSRAM
	 * bus shared with flash on ESP32-P4.
	 */
	psram_ctrlr_ll_enable_module_clock(PSRAM_CTRLR_LL_MSPI_ID_2, true);

#ifdef CONFIG_ESP_CONSOLE
	esp_console_init();
	print_banner();
#endif

	/*
	 * Cache and MMU must be initialized before any flash access.
	 * On P4, the cache init releases the SPI0/SPI1 bus arbitration
	 * that the ROM bootloader leaves in a locked state.
	 */
	cache_hal_config_t cache_config = {
		.core_nums = 1,
		.l2_cache_size = CONFIG_ESP32_CACHE_L2_SIZE,
		.l2_cache_line_size = CONFIG_ESP32_CACHE_L2_LINE_SIZE,
	};
	cache_hal_init(&cache_config);

	mmu_hal_config_t mmu_config = {
		.core_nums = 1,
		.mmu_page_size = CONFIG_MMU_PAGE_SIZE,
	};
	mmu_hal_ctx_init(&mmu_config);
	mmu_ll_set_page_size(0, CONFIG_MMU_PAGE_SIZE);

	flash_update_id();

	err = bootloader_flash_xmc_startup();
	if (err != 0) {
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

	return 0;
}
