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
#include <hal/apm_ll.h>

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

extern void esp_cpu_configure_invalid_regions(void);

int hardware_init(void)
{
	int err = 0;

	soc_hw_init();
	ana_reset_config();
	super_wdt_auto_feed();

	/* The APM access-path filters default to enabled and only allow
	 * masters in TEE mode. Disable them so the application (which runs in
	 * REE mode) is not denied access to peripherals, including the modem
	 * and RF register bus used by the Wi-Fi controller.
	 */
	apm_ll_hp_apm_enable_ctrl_filter_all(false);
	apm_ll_lp_apm_enable_ctrl_filter_all(false);
	/* The CPU APM filter stays at its defaults; it must also be
	 * opened when user mode support is added, since it silently
	 * denies U-mode fetches.
	 */

	/* Configure PMA (Physical Memory Attributes) entries to replace
	 * ROM's default configuration. This is needed because the Zephyr
	 * PMP init (z_riscv_pmp_init) enables MPRV which changes how
	 * PMA+PMP interact. Without proper PMA entries, peripheral
	 * register access (e.g. PMU at 0x600B0000) will fault.
	 * Only configure PMA here; PMP is managed by Zephyr's arch layer.
	 */
	esp_cpu_configure_invalid_regions();

	bootloader_clock_configure();

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
	/* Initialize MMU context and page size but skip mmu_hal_unmap_all().
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
