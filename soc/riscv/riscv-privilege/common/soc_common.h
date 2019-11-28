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

/* IRQ numbers */
#define RISCV_MACHINE_SOFT_IRQ       3  /* Machine Software Interrupt */
#define RISCV_MACHINE_TIMER_IRQ      7  /* Machine Timer Interrupt */
#define RISCV_MACHINE_EXT_IRQ        11 /* Machine External Interrupt */

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
#ifdef CONFIG_64BIT
/* Interrupt Mask */
#define SOC_MCAUSE_IRQ_MASK          (1 << 63)
/* Exception code Mask */
#define SOC_MCAUSE_EXP_MASK          0x7FFFFFFFFFFFFFFF
#else
/* Interrupt Mask */
#define SOC_MCAUSE_IRQ_MASK          (1 << 31)
/* Exception code Mask */
#define SOC_MCAUSE_EXP_MASK          0x7FFFFFFF
#endif
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

#endif /* !_ASMLANGUAGE */

#endif /* __SOC_COMMON_H_ */
