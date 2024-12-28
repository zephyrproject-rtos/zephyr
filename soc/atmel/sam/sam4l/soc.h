/*
 * Copyright (c) 2020-2025 Gerson Fernando Budke <nandojve@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the Atmel SAM4L family processors.
 */

#ifndef _SOC_ATMEL_SAM_SAM4L_SOC_H_
#define _SOC_ATMEL_SAM_SAM4L_SOC_H_

#ifndef _ASMLANGUAGE

#define DONT_USE_CMSIS_INIT
#define DONT_USE_PREDEFINED_CORE_HANDLERS
#define DONT_USE_PREDEFINED_PERIPHERALS_HANDLERS

#if defined(CONFIG_SOC_SAM4LS8C)
#include <sam4ls8c.h>
#elif defined(CONFIG_SOC_SAM4LS8B)
#include <sam4ls8b.h>
#elif defined(CONFIG_SOC_SAM4LS8A)
#include <sam4ls8a.h>
#elif defined(CONFIG_SOC_SAM4LS4C)
#include <sam4ls4c.h>
#elif defined(CONFIG_SOC_SAM4LS4B)
#include <sam4ls4b.h>
#elif defined(CONFIG_SOC_SAM4LS4A)
#include <sam4ls4a.h>
#elif defined(CONFIG_SOC_SAM4LS2C)
#include <sam4ls2c.h>
#elif defined(CONFIG_SOC_SAM4LS2B)
#include <sam4ls2b.h>
#elif defined(CONFIG_SOC_SAM4LS2A)
#include <sam4ls2a.h>
#elif defined(CONFIG_SOC_SAM4LC8C)
#include <sam4lc8c.h>
#elif defined(CONFIG_SOC_SAM4LC8B)
#include <sam4lc8b.h>
#elif defined(CONFIG_SOC_SAM4LC8A)
#include <sam4lc8a.h>
#elif defined(CONFIG_SOC_SAM4LC4C)
#include <sam4lc4c.h>
#elif defined(CONFIG_SOC_SAM4LC4B)
#include <sam4lc4b.h>
#elif defined(CONFIG_SOC_SAM4LC4A)
#include <sam4lc4a.h>
#elif defined(CONFIG_SOC_SAM4LC2C)
#include <sam4lc2c.h>
#elif defined(CONFIG_SOC_SAM4LC2B)
#include <sam4lc2b.h>
#elif defined(CONFIG_SOC_SAM4LC2A)
#include <sam4lc2a.h>
#else
#error Library does not support the specified device.
#endif

#include "../common/soc_pmc.h"
#include "../common/soc_gpio.h"
#include "../common/atmel_sam_dt.h"

/** Processor Clock (HCLK) Frequency */
#define SOC_ATMEL_SAM_HCLK_FREQ_HZ      ATMEL_SAM_DT_CPU_CLK_FREQ_HZ

/** Master Clock (MCK) Frequency */
#define SOC_ATMEL_SAM_MCK_FREQ_HZ       SOC_ATMEL_SAM_HCLK_FREQ_HZ

#define SOC_ATMEL_SAM_RCSYS_NOMINAL_HZ	115000
#define SOC_ATMEL_SAM_RC32K_NOMINAL_HZ	32768

/** Oscillator identifiers
 *    External Oscillator 0
 *    External 32 kHz oscillator
 *    Internal 32 kHz RC oscillator
 *    Internal 80 MHz RC oscillator
 *    Internal 4-8-12 MHz RCFAST oscillator
 *    Internal 1 MHz RC oscillator
 *    Internal System RC oscillator
 */
#define OSC_ID_OSC0             0
#define OSC_ID_OSC32            1
#define OSC_ID_RC32K            2
#define OSC_ID_RC80M            3
#define OSC_ID_RCFAST           4
#define OSC_ID_RC1M             5
#define OSC_ID_RCSYS            6

/** System clock source
 *    System RC oscillator
 *    Oscillator 0
 *    Phase Locked Loop 0
 *    Digital Frequency Locked Loop
 *    80 MHz RC oscillator
 *    4-8-12 MHz RC oscillator
 *    1 MHz RC oscillator
 */
#define OSC_SRC_RCSYS           0
#define OSC_SRC_OSC0            1
#define OSC_SRC_PLL0            2
#define OSC_SRC_DFLL            3
#define OSC_SRC_RC80M           4
#define OSC_SRC_RCFAST          5
#define OSC_SRC_RC1M            6

#define PM_CLOCK_MASK(bus, per) ((bus << 5) + per)

/** Bus index of maskable module clocks. Peripheral ids are defined out of
 * order.  It start from PBA up to PBD, then move to HSB, and finally CPU.
 */
