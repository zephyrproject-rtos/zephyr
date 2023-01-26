/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <soc/rtc_cntl_reg.h>
#include <soc/timer_group_reg.h>
#include <soc/gpio_reg.h>
#include <soc/syscon_reg.h>
#include <soc/system_reg.h>
#include <soc/cache_memory.h>
#include "hal/soc_ll.h"
#include "esp_cpu.h"
#include "esp_timer.h"
#include "esp_spi_flash.h"
#include "esp_clk_internal.h"
#include <soc/interrupt_reg.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32c3.h>

#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <string.h>
#include <zephyr/toolchain/gcc.h>
#include <soc.h>

/*
 * This is written in C rather than assembly since, during the port bring up,
 * Zephyr is being booted by the Espressif bootloader.  With it, the C stack
 * is already set up.
 */
void __attribute__((section(".iram1"))) __esp_platform_start(void)
{
	volatile uint32_t *wdt_rtc_protect = (uint32_t *)RTC_CNTL_WDTWPROTECT_REG;
	volatile uint32_t *wdt_rtc_reg = (uint32_t *)RTC_CNTL_WDTCONFIG0_REG;

#ifdef CONFIG_RISCV_GP
	/* Configure the global pointer register
	 * (This should be the first thing startup does, as any other piece of code could be
	 * relaxed by the linker to access something relative to __global_pointer$)
	 */
	__asm__ __volatile__(".option push\n"
						".option norelax\n"
						"la gp, __global_pointer$\n"
						".option pop");
#endif /* CONFIG_RISCV_GP */

	__asm__ __volatile__("la t0, _esp32c3_vector_table\n"
						"csrw mtvec, t0\n");

	z_bss_zero();

	/* Disable normal interrupts. */
	csr_read_clear(mstatus, MSTATUS_MIE);

	/* ESP-IDF 2nd stage bootloader enables RTC WDT to check on startup sequence
	 * related issues in application. Hence disable that as we are about to start
	 * Zephyr environment.
	 */
	*wdt_rtc_protect = RTC_CNTL_WDT_WKEY_VALUE;
	*wdt_rtc_reg &= ~RTC_CNTL_WDT_EN;
	*wdt_rtc_protect = 0;

	/* Configure the Cache MMU size for instruction and rodata in flash. */
	extern uint32_t esp_rom_cache_set_idrom_mmu_size(uint32_t irom_size,
			uint32_t drom_size);

	extern int _rodata_reserved_start;
	uint32_t rodata_reserved_start_align =
		(uint32_t)&_rodata_reserved_start & ~(MMU_PAGE_SIZE - 1);
	uint32_t cache_mmu_irom_size =
		((rodata_reserved_start_align - SOC_DROM_LOW) / MMU_PAGE_SIZE) *
			sizeof(uint32_t);

	esp_rom_cache_set_idrom_mmu_size(cache_mmu_irom_size,
		CACHE_DROM_MMU_MAX_END - cache_mmu_irom_size);

	/* Enable wireless phy subsystem clock,
	 * This needs to be done before the kernel starts
	 */
	REG_CLR_BIT(SYSTEM_WIFI_CLK_EN_REG, SYSTEM_WIFI_CLK_SDIOSLAVE_EN);
	SET_PERI_REG_MASK(SYSTEM_WIFI_CLK_EN_REG, SYSTEM_WIFI_CLK_EN);

	/* Configures the CPU clock, RTC slow and fast clocks, and performs
	 * RTC slow clock calibration.
	 */
	esp_clk_init();

	esp_timer_early_init();

#if CONFIG_SOC_FLASH_ESP32
	spi_flash_guard_set(&g_flash_guard_default_ops);
#endif

	/*Initialize the esp32c3 interrupt controller */
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

void IRAM_ATTR esp_restart_noos(void)
{
	/* Disable interrupts */
	csr_read_clear(mstatus, MSTATUS_MIE);

	/* Flush any data left in UART FIFOs */
	esp_rom_uart_tx_wait_idle(0);
	esp_rom_uart_tx_wait_idle(1);

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
			SYSTEM_BTMAC_RST | SYSTEM_SDIO_RST | SYSTEM_EMAC_RST |
			SYSTEM_MACPWR_RST | SYSTEM_RW_BTMAC_RST | SYSTEM_RW_BTLP_RST |
			BLE_REG_REST_BIT | BLE_PWR_REG_REST_BIT | BLE_BB_REG_REST_BIT);

	REG_WRITE(SYSTEM_CORE_RST_EN_REG, 0);

	/* Reset uart0 core first, then reset apb side. */
	SET_PERI_REG_MASK(UART_CLK_CONF_REG(0), UART_RST_CORE_M);

	/* Reset timer/spi/uart */
	SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN0_REG,
				SYSTEM_TIMERS_RST | SYSTEM_SPI01_RST | SYSTEM_UART_RST);
	REG_WRITE(SYSTEM_PERIP_RST_EN0_REG, 0);
	/* Reset dma */
	SET_PERI_REG_MASK(SYSTEM_PERIP_RST_EN1_REG, SYSTEM_DMA_RST);
	REG_WRITE(SYSTEM_PERIP_RST_EN1_REG, 0);

	/* Reset core */
	soc_ll_reset_core(0);

	while (true) {
		;
	}
}

void sys_arch_reboot(int type)
{
	esp_restart_noos();
}
