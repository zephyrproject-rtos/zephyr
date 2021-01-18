/*
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __RISCV64_ARIANE_SOC_H_
#define __RISCV64_ARIANE_SOC_H_

#include <soc_common.h>
#include <devicetree.h>

/* GPIO Interrupts */
#define ARIANE_GPIO_0_IRQ           (0)
#define ARIANE_GPIO_1_IRQ           (1)
#define ARIANE_GPIO_2_IRQ           (2)
#define ARIANE_GPIO_3_IRQ           (3)
#define ARIANE_GPIO_4_IRQ           (4)
#define ARIANE_GPIO_5_IRQ           (5)
#define ARIANE_GPIO_6_IRQ           (6)
#define ARIANE_GPIO_7_IRQ           (7)
#define ARIANE_GPIO_8_IRQ           (8)
#define ARIANE_GPIO_9_IRQ           (9)
#define ARIANE_GPIO_10_IRQ          (10)
#define ARIANE_GPIO_11_IRQ          (11)
#define ARIANE_GPIO_12_IRQ          (12)
#define ARIANE_GPIO_13_IRQ          (13)
#define ARIANE_GPIO_14_IRQ          (14)
#define ARIANE_GPIO_15_IRQ          (15)
#define ARIANE_GPIO_16_IRQ          (16)
#define ARIANE_GPIO_17_IRQ          (17)
#define ARIANE_GPIO_18_IRQ          (18)
#define ARIANE_GPIO_19_IRQ          (19)
#define ARIANE_GPIO_20_IRQ          (20)
#define ARIANE_GPIO_21_IRQ          (21)
#define ARIANE_GPIO_22_IRQ          (22)
#define ARIANE_GPIO_23_IRQ          (23)
#define ARIANE_GPIO_24_IRQ          (24)
#define ARIANE_GPIO_25_IRQ          (25)
#define ARIANE_GPIO_26_IRQ          (26)
#define ARIANE_GPIO_27_IRQ          (27)
#define ARIANE_GPIO_28_IRQ          (28)
#define ARIANE_GPIO_29_IRQ          (29)
#define ARIANE_GPIO_30_IRQ          (30)
#define ARIANE_GPIO_31_IRQ          (31)

/* UART Configuration */
#define ARIANE_UART_0_LINECFG           0x1

/* GPIO Configuration */
#define ARIANE_GPIO_0_BASE_ADDR         0x70002000

/* Clock controller. */
// TODO: Should this be 0x02000000?
#define PRCI_BASE_ADDR               0x44000000

/* Timer configuration */
/* Timer configuration */
#define RISCV_MTIME_BASE            0x200bff8
#define RISCV_MTIMECMP_BASE         0x2004000


/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE               CONFIG_SRAM_BASE_ADDRESS
#define RISCV_RAM_SIZE               KB(CONFIG_SRAM_SIZE)

#endif /* __RISCV64_ARIANE_SOC_H_ */
