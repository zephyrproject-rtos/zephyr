/*
 * Copyright (c) 2021-2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <soc_init.h>
#include <flash_init.h>
#include <esp_private/cache_utils.h>
#include <esp_private/system_internal.h>
#include <esp_timer.h>
#include <efuse_virtual.h>
#include <psram.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/sys/printk.h>

extern FUNC_NORETURN void z_prep_c(void);
extern void esp_reset_reason_init(void);

void IRAM_ATTR __esp_platform_app_start(void)
{
	/*
	 * Configure the mode of instruction cache :
	 * cache size, cache associated ways, cache line size.
	 */
	esp_config_instruction_cache_mode();

	/*
	 * If we need use SPIRAM, we should use data cache, or if we want to
	 * access rodata, we also should use data cache.
	 * Configure the mode of data : cache size, cache associated ways, cache
	 * line size.
	 * Enable data cache, so if we don't use SPIRAM, it just works.
	 */
	esp_config_data_cache_mode();
	esp_rom_Cache_Enable_DCache(0);

	esp_reset_reason_init();

	esp_timer_early_init();

	esp_flash_config();

	esp_efuse_init_virtual();

#if CONFIG_ESP_SPIRAM
	esp_init_psram();

	/* Init Shared Multi Heap for PSRAM */
	int err = esp_psram_smh_init();

	if (err) {
		printk("Failed to initialize PSRAM shared multi heap (%d)\n", err);
	}
#endif

	/* Start Zephyr */
	z_prep_c();

	CODE_UNREACHABLE;
}

void IRAM_ATTR __esp_platform_mcuboot_start(void)
{
	/* Start Zephyr */
	z_prep_c();

	CODE_UNREACHABLE;
}

/* Boot-time static default printk handler, possibly to be overridden later. */
int IRAM_ATTR arch_printk_char_out(int c)
{
	if (c == '\n') {
		esp_rom_uart_tx_one_char('\r');
	}
	esp_rom_uart_tx_one_char(c);
	return 0;
}

void sys_arch_reboot(int type)
{
	esp_restart();
}
