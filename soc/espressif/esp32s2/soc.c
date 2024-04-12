/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include "soc.h"
#include <soc/rtc_cntl_reg.h>
#include <soc/timer_group_reg.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/corebits.h>
#include <esp_private/spi_flash_os.h>
#include <esp_private/esp_mmu_map_private.h>
#include <esp_flash_internal.h>
#if CONFIG_ESP_SPIRAM
#include <esp_psram.h>
#include <esp_private/esp_psram_extram.h>
#endif

#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <string.h>
#include <zephyr/toolchain.h>
#include <zephyr/types.h>

#include <esp_private/system_internal.h>
#include <esp32s2/rom/cache.h>
#include <soc/gpio_periph.h>
#include <esp_cpu.h>
#include <hal/cpu_hal.h>
#include <hal/soc_hal.h>
#include <hal/wdt_hal.h>
#include <esp_timer.h>
#include <esp_err.h>
#include <esp_clk_internal.h>
#include <zephyr/sys/printk.h>

#ifdef CONFIG_MCUBOOT
#include "bootloader_init.h"
#endif /* CONFIG_MCUBOOT */

extern void rtc_clk_cpu_freq_set_xtal(void);
extern void esp_reset_reason_init(void);

#if CONFIG_ESP_SPIRAM
extern int _ext_ram_bss_start;
extern int _ext_ram_bss_end;
#endif

/*
 * This is written in C rather than assembly since, during the port bring up,
 * Zephyr is being booted by the Espressif bootloader.  With it, the C stack
 * is already set up.
 */
void __attribute__((section(".iram1"))) __esp_platform_start(void)
{
	extern uint32_t _init_start;

	/* Move the exception vector table to IRAM. */
	__asm__ __volatile__("wsr %0, vecbase" : : "r"(&_init_start));

	/* Zero out BSS */
	z_bss_zero();

	/* Disable normal interrupts. */
	__asm__ __volatile__("wsr %0, PS" : : "r"(PS_INTLEVEL(XCHAL_EXCM_LEVEL) | PS_UM | PS_WOE));

	/* Initialize the architecture CPU pointer.  Some of the
	 * initialization code wants a valid _current before
	 * arch_kernel_init() is invoked.
	 */
	__asm__ __volatile__("wsr.MISC0 %0; rsync" : : "r"(&_kernel.cpus[0]));

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

#ifdef CONFIG_SOC_FLASH_ESP32
	esp_mspi_pin_init();
	spi_flash_init_chip_state();
#endif /* CONFIG_SOC_FLASH_ESP32 */

	esp_mmu_map_init();

#if CONFIG_ESP_SPIRAM
	esp_err_t err = esp_psram_init();

	if (err != ESP_OK) {
		printk("Failed to Initialize SPIRAM, aborting.\n");
		abort();
	}
	if (esp_psram_get_size() < CONFIG_ESP_SPIRAM_SIZE) {
		printk("SPIRAM size is less than configured size, aborting.\n");
		abort();
	}

	if (esp_psram_is_initialized()) {
		if (!esp_psram_extram_test()) {
			printk("External RAM failed memory test!");
			abort();
		}
	}

	memset(&_ext_ram_bss_start, 0,
	       (&_ext_ram_bss_end - &_ext_ram_bss_start) * sizeof(_ext_ram_bss_start));

#endif /* CONFIG_ESP_SPIRAM */

	/* Configures the CPU clock, RTC slow and fast clocks, and performs
	 * RTC slow clock calibration.
	 */
	esp_clk_init();
	esp_timer_early_init();

	/* Scheduler is not started at this point. Hence, guard functions
	 * must be initialized after esp_spiram_init_cache which internally
	 * uses guard functions. Setting guard functions before SPIRAM
	 * cache initialization will result in a crash.
	 */
#if CONFIG_SOC_FLASH_ESP32 || CONFIG_ESP_SPIRAM
	spi_flash_guard_set(&g_flash_guard_default_ops);
#endif
#endif /* !CONFIG_MCUBOOT */

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
