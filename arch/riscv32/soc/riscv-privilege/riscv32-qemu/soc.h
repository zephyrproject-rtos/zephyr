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

#include <soc_common.h>

#include <misc/util.h>

/* UART configuration */
#define RISCV_QEMU_UART_BASE         0x40002000

/* Timer configuration */
#define RISCV_MTIME_BASE             0x40000000
#define RISCV_MTIMECMP_BASE          0x40000008

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE               CONFIG_RISCV_RAM_BASE_ADDR
#define RISCV_RAM_SIZE               MB(CONFIG_RISCV_RAM_SIZE_MB)

#endif /* __RISCV32_QEMU_SOC_H_ */
