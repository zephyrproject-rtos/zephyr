/*
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the Gotham SoC
 */

#ifndef __RISCV32_GOTHAM_SOC_H_
#define __RISCV32_GOTHAM_SOC_H_

#include <soc_common.h>

#define GOTHAM_FCLK_RATE	50000000

/* Platform Level Interrupt Controller (PLIC) interrupt sources */

/* PLIC Interrupt Sources */
#define GOTHAM_UART_IRQ           (RISCV_MAX_GENERIC_IRQ + 1)
#define GOTHAM_GPIO_IRQ           (RISCV_MAX_GENERIC_IRQ + 2)
#define GOTHAM_SPI_IRQ            (RISCV_MAX_GENERIC_IRQ + 3)
#define GOTHAM_I2C_IRQ            (RISCV_MAX_GENERIC_IRQ + 4)

/* UART Configuration */
#define GOTHAM_UART_BASE_ADDR       0x00030000
#define GOTHAM_UART_BAUDRATE        115200

/* GPIO Configuration */
#define GOTHAM_GPIO_0_BASE_ADDR       0x00000000

/* Platform Level Interrupt Controller Configuration */
#define GOTHAM_PLIC_BASE_ADDR		0x00060000
#define GOTHAM_PLIC_REG_PRI	(GOTHAM_PLIC_BASE_ADDR + 0x0C)
#define GOTHAM_PLIC_REG_IRQ_EN	(GOTHAM_PLIC_BASE_ADDR + 0x10)
#define GOTHAM_PLIC_REG_THRES	(GOTHAM_PLIC_BASE_ADDR + 0x14)
#define GOTHAM_PLIC_REG_ID	(GOTHAM_PLIC_BASE_ADDR + 0x18)

#define GOTHAM_PLIC_MAX_PRIORITY	7

/* Timer configuration */
#define RISCV_MTIME_BASE             0x0005000C
#define RISCV_MTIMECMP_BASE          0x00050004

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE               CONFIG_RISCV_RAM_BASE_ADDR
#define RISCV_RAM_SIZE               CONFIG_RISCV_RAM_SIZE

#endif /* __RISCV32_GOTHAM_SOC_H_ */
