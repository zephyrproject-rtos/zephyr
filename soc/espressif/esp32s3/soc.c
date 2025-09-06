/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2025 Espressif Systems (Shanghai) CO LTD.
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

static void IRAM_ATTR esp_errata(void)
{
	/* Handle the clock gating fix */
	REG_CLR_BIT(SYSTEM_CORE_1_CONTROL_0_REG, SYSTEM_CONTROL_CORE_1_CLKGATE_EN);
	/* The clock gating signal of the App core is invalid. We use RUNSTALL and RESETTING
	 * signals to ensure that the App core stops running in single-core mode.
	 */
	REG_SET_BIT(SYSTEM_CORE_1_CONTROL_0_REG, SYSTEM_CONTROL_CORE_1_RUNSTALL);
	REG_CLR_BIT(SYSTEM_CORE_1_CONTROL_0_REG, SYSTEM_CONTROL_CORE_1_RESETTING);

	/* Handle the Dcache case following the IDF startup code */
#if CONFIG_ESP32S3_DATA_CACHE_16KB
	Cache_Invalidate_DCache_All();
	Cache_Occupy_Addr(SOC_DROM_LOW, 0x4000);
#endif
}

void IRAM_ATTR __esp_platform_app_start(void)
{
	/* Configure the mode of instruction cache : cache size, cache line size. */
	esp_config_instruction_cache_mode();

	/* If we need use SPIRAM, we should use data cache.
	 * Configure the mode of data : cache size, cache line size.
	 */
	esp_config_data_cache_mode();

	/* Apply SoC patches */
	esp_errata();

	esp_reset_reason_init();

	esp_timer_early_init();

	esp_flash_config();

	esp_efuse_init_virtual();

#if CONFIG_ESP_SPIRAM
	esp_init_psram();

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
