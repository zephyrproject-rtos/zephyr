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
#include <psram.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/sys/printk.h>

extern void z_prep_c(void);
extern void esp_reset_reason_init(void);

void IRAM_ATTR __esp_platform_app_start(void)
{
	esp_reset_reason_init();

	esp_timer_early_init();

	esp_flash_config();

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
