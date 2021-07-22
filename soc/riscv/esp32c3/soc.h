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
#endif

#include <arch/riscv/arch.h>

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

extern void esp_rom_intr_matrix_set(int cpu_no, uint32_t model_num, uint32_t intr_num);
extern void esp_rom_uart_attach(void);
extern void esp_rom_uart_tx_wait_idle(uint8_t uart_no);
extern STATUS esp_rom_uart_tx_one_char(uint8_t chr);
extern STATUS esp_rom_uart_rx_one_char(uint8_t *chr);
extern void esp_rom_ets_set_user_start(uint32_t start);

ulong_t __soc_get_gp_initial_value(void);

#endif /* _ASMLANGUAGE */

#endif /* __SOC_H__ */
