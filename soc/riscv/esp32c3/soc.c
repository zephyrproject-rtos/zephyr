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
#include "esp_spi_flash.h"
#include <riscv/interrupt.h>

#include <kernel_structs.h>
#include <string.h>
#include <toolchain/gcc.h>
#include <soc.h>

#define ESP32C3_INTC_DEFAULT_PRIO 15

extern void _PrepC(void);
extern void esprv_intc_int_set_threshold(int priority_threshold);

/*
 * This is written in C rather than assembly since, during the port bring up,
 * Zephyr is being booted by the Espressif bootloader.  With it, the C stack
 * is already set up.
 */
void __attribute__((section(".iram1"))) __start(void)
{
	volatile uint32_t *wdt_rtc_protect = (uint32_t *)RTC_CNTL_WDTWPROTECT_REG;
	volatile uint32_t *wdt_rtc_reg = (uint32_t *)RTC_CNTL_WDTCONFIG0_REG;

	/* Configure the global pointer register
	 * (This should be the first thing startup does, as any other piece of code could be
	 * relaxed by the linker to access something relative to __global_pointer$)
	 */
	__asm__ __volatile__(".option push\n"
						".option norelax\n"
						"la gp, __global_pointer$\n"
						".option pop");

	__asm__ __volatile__("la t0, _esp32c3_vector_table\n"
						"csrw mtvec, t0\n");

	/* Disable normal interrupts. */
	csr_read_clear(mstatus, MSTATUS_MIE);

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

#if CONFIG_BOOTLOADER_ESP_IDF
	/* ESP-IDF 2nd stage bootloader enables RTC WDT to check on startup sequence
	 * related issues in application. Hence disable that as we are about to start
	 * Zephyr environment.
	 */
	*wdt_rtc_protect = RTC_CNTL_WDT_WKEY_VALUE;
	*wdt_rtc_reg &= ~RTC_CNTL_WDT_EN;
	*wdt_rtc_protect = 0;
#endif

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

	/* set global esp32c3's INTC masking level */
	esprv_intc_int_set_threshold(1);

	/* Start Zephyr */
	_PrepC();

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
	esp_rom_uart_tx_wait_idle(2);

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

void arch_irq_enable(unsigned int irq)
{
	uint32_t key = irq_lock();

	esprv_intc_int_set_priority(irq, ESP32C3_INTC_DEFAULT_PRIO);
	esprv_intc_int_set_type(irq, 0);
	esprv_intc_int_enable(1 << irq);

	irq_unlock(key);
}

void arch_irq_disable(unsigned int irq)
{
	esprv_intc_int_disable(1 << irq);
}

int arch_irq_is_enabled(unsigned int irq)
{
	return (esprv_intc_get_interrupt_unmask() & (1 << irq));
}

ulong_t __soc_get_gp_initial_value(void)
{
	extern uint32_t __global_pointer$;
	return (ulong_t)&__global_pointer$;
}
