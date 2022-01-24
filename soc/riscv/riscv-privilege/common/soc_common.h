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

/* ECALL Exception numbers */
#define SOC_MCAUSE_ECALL_EXP         11 /* Machine ECALL instruction */
#define SOC_MCAUSE_USER_ECALL_EXP    8  /* User ECALL instruction */

/* SOC-specific MCAUSE bitfields */
#ifdef CONFIG_64BIT
/* Interrupt Mask */
#define SOC_MCAUSE_IRQ_MASK          (1 << 63)
#else
/* Interrupt Mask */
#define SOC_MCAUSE_IRQ_MASK          (1 << 31)
#endif

/* Exception code Mask */
#define SOC_MCAUSE_EXP_MASK          CONFIG_RISCV_SOC_MCAUSE_EXCEPTION_MASK

/* SOC-Specific EXIT ISR command */
#define SOC_ERET                     mret

#ifndef _ASMLANGUAGE

#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
void soc_interrupt_init(void);
#endif

#if defined(CONFIG_RISCV_HAS_PLIC)
void riscv_plic_irq_enable(uint32_t irq);
void riscv_plic_irq_disable(uint32_t irq);
int riscv_plic_irq_is_enabled(uint32_t irq);
void riscv_plic_set_priority(uint32_t irq, uint32_t priority);
int riscv_plic_get_irq(void);
#endif

#if defined(CONFIG_NUCLEI_ECLIC)
void nuclei_eclic_irq_enable(uint32_t irq);
void nuclei_eclic_irq_disable(uint32_t irq);
int nuclei_eclic_irq_is_enabled(uint32_t irq);
void nuclei_eclic_set_priority(uint32_t irq, uint32_t priority);
#endif

#endif /* !_ASMLANGUAGE */

#endif /* __SOC_COMMON_H_ */
