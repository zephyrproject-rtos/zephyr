/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/printk.h>

#include <kernel_internal.h>
#include <esp_private/system_internal.h>
#include <esp32s2/rom/cache.h>
#include <esp32s2/spiram.h>
#include <esp_cpu.h>
#include <esp_err.h>
#include <esp_spi_flash.h>
#include <esp_timer.h>
#include <esp_clk_internal.h>
#include <hal/cpu_ll.h>
#include <hal/soc_ll.h>
#include <soc.h>
#include <soc/gpio_periph.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/timer_group_reg.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/corebits.h>

extern void rtc_clk_cpu_freq_set_xtal(void);

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
	volatile uint32_t *wdt_rtc_protect = (uint32_t *)RTC_CNTL_WDTWPROTECT_REG;
	volatile uint32_t *wdt_rtc_reg = (uint32_t *)RTC_CNTL_WDTCONFIG0_REG;
	extern uint32_t _init_start;

	/* Move the exception vector table to IRAM. */
	__asm__ __volatile__ (
		"wsr %0, vecbase"
		:
		: "r"(&_init_start));

	/* Zero out BSS */
	z_bss_zero();

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
#if CONFIG_ESP_SPIRAM
	esp_config_data_cache_mode();
	esp_rom_Cache_Enable_DCache(0);
#endif

	/* Disable normal interrupts. */
	__asm__ __volatile__ (
		"wsr %0, PS"
		:
		: "r"(PS_INTLEVEL(XCHAL_EXCM_LEVEL) | PS_UM | PS_WOE));

	/* Initialize the architecture CPU pointer.  Some of the
	 * initialization code wants a valid _current before
	 * arch_kernel_init() is invoked.
	 */
	__asm__ volatile("wsr.MISC0 %0; rsync" : : "r"(&_kernel.cpus[0]));

	/* ESP-IDF 2nd stage bootloader enables RTC WDT to check on startup sequence
	 * related issues in application. Hence disable that as we are about to start
	 * Zephyr environment.
	 */
	*wdt_rtc_protect = RTC_CNTL_WDT_WKEY_VALUE;
	*wdt_rtc_reg &= ~RTC_CNTL_WDT_EN;
	*wdt_rtc_protect = 0;

	/* Configures the CPU clock, RTC slow and fast clocks, and performs
	 * RTC slow clock calibration.
	 */
	esp_clk_init();

	esp_timer_early_init();

#if CONFIG_ESP_SPIRAM

	memset(&_ext_ram_bss_start,
		0,
		(&_ext_ram_bss_end - &_ext_ram_bss_start) * sizeof(_ext_ram_bss_start));

	esp_err_t err = esp_spiram_init();

	if (err != ESP_OK) {
		printk("Failed to Initialize SPIRAM, aborting.\n");
		abort();
	}
	esp_spiram_init_cache();
	if (esp_spiram_get_size() < CONFIG_ESP_SPIRAM_SIZE) {
		printk("SPIRAM size is less than configured size, aborting.\n");
		abort();
	}

#endif /* CONFIG_ESP_SPIRAM */

/* Scheduler is not started at this point. Hence, guard functions
 * must be initialized after esp_spiram_init_cache which internally
 * uses guard functions. Setting guard functions before SPIRAM
 * cache initialization will result in a crash.
 */
#if CONFIG_SOC_FLASH_ESP32 || CONFIG_ESP_SPIRAM
	spi_flash_guard_set(&g_flash_guard_default_ops);
#endif
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
	/* Disable interrupts */
	z_xt_ints_off(0xFFFFFFFF);

	/*
	 * Reset and stall the other CPU.
	 * CPU must be reset before stalling, in case it was running a s32c1i
	 * instruction. This would cause memory pool to be locked by arbiter
	 * to the stalled CPU, preventing current CPU from accessing this pool.
	 */
	const uint32_t core_id = cpu_ll_get_core_id();

	/* Flush any data left in UART FIFOs */
	esp_rom_uart_tx_wait_idle(0);
	esp_rom_uart_tx_wait_idle(1);
	/* Disable cache */
	esp_rom_Cache_Disable_ICache();
	esp_rom_Cache_Disable_DCache();

	/*
	 * 2nd stage bootloader reconfigures SPI flash signals.
	 * Reset them to the defaults expected by ROM
	 */
	WRITE_PERI_REG(GPIO_FUNC0_IN_SEL_CFG_REG, 0x30);
	WRITE_PERI_REG(GPIO_FUNC1_IN_SEL_CFG_REG, 0x30);
	WRITE_PERI_REG(GPIO_FUNC2_IN_SEL_CFG_REG, 0x30);
	WRITE_PERI_REG(GPIO_FUNC3_IN_SEL_CFG_REG, 0x30);
	WRITE_PERI_REG(GPIO_FUNC4_IN_SEL_CFG_REG, 0x30);
	WRITE_PERI_REG(GPIO_FUNC5_IN_SEL_CFG_REG, 0x30);

	/* Reset wifi/ethernet/sdio (bb/mac) */
	DPORT_SET_PERI_REG_MASK(DPORT_CORE_RST_EN_REG,
				DPORT_BB_RST | DPORT_FE_RST | DPORT_MAC_RST | DPORT_BT_RST |
				DPORT_BTMAC_RST | DPORT_SDIO_RST | DPORT_SDIO_RST |
				DPORT_SDIO_HOST_RST | DPORT_EMAC_RST | DPORT_MACPWR_RST |
				DPORT_RW_BTMAC_RST | DPORT_RW_BTLP_RST);
	DPORT_REG_WRITE(DPORT_CORE_RST_EN_REG, 0);

	/* Reset timer/spi/uart */
	DPORT_SET_PERI_REG_MASK(
		DPORT_PERIP_RST_EN_REG,
		DPORT_TIMERS_RST | DPORT_SPI01_RST | DPORT_SPI2_RST |
		DPORT_SPI3_RST | DPORT_SPI2_DMA_RST | DPORT_SPI3_DMA_RST |
		DPORT_UART_RST);
	DPORT_REG_WRITE(DPORT_PERIP_RST_EN_REG, 0);

	/* Reset CPUs */
	if (core_id == 0) {
		soc_ll_reset_core(0);
	}

	while (true) {
		;
	}
}
