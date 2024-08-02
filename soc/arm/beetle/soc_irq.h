/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Interrupt numbers for the ARM LTD Beetle SoC.
 */
#ifndef _ARM_BEETLE_SOC_IRQ_H_
#define _ARM_BEETLE_SOC_IRQ_H_

/* IRQ Numbers */
#define IRQ_SPARE               1  /* Undefined                             */
#define IRQ_I2C0                3  /* I2C 0 Interrupt                       */
#define IRQ_I2C1                4  /* I2C 1 Interrupt                       */
#define IRQ_RTC                 5  /* RTC Interrupt                         */
#define IRQ_SPI0                11 /* SPI 0 Interrupt                       */
#define IRQ_UARTOVF             12 /* Common UART Overflow Interrupt        */
#define IRQ_SPI1                13 /* SPI 1 Interrupt                       */
#define IRQ_QSPI                14 /* QUAD SPI Interrupt                    */
#define IRQ_DMA                 15 /* Reserved for DMA Interrupt            */
#define IRQ_PORT0_0             16 /* All P0 I/O pins used as irq source    */
#define IRQ_PORT0_1             17 /* There are 16 pins in total            */
#define IRQ_PORT0_2             18
#define IRQ_PORT0_3             19
#define IRQ_PORT0_4             20
#define IRQ_PORT0_5             21
#define IRQ_PORT0_6             22
#define IRQ_PORT0_7             23
#define IRQ_PORT0_8             24
#define IRQ_PORT0_9             25
#define IRQ_PORT0_10            26
#define IRQ_PORT0_11            27
#define IRQ_PORT0_12            28
#define IRQ_PORT0_13            29
#define IRQ_PORT0_14            30
#define IRQ_PORT0_15            31
#define IRQ_SYSERROR            32 /* System Error Interrupt                */
#define IRQ_EFLASH              33 /* Embedded Flash Interrupt              */
#define IRQ_LLCC_TXCMD_EMPTY    34 /* Cordio                                 */
#define IRQ_LLCC_TXEVT_EMPTY    35 /* Cordio                                 */
#define IRQ_LLCC_TXDMAH_DONE    36 /* Cordio                                 */
#define IRQ_LLCC_TXDMAL_DONE    37 /* Cordio                                 */
#define IRQ_LLCC_RXCMD_VALID    38 /* Cordio                                 */
#define IRQ_LLCC_RXEVT_VALID    39 /* Cordio                                 */
#define IRQ_LLCC_RXDMAH_DONE    40 /* Cordio                                 */
#define IRQ_LLCC_RXDMAL_DONE    41 /* Cordio                                 */
#define IRQ_TRNG                44 /* Random number generator Interrupt     */

#endif /* _ARM_BEETLE_SOC_IRQ_H_ */
