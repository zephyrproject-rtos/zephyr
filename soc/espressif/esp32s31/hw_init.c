/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hw_init.h>
#include <stdint.h>
#include <esp_cpu.h>
#include <soc/rtc.h>
#include <esp_rom_sys.h>

#include <hal/cache_hal.h>
#include <hal/mmu_hal.h>
#include <hal/mmu_ll.h>

#include <soc/hp_apm_reg.h>
#include <soc/hp_mem_apm_reg.h>
#include <soc/lp_apm_reg.h>

#include <bootloader_clock.h>
#include <bootloader_flash.h>
#include <esp_flash_internal.h>
#include <esp_private/esp_clk_tree_common.h>
#include <esp_log.h>

#include <console_init.h>
#include <flash_init.h>
#include <soc_flash_init.h>
#include <soc_init.h>

const static char *TAG = "hw_init";

int hardware_init(void)
{
	int err = 0;

	soc_hw_init();
	ana_reset_config();
	super_wdt_auto_feed();

	/*
	 * By default these access path filters are enabled and only allow
	 * access to masters that are in TEE mode. Since all masters except
	 * the HP CPU boot in REE mode, the default setting denies access to
	 * them. Disable the filters at boot; TEE initialization code can
	 * re-enable them per use case. The HP memory APM guards the HP SRAM
	 * against non-CPU masters (DMA); leaving it enabled silently drops
	 * every DMA access to internal RAM.
	 */
	REG_WRITE(LP_APM_FUNC_CTRL_REG, 0);
	REG_WRITE(HP_APM_FUNC_CTRL_REG, 0);
	REG_WRITE(HP_MEM_APM_FUNC_CTRL_REG, 0);

	/* Program the PMA entry that makes the external RAM (PSRAM) cache
	 * window reachable from the CPU. Without it any access to the
	 * 0x50000000 window raises a bus access fault.
	 */
	esp_cpu_configure_region_protection();

	bootloader_clock_configure();

	esp_clk_tree_initialize();

#ifdef CONFIG_ESP_CONSOLE
	esp_console_init();
	print_banner();
#endif

	/*
	 * Cache and MMU must be initialized before any flash access.
	 * ESP32-S31 uses a unified L1 cache with a flat HP SRAM region,
	 * so only the core count needs to be supplied.
	 */
	cache_hal_config_t cache_config = {
		.core_nums = 1,
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

	return 0;
}
