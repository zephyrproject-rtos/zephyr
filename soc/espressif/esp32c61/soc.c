/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
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
#include <esp_rom_serial_output.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/arch/common/init.h>
#include <hal/interrupt_clic_ll.h>
#include <riscv/interrupt.h>
#include <soc/interrupts.h>
#include <psram.h>
#include <zephyr/sys/printk.h>

extern void esp_reset_reason_init(void);
extern FUNC_NORETURN void z_cstart(void);

static void core_intr_matrix_clear(void)
{
	for (int i = 0; i < ETS_MAX_INTR_SOURCE; i++) {
		interrupt_clic_ll_route(0, i, ETS_INVALID_INUM);
	}

	for (int i = 0; i < 32; i++) {
		esprv_int_set_vectored(i, true);
	}
}

void IRAM_ATTR __esp_platform_app_start(void)
{
	core_intr_matrix_clear();

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

	z_cstart();

	CODE_UNREACHABLE;
}

void IRAM_ATTR __esp_platform_mcuboot_start(void)
{
	z_cstart();

	CODE_UNREACHABLE;
}

int IRAM_ATTR arch_printk_char_out(int c)
{
	if (c == '\n') {
		esp_rom_output_tx_one_char('\r');
	}
	esp_rom_output_tx_one_char(c);
	return 0;
}

void sys_arch_reboot(int type)
{
	esp_restart();
}
