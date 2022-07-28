/*
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __RISCV32_MIV_SOC_H_
#define __RISCV32_MIV_SOC_H_

#include <soc_common.h>

/* UART Configuration */
#define MIV_UART_0_LINECFG           0x1

/* Timer configuration */
#define RISCV_MTIME_BASE             0x4400bff8
#define RISCV_MTIMECMP_BASE          0x44004000

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE               CONFIG_SRAM_BASE_ADDRESS
#define RISCV_RAM_SIZE               KB(CONFIG_SRAM_SIZE)

#endif /* __RISCV32_MIV_SOC_H_ */
