/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the riscv-qemu
 */

#ifndef __RISCV32_QEMU_SOC_H_
#define __RISCV32_QEMU_SOC_H_

/* CSR Registers */
#define RISCV_QEMU_MSTATUS               mstatus /* Machine Status Register */

/* IRQ numbers */
#define RISCV_MACHINE_TIMER_IRQ          7  /* Machine Timer Interrupt */

/* Exception numbers */
#define RISCV_QEMU_ECALL_EXP             11 /* ECALL Instruction */

/*
 * SOC-specific MSTATUS related info
 */
/* MSTATUS register to save/restore upon interrupt/exception/context switch */
#define SOC_MSTATUS_REG              RISCV_QEMU_MSTATUS

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
/* Exception code Mask */
#define SOC_MCAUSE_IRQ_MASK          0x7FFFFFFF
/* ECALL exception number */
#define SOC_MCAUSE_ECALL_EXP         RISCV_QEMU_ECALL_EXP

/* SOC-Specific EXIT ISR command */
#define SOC_ERET                     mret

/* UART configuration */
#define RISCV_QEMU_UART_BASE         0x40002000

/* Timer configuration */
#define RISCV_MTIME_BASE             0x40000000
#define RISCV_MTIMECMP_BASE          0x40000008

#ifndef _ASMLANGUAGE
#include <irq.h>
#include <misc/util.h>

#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
void soc_interrupt_init(void);
#endif

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE             CONFIG_RAM_BASE_ADDR
#define RISCV_RAM_SIZE             MB(CONFIG_RAM_SIZE_MB)

#endif /* !_ASMLANGUAGE */

#endif /* __RISCV32_QEMU_SOC_H_ */
