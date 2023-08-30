/*
 * Copyright (c) 2023 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/timer_group_reg.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/corebits.h>

#include <zephyr/kernel_structs.h>
#include <string.h>
#include <zephyr/toolchain/gcc.h>
#include <zephyr/types.h>
#include <zephyr/linker/linker-defs.h>
#include <kernel_internal.h>
#include <zephyr/sys/util.h>

#include <esp_private/system_internal.h>
#include <esp32s3/rom/cache.h>
#include <esp32s3/rom/rtc.h>
#include <soc/syscon_reg.h>
#include <hal/soc_ll.h>
#include <hal/wdt_hal.h>
#include <soc/cpu.h>
#include <soc/gpio_periph.h>
#include <esp_spi_flash.h>
#include <esp_err.h>
#include <esp_timer.h>
#include <esp_app_format.h>
#include <esp_clk_internal.h>

extern void z_cstart(void);

static void core_intr_matrix_clear(void)
{
	uint32_t core_id = cpu_hal_get_core_id();

	for (int i = 0; i < ETS_MAX_INTR_SOURCE; i++) {
		intr_matrix_set(core_id, i, ETS_INVALID_INUM);
	}
}

void IRAM_ATTR __app_cpu_start(void)
{
	extern uint32_t _init_start;
	extern uint32_t _bss_start;
	extern uint32_t _bss_end;

	/* Move the exception vector table to IRAM. */
	__asm__ __volatile__("wsr %0, vecbase" : : "r"(&_init_start));

	/* Zero out BSS.  Clobber _bss_start to avoid memset() elision. */
	z_bss_zero();

	__asm__ __volatile__("" : : "g"(&__bss_start) : "memory");

	/* Disable normal interrupts. */
	__asm__ __volatile__("wsr %0, PS" : : "r"(PS_INTLEVEL(XCHAL_EXCM_LEVEL) | PS_UM | PS_WOE));

	/* Initialize the architecture CPU pointer.  Some of the
	 * initialization code wants a valid _current before
	 * arch_kernel_init() is invoked.
	 */
	__asm__ __volatile__("wsr.MISC0 %0; rsync" : : "r"(&_kernel.cpus[1]));

	core_intr_matrix_clear();

	esp_intr_initialize();

	/* Start Zephyr */
	z_cstart();

	CODE_UNREACHABLE;
}

/* Boot-time static default printk handler, possibly to be overridden later. */
int IRAM_ATTR arch_printk_char_out(int c)
{
	ARG_UNUSED(c);
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

	/* Running on APP CPU: need to reset PRO CPU and unstall it, */
	/* then reset APP CPU */
	soc_ll_reset_core(0);
	soc_ll_stall_core(0);
	soc_ll_reset_core(1);

	while (true) {
		;
	}
}
