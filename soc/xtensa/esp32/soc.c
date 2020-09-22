/*
 * Copyright (c) 2017 Intel Corporation
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
#include "esp32/rom/cache.h"
#include "hal/soc_ll.h"
#include "soc/cpu.h"
#include "soc/gpio_periph.h"

extern void z_cstart(void);

/*
 * This is written in C rather than assembly since, during the port bring up,
 * Zephyr is being booted by the Espressif bootloader.  With it, the C stack
 * is already set up.
 */
void __attribute__((section(".iram1"))) __start(void)
{
	volatile uint32_t *wdt_rtc_protect = (uint32_t *)RTC_CNTL_WDTWPROTECT_REG;
	volatile uint32_t *wdt_rtc_reg = (uint32_t *)RTC_CNTL_WDTCONFIG0_REG;
	volatile uint32_t *app_cpu_config_reg = (uint32_t *)DPORT_APPCPU_CTRL_B_REG;
	extern uint32_t _init_start;
	extern uint32_t _bss_start;
	extern uint32_t _bss_end;

	/* Move the exception vector table to IRAM. */
	__asm__ __volatile__ (
		"wsr %0, vecbase"
		:
		: "r"(&_init_start));

	/* Zero out BSS.  Clobber _bss_start to avoid memset() elision. */
	(void)memset(&_bss_start, 0,
		     (&_bss_end - &_bss_start) * sizeof(_bss_start));
	__asm__ __volatile__ (
		""
		:
		: "g"(&_bss_start)
		: "memory");

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

	/* Disable CPU1 while we figure out how to have SMP in Zephyr. */
	*app_cpu_config_reg &= ~DPORT_APPCPU_CLKGATE_EN;

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
int arch_printk_char_out(int c)
{
	if (c == '\n') {
		esp32_rom_uart_tx_one_char('\r');
	}
	esp32_rom_uart_tx_one_char(c);
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

	const uint32_t core_id = cpu_hal_get_core_id();
	const uint32_t other_core_id = (core_id == 0) ? 1 : 0;

	soc_ll_reset_core(other_core_id);
	soc_ll_stall_core(other_core_id);

	/* Flush any data left in UART FIFOs */
	esp32_rom_uart_tx_wait_idle(0);
	esp32_rom_uart_tx_wait_idle(1);
	esp32_rom_uart_tx_wait_idle(2);

	/* Disable cache */
	Cache_Read_Disable(0);
	Cache_Read_Disable(1);

	/* 2nd stage bootloader reconfigures SPI flash signals. */
	/* Reset them to the defaults expected by ROM */
	WRITE_PERI_REG(GPIO_FUNC0_IN_SEL_CFG_REG, 0x30);
	WRITE_PERI_REG(GPIO_FUNC1_IN_SEL_CFG_REG, 0x30);
	WRITE_PERI_REG(GPIO_FUNC2_IN_SEL_CFG_REG, 0x30);
	WRITE_PERI_REG(GPIO_FUNC3_IN_SEL_CFG_REG, 0x30);
	WRITE_PERI_REG(GPIO_FUNC4_IN_SEL_CFG_REG, 0x30);
	WRITE_PERI_REG(GPIO_FUNC5_IN_SEL_CFG_REG, 0x30);

	/* Reset wifi/bluetooth/ethernet/sdio (bb/mac) */
	DPORT_SET_PERI_REG_MASK(DPORT_CORE_RST_EN_REG,
				DPORT_BB_RST | DPORT_FE_RST | DPORT_MAC_RST |
				DPORT_BT_RST | DPORT_BTMAC_RST |
				DPORT_SDIO_RST | DPORT_SDIO_HOST_RST |
				DPORT_EMAC_RST | DPORT_MACPWR_RST |
				DPORT_RW_BTMAC_RST | DPORT_RW_BTLP_RST);
	DPORT_REG_WRITE(DPORT_CORE_RST_EN_REG, 0);

	/* Reset timer/spi/uart */
	DPORT_SET_PERI_REG_MASK(
		DPORT_PERIP_RST_EN_REG,
		/* UART TX FIFO cannot be reset correctly on ESP32, */
		/* so reset the UART memory by DPORT here. */
		DPORT_TIMERS_RST | DPORT_SPI01_RST | DPORT_UART_RST |
		DPORT_UART1_RST | DPORT_UART2_RST | DPORT_UART_MEM_RST);
	DPORT_REG_WRITE(DPORT_PERIP_RST_EN_REG, 0);

	/* Clear entry point for APP CPU */
	DPORT_REG_WRITE(DPORT_APPCPU_CTRL_D_REG, 0);

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
