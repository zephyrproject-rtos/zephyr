/*
 * Copyright (c) 2019 Gerson Fernando Budke
 * Copyright (c) 2016 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Register access macros for the Atmel SAM V71 MCU.
 *
 * This file provides register access macros for the Atmel SAM V71 MCU, HAL
 * drivers for core peripherals as well as symbols specific to Atmel SAM family.
 */

#ifndef _ATMEL_SAMV71_SOC_H_
#define _ATMEL_SAMV71_SOC_H_

#include <sys/util.h>

#ifndef _ASMLANGUAGE

#define DONT_USE_CMSIS_INIT
#define DONT_USE_PREDEFINED_CORE_HANDLERS
#define DONT_USE_PREDEFINED_PERIPHERALS_HANDLERS

#if defined CONFIG_SOC_PART_NUMBER_SAMV71J19
#include <samv71j19.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV71J20
#include <samv71j20.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV71J21
#include <samv71j21.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV71N19
#include <samv71n19.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV71N20
#include <samv71n20.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV71N21
#include <samv71n21.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV71Q19
#include <samv71q19.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV71Q20
#include <samv71q20.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV71Q21
#include <samv71q21.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV71J19B
#include <samv71j19b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV71J20B
#include <samv71j20b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV71J21B
#include <samv71j21b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV71N19B
#include <samv71n19b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV71N20B
#include <samv71n20b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV71N21B
#include <samv71n21b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV71Q19B
#include <samv71q19b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV71Q20B
#include <samv71q20b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV71Q21B
#include <samv71q21b.h>
#else
  #error Library does not support the specified device.
#endif

#include "soc_pinmap.h"

#include "../common/soc_pmc.h"
#include "../common/soc_gpio.h"
#include "../common/atmel_sam_dt.h"

/* Add include for DTS generated information */
#include <devicetree.h>

#endif /* _ASMLANGUAGE */

/** Peripheral Hardware Request Line Identifier */
#define DMA_PERID_HSMCI_TX_RX 0
#define DMA_PERID_SPI0_TX     1
#define DMA_PERID_SPI0_RX     2
#define DMA_PERID_SPI1_TX     3
#define DMA_PERID_SPI1_RX     4
#define DMA_PERID_QSPI_TX     5
#define DMA_PERID_QSPI_RX     6
#define DMA_PERID_USART0_TX   7
#define DMA_PERID_USART0_RX   8
#define DMA_PERID_USART1_TX   9
#define DMA_PERID_USART1_RX   10
#define DMA_PERID_USART2_TX   11
#define DMA_PERID_USART2_RX   12
#define DMA_PERID_PWM0_TX     13
#define DMA_PERID_TWIHS0_TX   14
#define DMA_PERID_TWIHS0_RX   15
#define DMA_PERID_TWIHS1_TX   16
#define DMA_PERID_TWIHS1_RX   17
#define DMA_PERID_TWIHS2_TX   18
#define DMA_PERID_TWIHS2_RX   19
#define DMA_PERID_UART0_TX    20
#define DMA_PERID_UART0_RX    21
#define DMA_PERID_UART1_TX    22
#define DMA_PERID_UART1_RX    23
#define DMA_PERID_UART2_TX    24
#define DMA_PERID_UART2_RX    25
#define DMA_PERID_UART3_TX    26
#define DMA_PERID_UART3_RX    27
#define DMA_PERID_UART4_TX    28
#define DMA_PERID_UART4_RX    29
#define DMA_PERID_DACC0_TX    30
#define DMA_PERID_DACC1_TX    31
#define DMA_PERID_SSC_TX      32
#define DMA_PERID_SSC_RX      33
#define DMA_PERID_PIOA_RX     34
#define DMA_PERID_AFEC0_RX    35
#define DMA_PERID_AFEC1_RX    36
#define DMA_PERID_AES_TX      37
#define DMA_PERID_AES_RX      38
#define DMA_PERID_PWM1_TX     39
#define DMA_PERID_TC0_RX      40
#define DMA_PERID_TC3_RX      41
#define DMA_PERID_TC6_RX      42
#define DMA_PERID_TC9_RX      43
#define DMA_PERID_I2SC0_TX_L  44
#define DMA_PERID_I2SC0_RX_L  45
#define DMA_PERID_I2SC1_TX_L  46
#define DMA_PERID_I2SC1_RX_L  47
#define DMA_PERID_I2SC0_TX_R  48
#define DMA_PERID_I2SC0_RX_R  49
#define DMA_PERID_I2SC1_TX_R  50
#define DMA_PERID_I2SC1_RX_R  51

/** Processor Clock (HCLK) Frequency */
#define SOC_ATMEL_SAM_HCLK_FREQ_HZ DT_ARM_CORTEX_M7_0_CLOCK_FREQUENCY
/** Master Clock (MCK) Frequency */
#define SOC_ATMEL_SAM_MCK_FREQ_HZ \
		(SOC_ATMEL_SAM_HCLK_FREQ_HZ / CONFIG_SOC_ATMEL_SAMV71_MDIV)

#endif /* _ATMEL_SAMV71_SOC_H_ */
