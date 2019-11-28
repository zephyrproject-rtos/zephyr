/*
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __RISCV32_MIV_SOC_H_
#define __RISCV32_MIV_SOC_H_

#include <soc_common.h>
#include <generated_dts_board.h>

/* GPIO Interrupts */
#define MIV_GPIO_0_IRQ           (0)
#define MIV_GPIO_1_IRQ           (1)
#define MIV_GPIO_2_IRQ           (2)
#define MIV_GPIO_3_IRQ           (3)
#define MIV_GPIO_4_IRQ           (4)
#define MIV_GPIO_5_IRQ           (5)
#define MIV_GPIO_6_IRQ           (6)
#define MIV_GPIO_7_IRQ           (7)
#define MIV_GPIO_8_IRQ           (8)
#define MIV_GPIO_9_IRQ           (9)
#define MIV_GPIO_10_IRQ          (10)
#define MIV_GPIO_11_IRQ          (11)
#define MIV_GPIO_12_IRQ          (12)
#define MIV_GPIO_13_IRQ          (13)
#define MIV_GPIO_14_IRQ          (14)
#define MIV_GPIO_15_IRQ          (15)
#define MIV_GPIO_16_IRQ          (16)
#define MIV_GPIO_17_IRQ          (17)
#define MIV_GPIO_18_IRQ          (18)
#define MIV_GPIO_19_IRQ          (19)
#define MIV_GPIO_20_IRQ          (20)
#define MIV_GPIO_21_IRQ          (21)
#define MIV_GPIO_22_IRQ          (22)
#define MIV_GPIO_23_IRQ          (23)
#define MIV_GPIO_24_IRQ          (24)
#define MIV_GPIO_25_IRQ          (25)
#define MIV_GPIO_26_IRQ          (26)
#define MIV_GPIO_27_IRQ          (27)
#define MIV_GPIO_28_IRQ          (28)
#define MIV_GPIO_29_IRQ          (29)
#define MIV_GPIO_30_IRQ          (30)
#define MIV_GPIO_31_IRQ          (31)

/* UART Configuration */
#define MIV_UART_0_LINECFG           0x1

/* GPIO Configuration */
#define MIV_GPIO_0_BASE_ADDR         0x70002000

/* Clock controller. */
#define PRCI_BASE_ADDR               0x44000000

/* Timer configuration */
#define RISCV_MTIME_BASE             0x4400bff8
#define RISCV_MTIMECMP_BASE          0x44004000

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE               DT_SRAM_BASE_ADDRESS
#define RISCV_RAM_SIZE               KB(DT_SRAM_SIZE)

#endif /* __RISCV32_MIV_SOC_H_ */