#define PM_CLK_GRP_CPU          5
#define PM_CLK_GRP_HSB          4
#define PM_CLK_GRP_PBA          0
#define PM_CLK_GRP_PBB          1
#define PM_CLK_GRP_PBC          2
#define PM_CLK_GRP_PBD          3

/** Clocks derived from the CPU clock
 */
#define SYSCLK_OCD              0

/** Clocks derived from the HSB clock
 */
#define SYSCLK_PDCA_HSB         0
#define SYSCLK_HFLASHC_DATA     1
#define SYSCLK_HRAMC1_DATA      2
#define SYSCLK_USBC_DATA        3
#define SYSCLK_CRCCU_DATA       4
#define SYSCLK_PBA_BRIDGE       5
#define SYSCLK_PBB_BRIDGE       6
#define SYSCLK_PBC_BRIDGE       7
#define SYSCLK_PBD_BRIDGE       8
#define SYSCLK_AESA_HSB         9

/** Clocks derived from the PBA clock
 */
#define SYSCLK_IISC             0
#define SYSCLK_SPI              1
#define SYSCLK_TC0              2
#define SYSCLK_TC1              3
#define SYSCLK_TWIM0            4
#define SYSCLK_TWIS0            5
#define SYSCLK_TWIM1            6
#define SYSCLK_TWIS1            7
#define SYSCLK_USART0           8
#define SYSCLK_USART1           9
#define SYSCLK_USART2           10
#define SYSCLK_USART3           11
#define SYSCLK_ADCIFE           12
#define SYSCLK_DACC             13
#define SYSCLK_ACIFC            14
#define SYSCLK_GLOC             15
#define SYSCLK_ABDACB           16
#define SYSCLK_TRNG             17
#define SYSCLK_PARC             18
#define SYSCLK_CATB             19
#define SYSCLK_TWIM2            21
#define SYSCLK_TWIM3            22
#define SYSCLK_LCDCA            23

/** Clocks derived from the PBB clock
 */
#define SYSCLK_HFLASHC_REGS     0
#define SYSCLK_HRAMC1_REGS      1
#define SYSCLK_HMATRIX          2
#define SYSCLK_PDCA_PB          3
#define SYSCLK_CRCCU_REGS       4
#define SYSCLK_USBC_REGS        5
#define SYSCLK_PEVC             6

/** Clocks derived from the PBC clock
 */
#define SYSCLK_PM               0
#define SYSCLK_CHIPID           1
#define SYSCLK_SCIF             2
#define SYSCLK_FREQM            3
#define SYSCLK_GPIO             4

/** Clocks derived from the PBD clock
 */
#define SYSCLK_BPM              0
#define SYSCLK_BSCIF            1
#define SYSCLK_AST              2
#define SYSCLK_WDT              3
#define SYSCLK_EIC              4
#define SYSCLK_PICOUART         5

/** Divided clock mask derived from the PBA clock
 */
#define PBA_DIVMASK_TIMER_CLOCK2     (1u << 0)
#define PBA_DIVMASK_TIMER_CLOCK3     (1u << 2)
#define PBA_DIVMASK_CLK_USART        (1u << 2)
#define PBA_DIVMASK_TIMER_CLOCK4     (1u << 4)
#define PBA_DIVMASK_TIMER_CLOCK5     (1u << 6)
#define PBA_DIVMASK_Msk              (0x7Fu << 0)

/** Generic Clock Instances
 *    0- DFLLIF main reference and GCLK0 pin (CLK_DFLLIF_REF)
 *    1- DFLLIF dithering and SSG reference and GCLK1 pin (CLK_DFLLIF_DITHER)
 *    2- AST and GCLK2 pin
 *    3- CATB and GCLK3 pin
 *    4- AESA
 *    5- GLOC, TC0 and RC32KIFB_REF
 *    6- ABDACB and IISC
 *    7- USBC
 *    8- TC1 and PEVC[0]
 *    9- PLL0 and PEVC[1]
 *   10- ADCIFE
 *   11- Master generic clock. Can be used as source for other generic clocks.
 */
#define GEN_CLK_DFLL_REF	0
#define GEN_CLK_DFLL_DITHER	1
#define GEN_CLK_AST		2
#define GEN_CLK_CATB		3
#define GEN_CLK_AESA		4
#define GEN_CLK_GLOC		5
#define GEN_CLK_ABDACB		6
#define GEN_CLK_USBC		7
#define GEN_CLK_TC1_PEVC0	8
#define GEN_CLK_PLL0_PEVC1	9
#define GEN_CLK_ADCIFE		10
#define GEN_CLK_MASTER_GEN	11

#endif /* !_ASMLANGUAGE */

#endif /* _SOC_ATMEL_SAM_SAM4L_SOC_H_ */
