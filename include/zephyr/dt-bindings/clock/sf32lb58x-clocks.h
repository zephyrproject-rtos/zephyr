/*
 * Copyright (c) 2026 Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
  * @file
  * @brief SF32LB58X Clock device tree bindings.
  */

#ifndef _INCLUDE_ZEPHYR_DT_BINDINGS_CLOCK_SF32LB58X_CLOCKS_H_
#define _INCLUDE_ZEPHYR_DT_BINDINGS_CLOCK_SF32LB58X_CLOCKS_H_

#include "sf32lb-clocks-common.h"

/**
 * @name Register offsets
 * @{
 */

/** Reset and Clock Control Enable Register 1 */
#define SF32LB58X_RCC_ENR1 0x08U
/** Reset and Clock Control Enable Register 2 */
#define SF32LB58X_RCC_ENR2 0x0CU

/** @} */

/**
 * @name Clock enable/disable definitions for peripherals
 * @{
 */

 /** DMA Controller 1 Clock */
#define SF32LB58X_CLOCK_DMAC1    SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 0U)

/** Mailbox 1 Clock */
#define SF32LB58X_CLOCK_MAILBOX1 SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 1U)

/** Pin Multiplexer 1 Clock */
#define SF32LB58X_CLOCK_PINMUX1  SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 2U)

/** Universal Synchronous Asynchronous Receiver Transmitter 1 Clock */
#define SF32LB58X_CLOCK_USART1   SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 3U)

/** Universal Synchronous Asynchronous Receiver Transmitter 2 Clock */
#define SF32LB58X_CLOCK_USART2   SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 4U)

/** EZIP Compression Engine 1 Clock */
#define SF32LB58X_CLOCK_EZIP1    SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 5U)

/** Embedded Processor Interrupt Controller Clock */
#define SF32LB58X_CLOCK_EPIC     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 6U)

/** LCD Controller 1 Clock */
#define SF32LB58X_CLOCK_LCDC1    SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 7U)

/** Inter-IC Sound 1 Clock */
#define SF32LB58X_CLOCK_I2S1     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 8U)

/** System Configuration 1 Clock */
#define SF32LB58X_CLOCK_SYSCFG1  SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 10U)

/** eFuse Controller Clock */
#define SF32LB58X_CLOCK_EFUSEC   SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 11U)

/** Advanced Encryption Standard Controller Clock */
#define SF32LB58X_CLOCK_AES      SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 12U)

/** Cyclic Redundancy Check 1 Clock */
#define SF32LB58X_CLOCK_CRC1     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 13U)

/** True Random Number Generator Clock */
#define SF32LB58X_CLOCK_TRNG     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 14U)

/** General Purpose Timer 1 Clock */
#define SF32LB58X_CLOCK_GPTIM1   SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 15U)

/** General Purpose Timer 2 Clock */
#define SF32LB58X_CLOCK_GPTIM2   SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 16U)

/** Basic Timer 1 Clock */
#define SF32LB58X_CLOCK_BTIM1    SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 17U)

/** Basic Timer 2 Clock */
#define SF32LB58X_CLOCK_BTIM2    SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 18U)

/** Serial Peripheral Interface 1 Clock */
#define SF32LB58X_CLOCK_SPI1     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 20U)

/** Serial Peripheral Interface 2 Clock */
#define SF32LB58X_CLOCK_SPI2     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 21U)

/** External Direct Memory Access Clock */
#define SF32LB58X_CLOCK_EXTDMA   SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 22U)

/** Direct Memory Access Controller 2 Clock */
#define SF32LB58X_CLOCK_DMAC2    SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 23U)

/** Neural Network Accelerator 1 Clock */
#define SF32LB58X_CLOCK_NNACC1   SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 24U)

/** Pulse Density Modulation 1 Clock */
#define SF32LB58X_CLOCK_PDM1     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 25U)

