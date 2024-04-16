/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_H__
#define __SOC_H__
#include <soc/dport_reg.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/soc_caps.h>
#include <esp32s3/rom/ets_sys.h>
#include <esp32s3/rom/spi_flash.h>
#include "esp32s3/rom/cache.h"
#include <esp_rom_uart.h>
#include "soc/extmem_reg.h"
#include "soc/cache_memory.h"
#include "hal/cpu_hal.h"
#include "hal/cpu_types.h"
#include <esp_rom_sys.h>

#include <zephyr/types.h>
#include <stdbool.h>
#include <zephyr/arch/xtensa/arch.h>

#include <xtensa/core-macros.h>
#include <esp32s3/clk.h>

void __esp_platform_start(void);

static inline void esp32_set_mask32(uint32_t v, uint32_t mem_addr)
{
	sys_write32(sys_read32(mem_addr) | v, mem_addr);
}

static inline void esp32_clear_mask32(uint32_t v, uint32_t mem_addr)
{
	sys_write32(sys_read32(mem_addr) & ~v, mem_addr);
}

static inline uint32_t esp_core_id(void)
{
	uint32_t id;

	__asm__ volatile (
		"rsr.prid %0\n"
		"extui %0,%0,13,1" : "=r" (id));
	return id;
}

extern void esp_rom_intr_matrix_set(int cpu_no, uint32_t model_num, uint32_t intr_num);

extern int esp_rom_gpio_matrix_in(uint32_t gpio, uint32_t signal_index,
				    bool inverted);
extern int esp_rom_gpio_matrix_out(uint32_t gpio, uint32_t signal_index,
				     bool out_inverted,
				     bool out_enabled_inverted);

extern void esp_rom_uart_attach(void);
extern void esp_rom_uart_tx_wait_idle(uint8_t uart_no);
extern int esp_rom_uart_tx_one_char(uint8_t chr);
extern int esp_rom_uart_rx_one_char(uint8_t *chr);
extern void esp_rom_uart_set_clock_baudrate(uint8_t uart_no, uint32_t clock_hz, uint32_t baud_rate);

extern void esp_rom_ets_set_appcpu_boot_addr(void *addr);
void esp_appcpu_start(void *entry_point);

extern int esp_rom_Cache_Dbus_MMU_Set(uint32_t ext_ram, uint32_t vaddr, uint32_t paddr,
				uint32_t psize, uint32_t num, uint32_t fixed);
extern int esp_rom_Cache_Ibus_MMU_Set(uint32_t ext_ram, uint32_t vaddr, uint32_t paddr,
				uint32_t psize, uint32_t num, uint32_t fixed);

/* ROM functions which read/write internal i2c control bus for PLL, APLL */
extern uint8_t esp_rom_i2c_readReg(uint8_t block, uint8_t host_id, uint8_t reg_add);
extern void esp_rom_i2c_writeReg(uint8_t block, uint8_t host_id, uint8_t reg_add, uint8_t data);

/* cache initialization functions */
void esp_config_instruction_cache_mode(void);
void esp_config_data_cache_mode(void);

#endif /* __SOC_H__ */
