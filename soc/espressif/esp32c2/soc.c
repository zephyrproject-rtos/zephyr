/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <soc/rtc_cntl_reg.h>
#include <soc/timer_group_reg.h>
#include <soc/ext_mem_defs.h>
#include <soc/gpio_reg.h>
#include <soc/syscon_reg.h>
#include <soc/system_reg.h>
#include "hal/wdt_hal.h"
#include "esp_cpu.h"
#include "hal/soc_hal.h"
#include "hal/cpu_hal.h"
#include "esp_timer.h"
#include "esp_private/system_internal.h"
#include "esp_clk_internal.h"
#include <soc/interrupt_reg.h>
#include <esp_private/spi_flash_os.h>
#include "esp_private/esp_mmu_map_private.h"
#include <esp_flash_internal.h>

#include <zephyr/drivers/interrupt_controller/intc_esp32c3.h>

#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <string.h>
#include <zephyr/toolchain.h>
#include <soc.h>

extern void esp_reset_reason_init(void);

/*
 * This is written in C rather than assembly since, during the port bring up,
 * Zephyr is being booted by the Espressif bootloader.  With it, the C stack
 * is already set up.
 */
void __attribute__((section(".iram1"))) __esp_platform_start(void)
{
	__asm__ __volatile__("la t0, _esp32c2_vector_table\n"
						"csrw mtvec, t0\n");

	z_bss_zero();

	/* Disable normal interrupts. */
	csr_read_clear(mstatus, MSTATUS_MIE);

	esp_reset_reason_init();

#ifndef CONFIG_MCUBOOT
	/* ESP-IDF 2nd stage bootloader enables RTC WDT to check on startup sequence
	 * related issues in application. Hence disable that as we are about to start
	 * Zephyr environment.
	 */
	wdt_hal_context_t rtc_wdt_ctx = {.inst = WDT_RWDT, .rwdt_dev = &RTCCNTL};

	wdt_hal_write_protect_disable(&rtc_wdt_ctx);
	wdt_hal_disable(&rtc_wdt_ctx);
	wdt_hal_write_protect_enable(&rtc_wdt_ctx);

	/* Enable wireless phy subsystem clock,
	 * This needs to be done before the kernel starts
	 */
	REG_CLR_BIT(SYSTEM_WIFI_CLK_EN_REG, SYSTEM_WIFI_CLK_SDIOSLAVE_EN);
	SET_PERI_REG_MASK(SYSTEM_WIFI_CLK_EN_REG, SYSTEM_WIFI_CLK_EN);

	esp_timer_early_init();

	esp_mspi_pin_init();

	esp_flash_app_init();

	esp_mmu_map_init();

#endif /* !CONFIG_MCUBOOT */

	/*Initialize the esp32c2 interrupt controller */
	esp_intr_initialize();

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
	esp_restart_noos();
}
