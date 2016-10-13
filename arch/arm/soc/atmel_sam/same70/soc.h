/*
 * Copyright (c) 2016 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Register access macros for the Atmel SAM E70 MCU.
 *
 * This file provides register access macros for the Atmel SAM E70 MCU, HAL
 * drivers for core peripherals as well as symbols specific to Atmel SAM family.
 */

#ifndef _ATMEL_SAME70_SOC_H_
#define _ATMEL_SAME70_SOC_H_

#define DONT_USE_CMSIS_INIT
#define DONT_USE_PREDEFINED_CORE_HANDLERS
#define DONT_USE_PREDEFINED_PERIPHERALS_HANDLERS

#if defined CONFIG_SOC_PART_NUMBER_SAME70J19
  #include <same70j19.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70J20
  #include <same70j20.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70J21
  #include <same70j21.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70N19
  #include <same70n19.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70N20
  #include <same70n20.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70N21
  #include <same70n21.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70Q19
  #include <same70q19.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70Q20
  #include <same70q20.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70Q21
  #include <same70q21.h>
#else
  #error Library does not support the specified device.
#endif

#include "soc_pinmap.h"

#include "../common/soc_pmc.h"
#include "../common/soc_gpio.h"

/******  Cortex-M7 Processor Exceptions Numbers ******************************/

/**  2 Non Maskable Interrupt                */
#define NonMaskableInt_IRQn    -14
/**  3 HardFault Interrupt                   */
#define HardFault_IRQn         -13
/**  4 Cortex-M7 Memory Management Interrupt */
#define MemoryManagement_IRQn  -12
/**  5 Cortex-M7 Bus Fault Interrupt         */
#define BusFault_IRQn          -11
/**  6 Cortex-M7 Usage Fault Interrupt       */
#define UsageFault_IRQn        -10
/** 11 Cortex-M7 SV Call Interrupt           */
#define SVCall_IRQn             -5
/** 12 Cortex-M7 Debug Monitor Interrupt     */
#define DebugMonitor_IRQn       -4
/** 14 Cortex-M7 Pend SV Interrupt           */
#define PendSV_IRQn             -2
/** 15 Cortex-M7 System Tick Interrupt       */
#define SysTick_IRQn            -1

/******  SAME70 specific Interrupt Numbers ***********************************/

