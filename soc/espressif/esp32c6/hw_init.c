/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
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

#include <soc/hp_apm_reg.h>
#include <soc/lp_apm_reg.h>
#include <soc/lp_apm0_reg.h>
#include <soc/pcr_reg.h>

#include <bootloader_clock.h>
#include <bootloader_flash.h>
#include <esp_flash_internal.h>
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

	/* By default, these access path filters are enable and allow the
	 * access to masters only if they are in TEE mode. Since all masters
	 * except HP CPU boots in REE mode, default setting of these filters
	 * will deny the access to all masters except HP CPU.
	 * So, at boot disabling these filters. They will enable as per the
	 * use case by TEE initialization code.
	 */
	REG_WRITE(LP_APM_FUNC_CTRL_REG, 0);
	REG_WRITE(LP_APM0_FUNC_CTRL_REG, 0);
	REG_WRITE(HP_APM_FUNC_CTRL_REG, 0);

#ifdef CONFIG_BOOTLOADER_REGION_PROTECTION_ENABLE
	esp_cpu_configure_region_protection();
#endif

	bootloader_clock_configure();

#ifdef CONFIG_ESP_CONSOLE
	/* initialize console, from now on, we can log */
	esp_console_init();
	print_banner();
#endif /* CONFIG_ESP_CONSOLE */

	cache_hal_config_t cache_config = {
		.core_nums = 1,
	};
	cache_hal_init(&cache_config);

	mmu_hal_config_t mmu_config = {
		.core_nums = 1,
		.mmu_page_size = CONFIG_MMU_PAGE_SIZE,
	};
	/*
	 * Initialize MMU context and page size but skip mmu_hal_unmap_all().
	 * In simple boot mode, ROM bootloader has already set up MMU mappings
	 * for flash access. Calling mmu_hal_unmap_all() would invalidate those
	 * mappings and break access to flash-based code and data (IROM/DROM).
	 */
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
