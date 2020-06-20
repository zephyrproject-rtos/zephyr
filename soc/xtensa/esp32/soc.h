/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_H__
#define __SOC_H__
#include <soc/dport_reg.h>
#include <soc/rtc_cntl_reg.h>
#include <rom/ets_sys.h>

#include <zephyr/types.h>
#include <stdbool.h>
#include <arch/xtensa/arch.h>

static inline void esp32_set_mask32(uint32_t v, uint32_t mem_addr)
{
	sys_write32(sys_read32(mem_addr) | v, mem_addr);
}

static inline void esp32_clear_mask32(uint32_t v, uint32_t mem_addr)
{
	sys_write32(sys_read32(mem_addr) & ~v, mem_addr);
}

extern int esp32_rom_intr_matrix_set(int cpu_no,
				     int interrupt_src,
				     int interrupt_line);

extern int esp32_rom_gpio_matrix_in(uint32_t gpio, uint32_t signal_index,
				    bool inverted);
extern int esp32_rom_gpio_matrix_out(uint32_t gpio, uint32_t signal_index,
				     bool out_inverted,
				     bool out_enabled_inverted);

extern void esp32_rom_uart_attach(void);
extern void esp32_rom_uart_tx_wait_idle(uint8_t uart_no);
extern STATUS esp32_rom_uart_tx_one_char(uint8_t chr);
extern STATUS esp32_rom_uart_rx_one_char(uint8_t *chr);

extern void esp32_rom_Cache_Flush(int cpu);
extern void esp32_rom_Cache_Read_Enable(int cpu);
extern void esp32_rom_ets_set_appcpu_boot_addr(void *addr);

/* ROM functions which read/write internal i2c control bus for PLL, APLL */
extern uint8_t esp32_rom_i2c_readReg(uint8_t block, uint8_t host_id, uint8_t reg_add);
extern void esp32_rom_i2c_writeReg(uint8_t block, uint8_t host_id, uint8_t reg_add, uint8_t data);

#endif /* __SOC_H__ */
