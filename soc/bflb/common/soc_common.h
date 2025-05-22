/*
 * Copyright (c) 2021-2025 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file interrupt management code for riscv SOCs supporting the SiFive clic
 */

#ifndef __SOC_COMMON_H_
#define __SOC_COMMON_H_

/* clang-format off */

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
/* Exception code Mask */
#define SOC_MCAUSE_EXP_MASK          0x7FFFFFFFFFFFFFFF
#else
/* Interrupt Mask */
#define SOC_MCAUSE_IRQ_MASK          (1 << 31)
/* Exception code Mask */
#define SOC_MCAUSE_EXP_MASK          0x7FFFFFFF
#endif

/* SOC-Specific EXIT ISR command */
#define SOC_ERET                     mret

/* CLINT Base Address */

#define CLIC_TIMER_ENABLE_ADDRESS    (0x02800407)

/* In mstatus register */

#define SOC_MIE_MSIE                 (0x1 << 3) /* Machine Software Interrupt Enable */

/* IRQ 0-15 : (exception:interrupt=0) */

#define SOC_IRQ_IAMISALIGNED	     (0)	/* Instruction Address Misaligned */
#define SOC_IRQ_IAFAULT		     (1)	/* Instruction Address Fault */
#define SOC_IRQ_IINSTRUCTION	     (2)	/* Illegal Instruction */
#define SOC_IRQ_BPOINT		     (3)	/* Break Point */
#define SOC_IRQ_LAMISALIGNED	     (4)	/* Load Address Misaligned */
#define SOC_IRQ_LAFAULT		     (5)	/* Load Access Fault */
#define SOC_IRQ_SAMISALIGNED	     (6)	/* Store/AMO Address Misaligned */
#define SOC_IRQ_SAFAULT		     (7)	/* Store/AMO Access Fault */
#define SOC_IRQ_ECALLU		     (8)	/* Environment Call from U-mode */
						/* 9-10: Reserved */
#define SOC_IRQ_ECALLM		     (11)	/* Environment Call from M-mode */
						/* 12-15: Reserved */
						/* IRQ 16- : (async event:interrupt=1) */
#define SOC_IRQ_NUM_BASE	     (16)
#define SOC_IRQ_ASYNC		     (16)

/* Machine Software Int */
#define SOC_IRQ_MSOFT		     (SOC_IRQ_ASYNC + RISCV_MACHINE_SOFT_IRQ)
/* Machine Timer Int */
#define SOC_IRQ_MTIMER		     (SOC_IRQ_ASYNC + RISCV_MACHINE_TIMER_IRQ)
/* Machine External Int */
#define SOC_IRQ_MEXT		     (SOC_IRQ_ASYNC + RISCV_MACHINE_EXT_IRQ)

/* Machine Global External Interrupt */
#define SOC_NR_MGEI_IRQS	     (64)

/* Total number of IRQs */
#define SOC_NR_IRQS		     (SOC_NR_MGEI_IRQS + SOC_IRQ_NUM_BASE)

/* clang-format on */

#endif /* __SOC_COMMON_H_ */
