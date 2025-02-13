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

#if XCHAL_ERRATUM_572
	uint32_t memctl = XCHAL_CACHE_MEMCTL_DEFAULT;

	WSR(MEMCTL, memctl);
#endif /*XCHAL_ERRATUM_572*/

#ifdef CONFIG_BOOTLOADER_REGION_PROTECTION_ENABLE
	esp_cpu_configure_region_protection();
#endif
#if CONFIG_BOOTLOADER_VDDSDIO_BOOST_1_9V
	rtc_vddsdio_config_t cfg = rtc_vddsdio_get_config();

	if (cfg.enable == 1 && cfg.tieh == RTC_VDDSDIO_TIEH_1_8V) {
		cfg.drefh = 3;
		cfg.drefm = 3;
		cfg.drefl = 3;
		cfg.force = 1;
		rtc_vddsdio_set_config(cfg);
		esp_rom_delay_us(10);
	}
#endif /* CONFIG_BOOTLOADER_VDDSDIO_BOOST_1_9V */

	bootloader_clock_configure();

#ifdef CONFIG_ESP_CONSOLE
	/* initialize console, from now on, we can log */
	esp_console_init();
	print_banner();
#endif /* CONFIG_ESP_CONSOLE */

	reset_mmu();

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
