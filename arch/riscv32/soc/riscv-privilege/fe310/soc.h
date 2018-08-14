/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the SiFive Freedom E310 processor
 */

#ifndef __RISCV32_FE310_SOC_H_
#define __RISCV32_FE310_SOC_H_

#include <soc_common.h>

/* PINMUX Configuration */
#define FE310_PINMUX_0_BASE_ADDR     (CONFIG_FE310_GPIO_0_BASE_ADDR + 0x38)

/* PINMUX IO Hardware Functions */
#define FE310_PINMUX_IOF0            0x00
#define FE310_PINMUX_IOF1            0x01

/* PINMUX MAX PINS */
#define FE310_PINMUX_PINS            32

/* Platform Level Interrupt Controller Configuration */
#define PLIC_PRIO_BASE_ADDR    PLIC_BASE_ADDRESS
#define PLIC_IRQ_EN_BASE_ADDR  (PLIC_BASE_ADDRESS + 0x2000)
#define PLIC_REG_BASE_ADDR     (PLIC_BASE_ADDRESS + 0x200000)

#define PLIC_MAX_PRIORITY      PLIC_RISCV_MAX_PRIORITY

/* Clock controller. */
#define PRCI_BASE_ADDR               0x10008000

/* Timer configuration */
#define RISCV_MTIME_BASE             0x0200BFF8
#define RISCV_MTIMECMP_BASE          0x02004000

/* Always ON Domain */
#define FE310_PMUIE		     0x10000140
#define FE310_PMUCAUSE		     0x10000144
#define FE310_PMUSLEEP		     0x10000148
#define FE310_PMUKEY		     0x1000014C
#define FE310_SLEEP_KEY_VAL	     0x0051F15E

#define FE310_BACKUP_REG_BASE	     0x10000080

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE               CONFIG_RISCV_RAM_BASE_ADDR
#define RISCV_RAM_SIZE               CONFIG_RISCV_RAM_SIZE

#endif /* __RISCV32_FE310_SOC_H_ */
