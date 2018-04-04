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

/* PLatform Level Interrupt Controller (PLIC) interrupt sources */

/* Watchdog Compare Interrupt */
#define FE310_WDOGCMP_IRQ            (RISCV_MAX_GENERIC_IRQ + 1)

/* RTC Compare Interrupt */
#define FE310_RTCCMP_IRQ             (RISCV_MAX_GENERIC_IRQ + 2)

/* UART Interrupts */
#define FE310_UART_0_IRQ             (RISCV_MAX_GENERIC_IRQ + 3)
#define FE310_UART_1_IRQ             (RISCV_MAX_GENERIC_IRQ + 4)

/* QSPI Interrupts */
#define FE310_QSPI_0_IRQ             (RISCV_MAX_GENERIC_IRQ + 5)
#define FE310_QSPI_1_IRQ             (RISCV_MAX_GENERIC_IRQ + 6)
#define FE310_QSPI_2_IRQ             (RISCV_MAX_GENERIC_IRQ + 7)

/* GPIO Interrupts */
#define FE310_GPIO_0_IRQ             (RISCV_MAX_GENERIC_IRQ + 8)
#define FE310_GPIO_1_IRQ             (RISCV_MAX_GENERIC_IRQ + 9)
#define FE310_GPIO_2_IRQ             (RISCV_MAX_GENERIC_IRQ + 10)
#define FE310_GPIO_3_IRQ             (RISCV_MAX_GENERIC_IRQ + 11)
#define FE310_GPIO_4_IRQ             (RISCV_MAX_GENERIC_IRQ + 12)
#define FE310_GPIO_5_IRQ             (RISCV_MAX_GENERIC_IRQ + 13)
#define FE310_GPIO_6_IRQ             (RISCV_MAX_GENERIC_IRQ + 14)
#define FE310_GPIO_7_IRQ             (RISCV_MAX_GENERIC_IRQ + 15)
#define FE310_GPIO_8_IRQ             (RISCV_MAX_GENERIC_IRQ + 16)
#define FE310_GPIO_9_IRQ             (RISCV_MAX_GENERIC_IRQ + 17)
#define FE310_GPIO_10_IRQ            (RISCV_MAX_GENERIC_IRQ + 18)
#define FE310_GPIO_11_IRQ            (RISCV_MAX_GENERIC_IRQ + 19)
#define FE310_GPIO_12_IRQ            (RISCV_MAX_GENERIC_IRQ + 20)
#define FE310_GPIO_13_IRQ            (RISCV_MAX_GENERIC_IRQ + 21)
#define FE310_GPIO_14_IRQ            (RISCV_MAX_GENERIC_IRQ + 22)
#define FE310_GPIO_15_IRQ            (RISCV_MAX_GENERIC_IRQ + 23)
#define FE310_GPIO_16_IRQ            (RISCV_MAX_GENERIC_IRQ + 24)
#define FE310_GPIO_17_IRQ            (RISCV_MAX_GENERIC_IRQ + 25)
#define FE310_GPIO_18_IRQ            (RISCV_MAX_GENERIC_IRQ + 26)
#define FE310_GPIO_19_IRQ            (RISCV_MAX_GENERIC_IRQ + 27)
#define FE310_GPIO_20_IRQ            (RISCV_MAX_GENERIC_IRQ + 28)
#define FE310_GPIO_21_IRQ            (RISCV_MAX_GENERIC_IRQ + 29)
#define FE310_GPIO_22_IRQ            (RISCV_MAX_GENERIC_IRQ + 30)
#define FE310_GPIO_23_IRQ            (RISCV_MAX_GENERIC_IRQ + 31)
#define FE310_GPIO_24_IRQ            (RISCV_MAX_GENERIC_IRQ + 32)
#define FE310_GPIO_25_IRQ            (RISCV_MAX_GENERIC_IRQ + 33)
#define FE310_GPIO_26_IRQ            (RISCV_MAX_GENERIC_IRQ + 34)
#define FE310_GPIO_27_IRQ            (RISCV_MAX_GENERIC_IRQ + 35)
#define FE310_GPIO_28_IRQ            (RISCV_MAX_GENERIC_IRQ + 36)
#define FE310_GPIO_29_IRQ            (RISCV_MAX_GENERIC_IRQ + 37)
#define FE310_GPIO_30_IRQ            (RISCV_MAX_GENERIC_IRQ + 38)
#define FE310_GPIO_31_IRQ            (RISCV_MAX_GENERIC_IRQ + 39)

/* PWM Interrupts */
#define FE310_PWM_0_CMP_0_IRQ        (RISCV_MAX_GENERIC_IRQ + 40)
#define FE310_PWM_0_CMP_1_IRQ        (RISCV_MAX_GENERIC_IRQ + 41)
#define FE310_PWM_0_CMP_2_IRQ        (RISCV_MAX_GENERIC_IRQ + 42)
#define FE310_PWM_0_CMP_3_IRQ        (RISCV_MAX_GENERIC_IRQ + 43)

#define FE310_PWM_1_CMP_0_IRQ        (RISCV_MAX_GENERIC_IRQ + 44)
#define FE310_PWM_1_CMP_1_IRQ        (RISCV_MAX_GENERIC_IRQ + 45)
#define FE310_PWM_1_CMP_2_IRQ        (RISCV_MAX_GENERIC_IRQ + 46)
#define FE310_PWM_1_CMP_3_IRQ        (RISCV_MAX_GENERIC_IRQ + 47)

#define FE310_PWM_2_CMP_0_IRQ        (RISCV_MAX_GENERIC_IRQ + 48)
#define FE310_PWM_2_CMP_1_IRQ        (RISCV_MAX_GENERIC_IRQ + 49)
#define FE310_PWM_2_CMP_2_IRQ        (RISCV_MAX_GENERIC_IRQ + 50)
#define FE310_PWM_2_CMP_3_IRQ        (RISCV_MAX_GENERIC_IRQ + 51)

/* UART Configuration */
#define FE310_UART_0_BASE_ADDR       0x10013000
#define FE310_UART_1_BASE_ADDR       0x10023000

/* GPIO Configuration */
#define FE310_GPIO_0_BASE_ADDR       0x10012000

/* PINMUX Configuration */
#define FE310_PINMUX_0_BASE_ADDR     (FE310_GPIO_0_BASE_ADDR + 0x38)

/* PINMUX IO Hardware Functions */
#define FE310_PINMUX_IOF0            0x00
#define FE310_PINMUX_IOF1            0x01

/* PINMUX MAX PINS */
#define FE310_PINMUX_PINS            32

/* Platform Level Interrupt Controller Configuration */
#define FE310_PLIC_BASE_ADDR         0x0C000000
#define FE310_PLIC_PRIO_BASE_ADDR    FE310_PLIC_BASE_ADDR
#define FE310_PLIC_IRQ_EN_BASE_ADDR  (FE310_PLIC_BASE_ADDR + 0x2000)
#define FE310_PLIC_REG_BASE_ADDR     (FE310_PLIC_BASE_ADDR + 0x200000)

#define FE310_PLIC_MAX_PRIORITY      7

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
