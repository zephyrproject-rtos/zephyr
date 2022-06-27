/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_H__
#define __SOC_H__

#ifndef _ASMLANGUAGE
#include <rom/ets_sys.h>
#include <rom/spi_flash.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <esp_clk.h>
#endif

#include <zephyr/arch/riscv/arch.h>

/* IRQ numbers */
#define RISCV_MACHINE_SOFT_IRQ 3 /* Machine Software Interrupt */
#define RISCV_MACHINE_TIMER_IRQ 7 /* Machine Timer Interrupt */
#define RISCV_MACHINE_EXT_IRQ 11 /* Machine External Interrupt */

/* ECALL Exception numbers */
#define SOC_MCAUSE_ECALL_EXP 11 /* Machine ECALL instruction */
#define SOC_MCAUSE_USER_ECALL_EXP 8 /* User ECALL instruction */

/* Interrupt Mask */
#define SOC_MCAUSE_IRQ_MASK (1 << 31)
/* Exception code Mask */
#define SOC_MCAUSE_EXP_MASK 0x7FFFFFFF
/* SOC-Specific EXIT ISR command */
#define SOC_ERET mret

#ifndef _ASMLANGUAGE

void __esp_platform_start(void);

extern void esp_rom_intr_matrix_set(int cpu_no, uint32_t model_num, uint32_t intr_num);
extern void esp_rom_uart_attach(void);
extern void esp_rom_uart_tx_wait_idle(uint8_t uart_no);
extern STATUS esp_rom_uart_tx_one_char(uint8_t chr);
extern STATUS esp_rom_uart_rx_one_char(uint8_t *chr);
extern int esp_rom_gpio_matrix_in(uint32_t gpio, uint32_t signal_index, bool inverted);
extern int esp_rom_gpio_matrix_out(uint32_t gpio, uint32_t signal_index,
				bool out_invrted, bool out_enabled_inverted);
extern void esp_rom_ets_set_user_start(uint32_t start);
extern void esprv_intc_int_set_threshold(int priority_threshold);
uint32_t soc_intr_get_next_source(void);
extern void esp_rom_Cache_Resume_ICache(uint32_t autoload);
extern int esp_rom_Cache_Invalidate_Addr(uint32_t addr, uint32_t size);
extern uint32_t esp_rom_Cache_Suspend_ICache(void);
extern void esp_rom_Cache_Invalidate_ICache_All(void);
extern int esp_rom_Cache_Dbus_MMU_Set(uint32_t ext_ram, uint32_t vaddr, uint32_t paddr,
				uint32_t psize, uint32_t num, uint32_t fixed);
extern int esp_rom_Cache_Ibus_MMU_Set(uint32_t ext_ram, uint32_t vaddr, uint32_t paddr,
				uint32_t psize, uint32_t num, uint32_t fixed);
extern void esp_rom_Cache_Resume_ICache(uint32_t autoload);
extern spiflash_legacy_data_t esp_rom_spiflash_legacy_data;
extern int esp_rom_gpio_matrix_in(uint32_t gpio, uint32_t signal_index,
				    bool inverted);
extern int esp_rom_gpio_matrix_out(uint32_t gpio, uint32_t signal_index,
				     bool out_inverted,
				     bool out_enabled_inverted);

#endif /* _ASMLANGUAGE */

#endif /* __SOC_H__ */
