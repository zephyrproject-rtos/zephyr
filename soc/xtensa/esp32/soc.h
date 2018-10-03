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

/**
 * @brief Struct to peripheral masks to enable peripheral
 * clock and reset peripherals.
 */
struct esp32_peripheral {
	/** Mask for clock peripheral */
	int clk;
	/** Mask for reset peripheral */
	int rst;
};

static inline void esp32_set_mask32(u32_t v, u32_t mem_addr)
{
	sys_write32(sys_read32(mem_addr) | v, mem_addr);
}

static inline void esp32_clear_mask32(u32_t v, u32_t mem_addr)
{
	sys_write32(sys_read32(mem_addr) & ~v, mem_addr);
}

static inline void esp32_enable_peripheral(const struct esp32_peripheral *peripheral)
{
	esp32_set_mask32(peripheral->clk, DPORT_PERIP_CLK_EN_REG);
	esp32_clear_mask32(peripheral->rst, DPORT_PERIP_RST_EN_REG);
}

extern int esp32_rom_intr_matrix_set(int cpu_no,
				     int interrupt_src,
				     int interrupt_line);

extern int esp32_rom_gpio_matrix_in(u32_t gpio, u32_t signal_index,
				    bool inverted);
extern int esp32_rom_gpio_matrix_out(u32_t gpio, u32_t signal_index,
				     bool out_inverted,
				     bool out_enabled_inverted);

extern void esp32_rom_uart_attach(void);
extern STATUS esp32_rom_uart_tx_one_char(u8_t chr);
extern STATUS esp32_rom_uart_rx_one_char(u8_t *chr);

extern void esp32_rom_Cache_Flush(int cpu);
extern void esp32_rom_Cache_Read_Enable(int cpu);
extern void esp32_rom_ets_set_appcpu_boot_addr(void *addr);

#endif /* __SOC_H__ */
