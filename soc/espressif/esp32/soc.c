/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <soc.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/timer_group_reg.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/corebits.h>
#include <esp_private/spi_flash_os.h>
#include <esp_private/esp_mmu_map_private.h>
#if CONFIG_ESP_SPIRAM
#include <esp_psram.h>
#include <esp_private/esp_psram_extram.h>
#endif

#include <zephyr/kernel_structs.h>
#include <string.h>
#include <zephyr/toolchain.h>
#include <zephyr/types.h>
#include <zephyr/linker/linker-defs.h>
#include <kernel_internal.h>

#include <esp_private/system_internal.h>
#include <esp32/rom/cache.h>
#include <esp_cpu.h>
#include <hal/soc_hal.h>
#include <hal/cpu_hal.h>
#include <soc/gpio_periph.h>
#include <esp_err.h>
#include <esp_timer.h>
#include <hal/wdt_hal.h>
#include <esp_app_format.h>

#ifndef CONFIG_SOC_ENABLE_APPCPU
#include "esp_clk_internal.h"
#endif /* CONFIG_SOC_ENABLE_APPCPU */

#ifdef CONFIG_MCUBOOT
#include "bootloader_init.h"
#endif /* CONFIG_MCUBOOT */
#include <zephyr/sys/printk.h>

#if CONFIG_ESP_SPIRAM
extern int _ext_ram_bss_start;
extern int _ext_ram_bss_end;
#endif

extern void z_cstart(void);
extern void esp_reset_reason_init(void);

#ifdef CONFIG_SOC_ENABLE_APPCPU
extern const unsigned char esp32_appcpu_fw_array[];

void IRAM_ATTR esp_start_appcpu(void)
{
	esp_image_header_t *header = (esp_image_header_t *)&esp32_appcpu_fw_array[0];
	esp_image_segment_header_t *segment =
		(esp_image_segment_header_t *)&esp32_appcpu_fw_array[sizeof(esp_image_header_t)];
	uint8_t *segment_payload;
	uint32_t entry_addr = header->entry_addr;
	uint32_t idx = sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t);

	for (int i = 0; i < header->segment_count; i++) {
		segment_payload = (uint8_t *)&esp32_appcpu_fw_array[idx];

		if (segment->load_addr >= SOC_IRAM_LOW && segment->load_addr < SOC_IRAM_HIGH) {
			/* IRAM segment only accepts 4 byte access, avoid memcpy usage here */
			volatile uint32_t *src = (volatile uint32_t *)segment_payload;
			volatile uint32_t *dst = (volatile uint32_t *)segment->load_addr;

			for (int j = 0; j < segment->data_len / 4; j++) {
				dst[j] = src[j];
			}
		} else if (segment->load_addr >= SOC_DRAM_LOW &&
			   segment->load_addr < SOC_DRAM_HIGH) {

			memcpy((void *)segment->load_addr, (const void *)segment_payload,
			       segment->data_len);
		}

		idx += segment->data_len;
		segment = (esp_image_segment_header_t *)&esp32_appcpu_fw_array[idx];
		idx += sizeof(esp_image_segment_header_t);
	}

	esp_appcpu_start((void *)entry_addr);
}
#endif /* CONFIG_SOC_ENABLE_APPCPU */

/*
 * This is written in C rather than assembly since, during the port bring up,
 * Zephyr is being booted by the Espressif bootloader.  With it, the C stack
 * is already set up.
 */
void IRAM_ATTR __esp_platform_start(void)
{
	extern uint32_t _init_start;

	/* Move the exception vector table to IRAM. */
	__asm__ __volatile__ (
		"wsr %0, vecbase"
		:
		: "r"(&_init_start));

	z_bss_zero();

	__asm__ __volatile__ (
		""
		:
		: "g"(&__bss_start)
		: "memory");

	/* Disable normal interrupts. */
	__asm__ __volatile__ (
		"wsr %0, PS"
		:
		: "r"(PS_INTLEVEL(XCHAL_EXCM_LEVEL) | PS_UM | PS_WOE));

	/* Initialize the architecture CPU pointer.  Some of the
	 * initialization code wants a valid _current before
	 * arch_kernel_init() is invoked.
	 */
	__asm__ __volatile__("wsr.MISC0 %0; rsync" : : "r"(&_kernel.cpus[0]));

	esp_reset_reason_init();

#ifndef CONFIG_MCUBOOT
	/* ESP-IDF/MCUboot 2nd stage bootloader enables RTC WDT to check
	 * on startup sequence related issues in application. Hence disable that
	 * as we are about to start Zephyr environment.
	 */
	wdt_hal_context_t rtc_wdt_ctx = {.inst = WDT_RWDT, .rwdt_dev = &RTCCNTL};

	wdt_hal_write_protect_disable(&rtc_wdt_ctx);
	wdt_hal_disable(&rtc_wdt_ctx);
	wdt_hal_write_protect_enable(&rtc_wdt_ctx);

#ifndef CONFIG_SOC_ENABLE_APPCPU
	/* Configures the CPU clock, RTC slow and fast clocks, and performs
	 * RTC slow clock calibration.
	 */
	esp_clk_init();
#endif

	esp_timer_early_init();

#if CONFIG_SOC_ENABLE_APPCPU
	/* start the ESP32 APP CPU */
	esp_start_appcpu();
#endif

	esp_mmu_map_init();

#ifdef CONFIG_SOC_FLASH_ESP32
	esp_mspi_pin_init();
	spi_flash_init_chip_state();
#endif

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
