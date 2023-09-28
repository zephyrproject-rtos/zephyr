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

#include <zephyr/kernel_structs.h>
#include <string.h>
#include <zephyr/toolchain.h>
#include <zephyr/types.h>
#include <zephyr/linker/linker-defs.h>
#include <kernel_internal.h>
#include <zephyr/sys/util.h>

#include "esp_private/system_internal.h"
#include "esp32s3/rom/cache.h"
#include "esp32s3/rom/rtc.h"
#include "soc/syscon_reg.h"
#include "hal/soc_ll.h"
#include "hal/wdt_hal.h"
#include "soc/cpu.h"
#include "soc/gpio_periph.h"
#include "esp_spi_flash.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_app_format.h"
#include "esp_clk_internal.h"

#include "esp32s3/spiram.h"

#ifdef CONFIG_MCUBOOT
#include "bootloader_init.h"
#endif /* CONFIG_MCUBOOT */
#include <zephyr/sys/printk.h>

#if CONFIG_ESP_SPIRAM
extern int _ext_ram_bss_start;
extern int _ext_ram_bss_end;
#endif

extern void z_cstart(void);

#ifdef CONFIG_SOC_ESP32S3_PROCPU
extern const unsigned char esp32s3_appcpu_fw_array[];

void IRAM_ATTR esp_start_appcpu(void)
{
	esp_image_header_t *header = (esp_image_header_t *)&esp32s3_appcpu_fw_array[0];
	esp_image_segment_header_t *segment =
		(esp_image_segment_header_t *)&esp32s3_appcpu_fw_array[sizeof(esp_image_header_t)];
	uint8_t *segment_payload;
	uint32_t entry_addr = header->entry_addr;
	uint32_t idx = sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t);

	for (int i = 0; i < header->segment_count; i++) {
		segment_payload = (uint8_t *)&esp32s3_appcpu_fw_array[idx];

		if (segment->load_addr >= SOC_IRAM_LOW && segment->load_addr < SOC_IRAM_HIGH) {
			/* IRAM segment only accepts 4 byte access, avoid memcpy usage here */
			volatile uint32_t *src = (volatile uint32_t *)segment_payload;
			volatile uint32_t *dst = (volatile uint32_t *)segment->load_addr;

			for (int i = 0; i < segment->data_len / 4; i++) {
				dst[i] = src[i];
			}

		} else if (segment->load_addr >= SOC_DRAM_LOW &&
			   segment->load_addr < SOC_DRAM_HIGH) {
			memcpy((void *)segment->load_addr, (const void *)segment_payload,
			       segment->data_len);
		}

		idx += segment->data_len;
		segment = (esp_image_segment_header_t *)&esp32s3_appcpu_fw_array[idx];
		idx += sizeof(esp_image_segment_header_t);
	}

	esp_appcpu_start((void *)entry_addr);
}
#endif /* CONFIG_SOC_ESP32S3_PROCPU*/

#ifndef CONFIG_MCUBOOT
/*
 * This function is a container for SoC patches
 * that needs to be applied during the startup.
 */
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
#endif /* CONFIG_MCUBOOT */

/*
 * This is written in C rather than assembly since, during the port bring up,
 * Zephyr is being booted by the Espressif bootloader.  With it, the C stack
 * is already set up.
 */
