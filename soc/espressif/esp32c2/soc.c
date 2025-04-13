/*
 * Copyright (c) 2024-2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <soc_init.h>
#include <flash_init.h>
#include <esp_private/cache_utils.h>
#include <esp_private/system_internal.h>
#include <esp_timer.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>

extern void esp_reset_reason_init(void);

void IRAM_ATTR __esp_platform_app_start(void)
{
	esp_reset_reason_init();

	esp_timer_early_init();

	esp_flash_config();

	/* Start Zephyr */
	z_cstart();

	CODE_UNREACHABLE;
}

void IRAM_ATTR __esp_platform_mcuboot_start(void)
{
	/* Start Zephyr */
	z_cstart();

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