/** Pulse Density Modulation 2 Clock */
#define SF32LB58X_CLOCK_PDM2     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 26U)

/** Inter-Integrated Circuit 1 Clock */
#define SF32LB58X_CLOCK_I2C1     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 27U)

/** Inter-Integrated Circuit 2 Clock */
#define SF32LB58X_CLOCK_I2C2     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 28U)

/** Display Serial Interface Host Clock */
#define SF32LB58X_CLOCK_DSIHOST  SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 29U)

/** Display Serial Interface PHY Clock */
#define SF32LB58X_CLOCK_DSIPHY   SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 30U)

/** Parallel Trace Controller 1 Clock */
#define SF32LB58X_CLOCK_PTC1     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR1, 31U)

/** General Purpose Input Output 1 Clock */
#define SF32LB58X_CLOCK_GPIO1    SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 0U)

/** Multimedia Processing Interface 1 Clock */
#define SF32LB58X_CLOCK_MPI1     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 1U)

/** Multimedia Processing Interface 2 Clock */
#define SF32LB58X_CLOCK_MPI2     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 2U)

/** Multimedia Processing Interface 3 Clock */
#define SF32LB58X_CLOCK_MPI3     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 3U)

/** Secure Digital MultiMedia Card 1 Clock */
#define SF32LB58X_CLOCK_SDMMC1   SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 4U)

/** Secure Digital MultiMedia Card 2 Clock */
#define SF32LB58X_CLOCK_SDMMC2   SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 5U)

/** USB Controller Clock */
#define SF32LB58X_CLOCK_USBC     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 6U)

/** Inter-Integrated Circuit 3 Clock */
#define SF32LB58X_CLOCK_I2C3     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 8U)

/** Advanced Timer 1 Clock */
#define SF32LB58X_CLOCK_ATIM1    SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 9U)

/** Advanced Timer 2 Clock */
#define SF32LB58X_CLOCK_ATIM2    SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 10U)

/** Multimedia Processing Interface 4 Clock */
#define SF32LB58X_CLOCK_MPI4     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 11U)

/** Universal Synchronous Asynchronous Receiver Transmitter 3 Clock */
#define SF32LB58X_CLOCK_USART3   SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 12U)

/** EZIP Compression Engine 2 Clock */
#define SF32LB58X_CLOCK_EZIP2    SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 13U)

/** Fast Fourier Transform 1 Clock */
#define SF32LB58X_CLOCK_FFT1     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 14U)

/** Floating Point Accelerator 1 Clock */
#define SF32LB58X_CLOCK_FACC1    SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 15U)

/** Serial Communication Interface Clock */
#define SF32LB58X_CLOCK_SCI      SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 16U)

/** Controller Area Network 1 Clock */
#define SF32LB58X_CLOCK_CAN1     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 17U)

/** Controller Area Network 2 Clock */
#define SF32LB58X_CLOCK_CAN2     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 18U)

/** Audio Codec Clock */
#define SF32LB58X_CLOCK_AUDCODEC SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 19U)

/** Audio Processor Clock */
#define SF32LB58X_CLOCK_AUDPRC   SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 20U)

/** Graphics Processing Unit Clock */
#define SF32LB58X_CLOCK_GPU      SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 21U)

/** JPEG Encoder Clock */
#define SF32LB58X_CLOCK_JENC     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 22U)

/** JPEG Decoder Clock */
#define SF32LB58X_CLOCK_JDEC     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 23U)

/** Bus Monitor 2 Clock */
#define SF32LB58X_CLOCK_BUSMON2  SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 23U)

/** Inter-Integrated Circuit 4 Clock */
#define SF32LB58X_CLOCK_I2C4     SF32LB_CLOCK_CONFIG(SF32LB58X_RCC_ENR2, 25U)

/** @} */

#endif /* _INCLUDE_ZEPHYR_DT_BINDINGS_CLOCK_SF32LB58X_CLOCKS_H_ */