/**  0 SAME70Q21 Supply Controller (SUPC) */
#define SUPC_IRQn                0
/**  1 SAME70Q21 Reset Controller (RSTC) */
#define RSTC_IRQn                1
/**  2 SAME70Q21 Real Time Clock (RTC) */
#define RTC_IRQn                 2
/**  3 SAME70Q21 Real Time Timer (RTT) */
#define RTT_IRQn                 3
/**  4 SAME70Q21 Watchdog Timer (WDT) */
#define WDT_IRQn                 4
/**  5 SAME70Q21 Power Management Controller (PMC) */
#define PMC_IRQn                 5
/**  6 SAME70Q21 Enhanced Embedded Flash Controller (EFC) */
#define EFC_IRQn                 6
/**  7 SAME70Q21 UART 0 (UART0) */
#define UART0_IRQn               7
/**  8 SAME70Q21 UART 1 (UART1) */
#define UART1_IRQn               8
/** 10 SAME70Q21 Parallel I/O Controller A (PIOA) */
#define PIOA_IRQn               10
/** 11 SAME70Q21 Parallel I/O Controller B (PIOB) */
#define PIOB_IRQn               11
/** 12 SAME70Q21 Parallel I/O Controller C (PIOC) */
#define PIOC_IRQn               12
/** 13 SAME70Q21 USART 0 (USART0) */
#define USART0_IRQn             13
/** 14 SAME70Q21 USART 1 (USART1) */
#define USART1_IRQn             14
/** 15 SAME70Q21 USART 2 (USART2) */
#define USART2_IRQn             15
/** 16 SAME70Q21 Parallel I/O Controller D (PIOD) */
#define PIOD_IRQn               16
/** 17 SAME70Q21 Parallel I/O Controller E (PIOE) */
#define PIOE_IRQn               17
/** 18 SAME70Q21 Multimedia Card Interface (HSMCI) */
#define HSMCI_IRQn              18
/** 19 SAME70Q21 Two Wire Interface 0 HS (TWIHS0) */
#define TWIHS0_IRQn             19
/** 20 SAME70Q21 Two Wire Interface 1 HS (TWIHS1) */
#define TWIHS1_IRQn             20
/** 21 SAME70Q21 Serial Peripheral Interface 0 (SPI0) */
#define SPI0_IRQn               21
/** 22 SAME70Q21 Synchronous Serial Controller (SSC) */
#define SSC_IRQn                22
/** 23 SAME70Q21 Timer/Counter 0 (TC0) */
#define TC0_IRQn                23
/** 24 SAME70Q21 Timer/Counter 1 (TC1) */
#define TC1_IRQn                24
/** 25 SAME70Q21 Timer/Counter 2 (TC2) */
#define TC2_IRQn                25
/** 26 SAME70Q21 Timer/Counter 3 (TC3) */
#define TC3_IRQn                26
/** 27 SAME70Q21 Timer/Counter 4 (TC4) */
#define TC4_IRQn                27
/** 28 SAME70Q21 Timer/Counter 5 (TC5) */
#define TC5_IRQn                28
/** 29 SAME70Q21 Analog Front End 0 (AFEC0) */
#define AFEC0_IRQn              29
/** 30 SAME70Q21 Digital To Analog Converter (DACC) */
#define DACC_IRQn               30
/** 31 SAME70Q21 Pulse Width Modulation 0 (PWM0) */
#define PWM0_IRQn               31
/** 32 SAME70Q21 Integrity Check Monitor (ICM) */
#define ICM_IRQn                32
/** 33 SAME70Q21 Analog Comparator (ACC) */
#define ACC_IRQn                33
/** 34 SAME70Q21 USB Host / Device Controller (USBHS) */
#define USBHS_IRQn              34
/** 35 SAME70Q21 MCAN Controller 0 (MCAN0) */
#define MCAN0_IRQn              35
/** 36 SAME70Q21 MCAN Controller 0 LINE1 (MCAN0) */
#define MCAN0_LINE1_IRQn        36
/** 37 SAME70Q21 MCAN Controller 1 (MCAN1) */
#define MCAN1_IRQn              37
/** 38 SAME70Q21 MCAN Controller 1 LINE1 (MCAN1) */
#define MCAN1_LINE1_IRQn        38
/** 39 SAME70Q21 Ethernet MAC (GMAC) */
#define GMAC_IRQn               39
/** 40 SAME70Q21 Analog Front End 1 (AFEC1) */
#define AFEC1_IRQn              40
/** 41 SAME70Q21 Two Wire Interface 2 HS (TWIHS2) */
#define TWIHS2_IRQn             41
/** 42 SAME70Q21 Serial Peripheral Interface 1 (SPI1) */
#define SPI1_IRQn               42
/** 43 SAME70Q21 Quad I/O Serial Peripheral Interface (QSPI) */
#define QSPI_IRQn               43
/** 44 SAME70Q21 UART 2 (UART2) */
#define UART2_IRQn              44
/** 45 SAME70Q21 UART 3 (UART3) */
#define UART3_IRQn              45
/** 46 SAME70Q21 UART 4 (UART4) */
#define UART4_IRQn              46
/** 47 SAME70Q21 Timer/Counter 6 (TC6) */
#define TC6_IRQn                47
/** 48 SAME70Q21 Timer/Counter 7 (TC7) */
#define TC7_IRQn                48
/** 49 SAME70Q21 Timer/Counter 8 (TC8) */
#define TC8_IRQn                49
/** 50 SAME70Q21 Timer/Counter 9 (TC9) */
#define TC9_IRQn                50
/** 51 SAME70Q21 Timer/Counter 10 (TC10) */
#define TC10_IRQn               51
/** 52 SAME70Q21 Timer/Counter 11 (TC11) */
#define TC11_IRQn               52
/** 56 SAME70Q21 AES (AES) */
#define AES_IRQn                56
/** 57 SAME70Q21 True Random Generator (TRNG) */
#define TRNG_IRQn               57
/** 58 SAME70Q21 DMA (XDMAC) */
#define XDMAC_IRQn              58
/** 59 SAME70Q21 Camera Interface (ISI) */
#define ISI_IRQn                59
/** 60 SAME70Q21 Pulse Width Modulation 1 (PWM1) */
#define PWM1_IRQn               60
/** 62 SAME70Q21 SDRAM Controller (SDRAMC) */
#define SDRAMC_IRQn             62
/** 63 SAME70Q21 Reinforced Secure Watchdog Timer (RSWDT) */
#define RSWDT_IRQn              63

/** Number of peripheral IDs */
#define PERIPH_COUNT_IRQn       64

/** Processor Clock (HCLK) Frequency */
#define SOC_ATMEL_SAM_HCLK_FREQ_HZ CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
/** Master Clock (MCK) Frequency */
#define SOC_ATMEL_SAM_MCK_FREQ_HZ \
		(SOC_ATMEL_SAM_HCLK_FREQ_HZ / CONFIG_SOC_ATMEL_SAME70_MDIV)

#endif /* _ATMEL_SAME70_SOC_H_ */
