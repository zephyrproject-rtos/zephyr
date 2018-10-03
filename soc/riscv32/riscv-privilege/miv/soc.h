#ifndef __RISCV32_MIV_SOC_H_
#define __RISCV32_MIV_SOC_H_

#include <soc_common.h>

/* GPIO Interrupts */
#define MIV_GPIO_0_IRQ           (RISCV_MAX_GENERIC_IRQ + 0)
#define MIV_GPIO_1_IRQ           (RISCV_MAX_GENERIC_IRQ + 1)
#define MIV_GPIO_2_IRQ           (RISCV_MAX_GENERIC_IRQ + 2)
#define MIV_GPIO_3_IRQ           (RISCV_MAX_GENERIC_IRQ + 3)
#define MIV_GPIO_4_IRQ           (RISCV_MAX_GENERIC_IRQ + 4)
#define MIV_GPIO_5_IRQ           (RISCV_MAX_GENERIC_IRQ + 5)
#define MIV_GPIO_6_IRQ           (RISCV_MAX_GENERIC_IRQ + 6)
#define MIV_GPIO_7_IRQ           (RISCV_MAX_GENERIC_IRQ + 7)
#define MIV_GPIO_8_IRQ           (RISCV_MAX_GENERIC_IRQ + 8)
#define MIV_GPIO_9_IRQ           (RISCV_MAX_GENERIC_IRQ + 9)
#define MIV_GPIO_10_IRQ          (RISCV_MAX_GENERIC_IRQ + 10)
#define MIV_GPIO_11_IRQ          (RISCV_MAX_GENERIC_IRQ + 11)
#define MIV_GPIO_12_IRQ          (RISCV_MAX_GENERIC_IRQ + 12)
#define MIV_GPIO_13_IRQ          (RISCV_MAX_GENERIC_IRQ + 13)
#define MIV_GPIO_14_IRQ          (RISCV_MAX_GENERIC_IRQ + 14)
#define MIV_GPIO_15_IRQ          (RISCV_MAX_GENERIC_IRQ + 15)
#define MIV_GPIO_16_IRQ          (RISCV_MAX_GENERIC_IRQ + 16)
#define MIV_GPIO_17_IRQ          (RISCV_MAX_GENERIC_IRQ + 17)
#define MIV_GPIO_18_IRQ          (RISCV_MAX_GENERIC_IRQ + 18)
#define MIV_GPIO_19_IRQ          (RISCV_MAX_GENERIC_IRQ + 19)
#define MIV_GPIO_20_IRQ          (RISCV_MAX_GENERIC_IRQ + 20)
#define MIV_GPIO_21_IRQ          (RISCV_MAX_GENERIC_IRQ + 21)
#define MIV_GPIO_22_IRQ          (RISCV_MAX_GENERIC_IRQ + 22)
#define MIV_GPIO_23_IRQ          (RISCV_MAX_GENERIC_IRQ + 23)
#define MIV_GPIO_24_IRQ          (RISCV_MAX_GENERIC_IRQ + 24)
#define MIV_GPIO_25_IRQ          (RISCV_MAX_GENERIC_IRQ + 25)
#define MIV_GPIO_26_IRQ          (RISCV_MAX_GENERIC_IRQ + 26)
#define MIV_GPIO_27_IRQ          (RISCV_MAX_GENERIC_IRQ + 27)
#define MIV_GPIO_28_IRQ          (RISCV_MAX_GENERIC_IRQ + 28)
#define MIV_GPIO_29_IRQ          (RISCV_MAX_GENERIC_IRQ + 29)
#define MIV_GPIO_30_IRQ          (RISCV_MAX_GENERIC_IRQ + 30)
#define MIV_GPIO_31_IRQ          (RISCV_MAX_GENERIC_IRQ + 31)

/* UART Configuration */
#define MIV_UART_0_BASE_ADDR         0x70001000
#define MIV_UART_0_LINECFG           0x1

/* GPIO Configuration */
#define MIV_GPIO_0_BASE_ADDR         0x70002000

/* Platform Level Interrupt Controller Configuration */
#define MIV_PLIC_BASE_ADDR           0x40000000
#define MIV_PLIC_PRIO_BASE_ADDR      MIV_PLIC_BASE_ADDR
#define MIV_PLIC_IRQ_EN_BASE_ADDR    (MIV_PLIC_BASE_ADDR + 0x2000)
#define MIV_PLIC_REG_BASE_ADDR       (MIV_PLIC_BASE_ADDR + 0x200000)

#define MIV_PLIC_MAX_PRIORITY        7

/* Clock controller. */
#define PRCI_BASE_ADDR               0x44000000

/* Timer configuration */
#define RISCV_MTIME_BASE             0x4400bff8
#define RISCV_MTIMECMP_BASE          0x44004000

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE               CONFIG_RISCV_RAM_BASE_ADDR
#define RISCV_RAM_SIZE               CONFIG_RISCV_RAM_SIZE

#endif /* __RISCV32_MIV_SOC_H_ */
