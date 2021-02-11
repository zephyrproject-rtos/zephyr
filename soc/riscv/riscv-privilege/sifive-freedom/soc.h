/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the SiFive Freedom processor
 */

#ifndef __RISCV_SIFIVE_FREEDOM_SOC_H_
#define __RISCV_SIFIVE_FREEDOM_SOC_H_

#include <soc_common.h>
#include <devicetree.h>

/* PINMUX IO Hardware Functions */
#define SIFIVE_PINMUX_IOF0            0x00
#define SIFIVE_PINMUX_IOF1            0x01

/* PINMUX MAX PINS */
#define SIFIVE_PINMUX_PINS            32

/* Clock controller. */
#define PRCI_BASE_ADDR               0x10008000

/* Timer configuration */
#define RISCV_MTIME_BASE             0x0200BFF8
#define RISCV_MTIMECMP_BASE          0x02004000

/* Always ON Domain */
#define SIFIVE_PMUIE		     0x10000140
#define SIFIVE_PMUCAUSE		     0x10000144
#define SIFIVE_PMUSLEEP		     0x10000148
#define SIFIVE_PMUKEY		     0x1000014C
#define SIFIVE_SLEEP_KEY_VAL	     0x0051F15E

#define SIFIVE_BACKUP_REG_BASE	     0x10000080

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE               CONFIG_SRAM_BASE_ADDRESS
#define RISCV_RAM_SIZE               KB(CONFIG_SRAM_SIZE)

#endif /* __RISCV_SIFIVE_FREEDOM_SOC_H_ */