void IRAM_ATTR __esp_platform_start(void)
{
	extern uint32_t _init_start;

	/* Move the exception vector table to IRAM. */
	__asm__ __volatile__("wsr %0, vecbase" : : "r"(&_init_start));

	z_bss_zero();

	/* Disable normal interrupts. */
	__asm__ __volatile__("wsr %0, PS" : : "r"(PS_INTLEVEL(XCHAL_EXCM_LEVEL) | PS_UM | PS_WOE));

	/* Initialize the architecture CPU pointer.  Some of the
	 * initialization code wants a valid _current before
	 * arch_kernel_init() is invoked.
	 */
	__asm__ __volatile__("wsr.MISC0 %0; rsync" : : "r"(&_kernel.cpus[0]));

#ifdef CONFIG_MCUBOOT
	/* MCUboot early initialisation. */
	if (bootloader_init()) {
		abort();
	}
#else
	/* Configure the mode of instruction cache : cache size, cache line size. */
	esp_config_instruction_cache_mode();

	/* If we need use SPIRAM, we should use data cache.
	 * Configure the mode of data : cache size, cache line size.
	 */
	esp_config_data_cache_mode();

	/* Apply SoC patches */
	esp_errata();

#if CONFIG_ESP_SPIRAM
	esp_err_t err = esp_spiram_init();

	if (err != ESP_OK) {
		printk("Failed to Initialize external RAM, aborting.\n");
		abort();
	}

	esp_spiram_init_cache();
	if (esp_spiram_get_size() < CONFIG_ESP_SPIRAM_SIZE) {
		printk("External RAM size is less than configured, aborting.\n");
		abort();
	}

	if (!esp_spiram_test()) {
		printk("External RAM failed memory test!\n");
		abort();
	}

	memset(&_ext_ram_bss_start, 0,
	       (&_ext_ram_bss_end - &_ext_ram_bss_start) * sizeof(_ext_ram_bss_start));

#endif /* CONFIG_ESP_SPIRAM */

	/* ESP-IDF/MCUboot 2nd stage bootloader enables RTC WDT to check on startup sequence
	 * related issues in application. Hence disable that as we are about to start
	 * Zephyr environment.
	 */
	wdt_hal_context_t rtc_wdt_ctx = {.inst = WDT_RWDT, .rwdt_dev = &RTCCNTL};

	wdt_hal_write_protect_disable(&rtc_wdt_ctx);
	wdt_hal_disable(&rtc_wdt_ctx);
	wdt_hal_write_protect_enable(&rtc_wdt_ctx);

	esp_clk_init();

	esp_timer_early_init();

#if CONFIG_SOC_ESP32S3_PROCPU
	/* start the ESP32S3 APP CPU */
	esp_start_appcpu();
#endif

#if CONFIG_SOC_FLASH_ESP32
	spi_flash_guard_set(&g_flash_guard_default_ops);
#endif
#endif /* CONFIG_MCUBOOT */

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

void IRAM_ATTR esp_restart_noos(void)
{
	/* disable interrupts */
	z_xt_ints_off(0xFFFFFFFF);

	/* enable RTC watchdog for 1 second */
	wdt_hal_context_t wdt_ctx;
	uint32_t timeout_ticks = (uint32_t)(1000ULL * rtc_clk_slow_freq_get_hz() / 1000ULL);

	wdt_hal_init(&wdt_ctx, WDT_RWDT, 0, false);
	wdt_hal_write_protect_disable(&wdt_ctx);
	wdt_hal_config_stage(&wdt_ctx, WDT_STAGE0, timeout_ticks, WDT_STAGE_ACTION_RESET_SYSTEM);
	wdt_hal_config_stage(&wdt_ctx, WDT_STAGE1, timeout_ticks, WDT_STAGE_ACTION_RESET_RTC);

	/* enable flash boot mode so that flash booting after restart is protected by the RTC WDT */
	wdt_hal_set_flashboot_en(&wdt_ctx, true);
	wdt_hal_write_protect_enable(&wdt_ctx);

	/* disable TG0/TG1 watchdogs */
	wdt_hal_context_t wdt0_context = {.inst = WDT_MWDT0, .mwdt_dev = &TIMERG0};

	wdt_hal_write_protect_disable(&wdt0_context);
	wdt_hal_disable(&wdt0_context);
	wdt_hal_write_protect_enable(&wdt0_context);

	wdt_hal_context_t wdt1_context = {.inst = WDT_MWDT1, .mwdt_dev = &TIMERG1};

	wdt_hal_write_protect_disable(&wdt1_context);
	wdt_hal_disable(&wdt1_context);
	wdt_hal_write_protect_enable(&wdt1_context);

	/* Flush any data left in UART FIFOs */
	esp_rom_uart_tx_wait_idle(0);
	esp_rom_uart_tx_wait_idle(1);
	esp_rom_uart_tx_wait_idle(2);

	/* Disable cache */
	Cache_Disable_ICache();
	Cache_Disable_DCache();

	const uint32_t core_id = cpu_hal_get_core_id();
#if CONFIG_SMP
	const uint32_t other_core_id = (core_id == 0) ? 1 : 0;

	soc_ll_reset_core(other_core_id);
	soc_ll_stall_core(other_core_id);
#endif

	/* 2nd stage bootloader reconfigures SPI flash signals. */
	/* Reset them to the defaults expected by ROM */
	WRITE_PERI_REG(GPIO_FUNC0_IN_SEL_CFG_REG, 0x30);
	WRITE_PERI_REG(GPIO_FUNC1_IN_SEL_CFG_REG, 0x30);
	WRITE_PERI_REG(GPIO_FUNC2_IN_SEL_CFG_REG, 0x30);
	WRITE_PERI_REG(GPIO_FUNC3_IN_SEL_CFG_REG, 0x30);
	WRITE_PERI_REG(GPIO_FUNC4_IN_SEL_CFG_REG, 0x30);
	WRITE_PERI_REG(GPIO_FUNC5_IN_SEL_CFG_REG, 0x30);

	/* Reset wifi/bluetooth/ethernet/sdio (bb/mac) */
	SET_PERI_REG_MASK(SYSTEM_CORE_RST_EN_REG,
			  SYSTEM_BB_RST | SYSTEM_FE_RST | SYSTEM_MAC_RST | SYSTEM_BT_RST |
				  SYSTEM_BTMAC_RST | SYSTEM_SDIO_RST | SYSTEM_SDIO_HOST_RST |
				  SYSTEM_EMAC_RST | SYSTEM_MACPWR_RST | SYSTEM_RW_BTMAC_RST |
				  SYSTEM_RW_BTLP_RST | SYSTEM_BLE_REG_RST | SYSTEM_PWR_REG_RST);
	REG_WRITE(SYSTEM_CORE_RST_EN_REG, 0);

	/* Reset timer/spi/uart */
	SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN0_REG, SYSTEM_TIMERS_RST | SYSTEM_SPI01_RST |
							    SYSTEM_UART_RST | SYSTEM_SYSTIMER_RST);
	REG_WRITE(SYSTEM_PERIP_RST_EN0_REG, 0);

	/* Reset DMA */
	SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN1_REG, SYSTEM_DMA_RST);
	REG_WRITE(SYSTEM_PERIP_RST_EN1_REG, 0);

	SET_PERI_REG_MASK(SYSTEM_EDMA_CTRL_REG, SYSTEM_EDMA_RESET);
	CLEAR_PERI_REG_MASK(SYSTEM_EDMA_CTRL_REG, SYSTEM_EDMA_RESET);

	rtc_clk_cpu_freq_set_xtal();

	/* Reset CPUs */
	if (core_id == 0) {
		/* Running on PRO CPU: APP CPU is stalled. Can reset both CPUs. */
		soc_ll_reset_core(1);
		soc_ll_reset_core(0);
	} else {
		/* Running on APP CPU: need to reset PRO CPU and unstall it, */
		/* then reset APP CPU */
		soc_ll_reset_core(0);
		soc_ll_stall_core(0);
		soc_ll_reset_core(1);
	}

	while (true) {
		;
	}
}
