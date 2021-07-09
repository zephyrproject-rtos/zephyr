/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include "soc.h"
#include <soc/rtc_cntl_reg.h>
#include <soc/timer_group_reg.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/corebits.h>

#include <kernel_structs.h>
#include <string.h>
#include <toolchain/gcc.h>
#include <zephyr/types.h>

#include "esp_private/system_internal.h"
#include "esp32s2/rom/cache.h"
#include "soc/gpio_periph.h"
#include "hal/cpu_ll.h"
#include "esp_err.h"
#include "sys/printk.h"

extern void z_cstart(void);
extern void z_bss_zero(void);
extern void rtc_clk_cpu_freq_set_xtal(void);

/*
 * This is written in C rather than assembly since, during the port bring up,
 * Zephyr is being booted by the Espressif bootloader.  With it, the C stack
 * is already set up.
 */
void __attribute__((section(".iram1"))) __start(void)
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
	cache_size_t cache_size;
	cache_ways_t cache_ways;
	cache_line_size_t cache_line_size;

#if CONFIG_ESP32S2_INSTRUCTION_CACHE_8KB
	esp_rom_Cache_Allocate_SRAM(CACHE_MEMORY_ICACHE_LOW, CACHE_MEMORY_INVALID,
			CACHE_MEMORY_INVALID, CACHE_MEMORY_INVALID);
	cache_size = CACHE_SIZE_8KB;
#else
	esp_rom_Cache_Allocate_SRAM(CACHE_MEMORY_ICACHE_LOW, CACHE_MEMORY_ICACHE_HIGH,
			CACHE_MEMORY_INVALID, CACHE_MEMORY_INVALID);
	cache_size = CACHE_SIZE_16KB;
#endif

	cache_ways = CACHE_4WAYS_ASSOC;

#if CONFIG_ESP32S2_INSTRUCTION_CACHE_LINE_16B
	cache_line_size = CACHE_LINE_SIZE_16B;
#else
	cache_line_size = CACHE_LINE_SIZE_32B;
#endif

	esp_rom_Cache_Suspend_ICache();
	esp_rom_Cache_Set_ICache_Mode(cache_size, cache_ways, cache_line_size);
	esp_rom_Cache_Invalidate_ICache_All();
	esp_rom_Cache_Resume_ICache(0);

#if !CONFIG_BOOTLOADER_ESP_IDF
	/* The watchdog timer is enabled in the 1st stage (ROM) bootloader.
	 * We're done booting, so disable it.
	 * If 2nd stage bootloader from IDF is enabled, then that will take
	 * care of this.
	 */
	volatile uint32_t *wdt_timg_protect = (uint32_t *)TIMG_WDTWPROTECT_REG(0);
	volatile uint32_t *wdt_timg_reg = (uint32_t *)TIMG_WDTCONFIG0_REG(0);

	*wdt_rtc_protect = RTC_CNTL_WDT_WKEY_VALUE;
	*wdt_rtc_reg &= ~RTC_CNTL_WDT_FLASHBOOT_MOD_EN;
	*wdt_rtc_protect = 0;
	*wdt_timg_protect = TIMG_WDT_WKEY_VALUE;
	*wdt_timg_reg &= ~TIMG_WDT_FLASHBOOT_MOD_EN;
	*wdt_timg_protect = 0;
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

#if CONFIG_BOOTLOADER_ESP_IDF
	/* ESP-IDF 2nd stage bootloader enables RTC WDT to check on startup sequence
	 * related issues in application. Hence disable that as we are about to start
	 * Zephyr environment.
	 */
	*wdt_rtc_protect = RTC_CNTL_WDT_WKEY_VALUE;
	*wdt_rtc_reg &= ~RTC_CNTL_WDT_EN;
	*wdt_rtc_protect = 0;
#endif

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
				DPORT_BB_RST | DPORT_FE_RST | DPORT_MAC_RST |
				DPORT_SDIO_RST | DPORT_SDIO_HOST_RST |
				DPORT_EMAC_RST | DPORT_MACPWR_RST);
	DPORT_REG_WRITE(DPORT_CORE_RST_EN_REG, 0);

	/* Reset timer/spi/uart */
	DPORT_SET_PERI_REG_MASK(
		DPORT_PERIP_RST_EN_REG,
		DPORT_TIMERS_RST | DPORT_SPI01_RST | DPORT_SPI2_RST |
		DPORT_SPI3_RST | DPORT_SPI2_DMA_RST | DPORT_SPI3_DMA_RST |
		DPORT_UART_RST);
	DPORT_REG_WRITE(DPORT_PERIP_RST_EN_REG, 0);

	/* Set CPU back to XTAL source, no PLL, same as hard reset */
	rtc_clk_cpu_freq_set_xtal();

	/* Reset CPUs */
	if (core_id == 0) {
		SET_PERI_REG_MASK(RTC_CNTL_OPTIONS0_REG, RTC_CNTL_SW_PROCPU_RST_M);
	}

	while (true) {
		;
	}
}
