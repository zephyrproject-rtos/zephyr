/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file configuration macros for riscv SOCs supporting the riscv
 *       privileged architecture specification
 */

#ifndef __SOC_COMMON_H_
#define __SOC_COMMON_H_

#include "chip_chipregs.h"

/* SOC-specific MCAUSE bitfields */

/* Interrupt Mask. 1 (interrupt) or 0 (exception) */
#define SOC_MCAUSE_IRQ_MASK          BIT(31)

/* Exception code Mask */
#define SOC_MCAUSE_EXP_MASK          0x7FFFFFFF

/* Exception code of environment call from M-mode */
#define SOC_MCAUSE_ECALL_EXP         11

/* SOC-Specific EXIT ISR command */
#define SOC_ERET                     mret

#ifndef _ASMLANGUAGE

#if CONFIG_ITE_IT8XXX2_INTC
/*
 * Save current interrupt state of soc-level into ier_setting[] with
 * disabling interrupt.
 */
void ite_intc_save_and_disable_interrupts(void);
/* Restore interrupt state of soc-level from ier_setting[], use with care. */
void ite_intc_restore_interrupts(void);

extern void ite_intc_irq_enable(unsigned int irq);
extern void ite_intc_irq_disable(unsigned int irq);
extern uint8_t ite_intc_get_irq_num(void);
extern int ite_intc_irq_is_enable(unsigned int irq);
extern void ite_intc_irq_polarity_set(unsigned int irq, unsigned int flags);
extern void ite_intc_isr_clear(unsigned int irq);
void ite_intc_init(void);
bool ite_intc_no_irq(void);
#endif /* CONFIG_ITE_IT8XXX2_INTC */

#ifdef CONFIG_SOC_IT8XXX2_PLL_FLASH_48M
void timer_5ms_one_shot(void);
#endif

uint32_t chip_get_pll_freq(void);
void chip_pll_ctrl(enum chip_pll_mode mode);
void riscv_idle(enum chip_pll_mode mode, unsigned int key);

#ifdef CONFIG_SOC_IT8XXX2_CPU_IDLE_GATING
void chip_permit_idle(void);
void chip_block_idle(void);
bool cpu_idle_not_allowed(void);
#endif

#endif /* !_ASMLANGUAGE */

#endif /* __SOC_COMMON_H_ */
