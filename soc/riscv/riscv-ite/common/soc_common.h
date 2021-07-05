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
#include "encoding.h"

/* IRQ numbers */
#define RISCV_MACHINE_SOFT_IRQ       161  /* Machine Software Interrupt */
#define RISCV_MACHINE_TIMER_IRQ      157  /* Machine Timer Interrupt */
#define RISCV_MACHINE_EXT_IRQ        11 /* Machine External Interrupt */

#define RISCV_MAX_GENERIC_IRQ        191 /* Max Generic Interrupt */

/* Exception numbers */
#define RISCV_MACHINE_ECALL_EXP      11 /* Machine ECALL instruction */

/*
 * SOC-specific MSTATUS related info
 */
/* MSTATUS register to save/restore upon interrupt/exception/context switch */
#define SOC_MSTATUS_REG              mstatus

#define SOC_MSTATUS_IEN              (1 << 3) /* Machine Interrupt Enable bit */

/* Previous Privilege Mode - Machine Mode */
#define SOC_MSTATUS_MPP_M_MODE       (3 << 11)
/* Interrupt Enable Bit in Previous Privilege Mode */
#define SOC_MSTATUS_MPIE             (1 << 7)

/*
 * Default MSTATUS register value to restore from stack
 * upon scheduling a thread for the first time
 */
#define SOC_MSTATUS_DEF_RESTORE      (SOC_MSTATUS_MPP_M_MODE | SOC_MSTATUS_MPIE)


/* SOC-specific MCAUSE bitfields */
/* Interrupt Mask */
#define SOC_MCAUSE_IRQ_MASK          (1 << 31)
/* Exception code Mask */
#define SOC_MCAUSE_EXP_MASK          0x7FFFFFFF
/* ECALL exception number */
#define SOC_MCAUSE_ECALL_EXP         RISCV_MACHINE_ECALL_EXP

/* SOC-Specific EXIT ISR command */
#define SOC_ERET                     mret

#ifndef _ASMLANGUAGE

#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
void soc_interrupt_init(void);
#endif

#if defined(CONFIG_RISCV_HAS_PLIC)
void riscv_plic_irq_enable(u32_t irq);
void riscv_plic_irq_disable(u32_t irq);
int riscv_plic_irq_is_enabled(u32_t irq);
void riscv_plic_set_priority(u32_t irq, u32_t priority);
int riscv_plic_get_irq(void);
#endif

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
extern void ite_intc_irq_priority_set(unsigned int irq,
			unsigned int prio, unsigned int flags);
extern void ite_intc_isr_clear(unsigned int irq);
#endif /* CONFIG_ITE_IT8XXX2_INTC */

#ifdef CONFIG_SOC_IT8XXX2_PLL_FLASH_48M
void timer_5ms_one_shot(void);
#endif

#endif /* !_ASMLANGUAGE */

#endif /* __SOC_COMMON_H_ */
