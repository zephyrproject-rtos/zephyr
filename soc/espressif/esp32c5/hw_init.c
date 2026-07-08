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

#include <soc/pcr_reg.h>
#include <hal/apm_ll.h>

#include <bootloader_clock.h>
#include <bootloader_flash.h>
#include <esp_flash_internal.h>
#include <esp_log.h>
#include <esp_private/esp_clk_tree_common.h>

#include <console_init.h>
#include <flash_init.h>
#include <soc_flash_init.h>
#include <soc_init.h>

const static char *TAG = "hw_init";

extern void esp_cpu_configure_invalid_regions(void);

int hardware_init(void)
{
	int err = 0;

	soc_hw_init();
	ana_reset_config();
	super_wdt_auto_feed();

	/*
	 * The APM access-path filters default to enabled and only allow
	 * masters in TEE mode. Disable them so the application (which runs in
	 * REE mode) is not denied access to peripherals.
	 */
	apm_ll_hp_apm_enable_ctrl_filter_all(false);
	apm_ll_lp_apm_enable_ctrl_filter_all(false);
	apm_ll_lp_apm0_enable_ctrl_filter_all(false);

	/*
	 * On power-up only the HP CPU starts in TEE mode; the LP CPU defaults
	 * to REE2. The TEE controller grants access to the PMU and LP_AON
	 * peripherals to TEE mode only, so LP-core accesses to that window are
	 * silently dropped, breaking the LP->HP wakeup path. Put both cores in
	 * TEE mode to match the access-control configuration.
	 */
	apm_ll_hp_tee_set_master_sec_mode(APM_MASTER_HPCORE, APM_SEC_MODE_TEE);
	apm_ll_lp_tee_set_master_sec_mode(APM_MASTER_LPCORE, APM_SEC_MODE_TEE);

	/*
	 * Configure PMA (Physical Memory Attributes) entries to replace
	 * ROM's default configuration. This is needed because the Zephyr
	 * PMP init (z_riscv_pmp_init) enables MPRV which changes how
	 * PMA+PMP interact. Without proper PMA entries, peripheral
	 * register access (e.g. PMU at 0x600B0000) will fault.
	 * Only configure PMA here; PMP is managed by Zephyr's arch layer.
	 */
	esp_cpu_configure_invalid_regions();

	bootloader_clock_configure();
	esp_clk_tree_initialize();

#ifdef CONFIG_ESP_CONSOLE
	esp_console_init();
	print_banner();
#endif

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
