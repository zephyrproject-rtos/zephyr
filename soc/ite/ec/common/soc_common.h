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

#ifndef _ASMLANGUAGE

struct ite_clk_cfg {
	uint8_t ctrl;
	uint8_t bits;
};

#ifdef CONFIG_HAS_ITE_INTC
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
#endif /* CONFIG_HAS_ITE_INTC */

#ifdef CONFIG_SOC_IT8XXX2_PLL_FLASH_48M
void timer_5ms_one_shot(void);
#endif

uint32_t chip_get_pll_freq(void);
void chip_pll_ctrl(enum chip_pll_mode mode);
void riscv_idle(enum chip_pll_mode mode, unsigned int key);

/* Functions for managing the CPU idle state */
void chip_permit_idle(void);
void chip_block_idle(void);
bool cpu_idle_not_allowed(void);

#endif /* !_ASMLANGUAGE */

#endif /* __SOC_COMMON_H_ */
