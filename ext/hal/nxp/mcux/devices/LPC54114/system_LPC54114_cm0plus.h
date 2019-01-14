/*
** ###################################################################
**     Processors:          LPC54114J256BD64_cm0plus
**                          LPC54114J256UK49_cm0plus
**
**     Compilers:           Keil ARM C/C++ Compiler
**                          GNU C Compiler
**                          IAR ANSI C/C++ Compiler for ARM
**                          MCUXpresso Compiler
**
**     Reference manual:    LPC5411x User manual Rev. 1.1 25 May 2016
**     Version:             rev. 1.0, 2016-04-29
**     Build:               b180802
**
**     Abstract:
**         Provides a system configuration function and a global variable that
**         contains the system frequency. It configures the device and initializes
**         the oscillator (PLL) that is part of the microcontroller device.
**
**     Copyright 2016 Freescale Semiconductor, Inc.
**     Copyright 2016-2018 NXP
**
**     SPDX-License-Identifier: BSD-3-Clause
**
**     http:                 www.nxp.com
**     mail:                 support@nxp.com
**
**     Revisions:
**     - rev. 1.0 (2016-04-29)
**         Initial version.
**
** ###################################################################
*/

/*!
 * @file LPC54114_cm0plus
 * @version 1.0
 * @date 2016-04-29
 * @brief Device specific configuration file for LPC54114_cm0plus (header file)
 *
 * Provides a system configuration function and a global variable that contains
 * the system frequency. It configures the device and initializes the oscillator
 * (PLL) that is part of the microcontroller device.
 */

#ifndef _SYSTEM_LPC54114_cm0plus_H_
#define _SYSTEM_LPC54114_cm0plus_H_              /**< Symbol preventing repeated inclusion */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


#define DEFAULT_SYSTEM_CLOCK           12000000u           /* Default System clock value */
#define CLK_RTC_32K_CLK                   32768u           /* RTC oscillator 32 kHz output (32k_clk */
#define CLK_FRO_12MHZ                  12000000u           /* FRO 12 MHz (fro_12m) */
#define CLK_FRO_48MHZ                  48000000u           /* FRO 48 MHz (fro_48m) */
#define CLK_FRO_96MHZ                  96000000u           /* FRO 96 MHz (fro_96m) */
#define CLK_CLK_IN                            0u           /* Default CLK_IN pin clock */


/**
 * @brief System clock frequency (core clock)
 *
 * The system clock frequency supplied to the SysTick timer and the processor
 * core clock. This variable can be used by the user application to setup the
 * SysTick timer or configure other parameters. It may also be used by debugger to
 * query the frequency of the debug timer or configure the trace clock speed
 * SystemCoreClock is initialized with a correct predefined value.
 */
extern uint32_t SystemCoreClock;

/**
 * @brief Setup the microcontroller system.
 *
 * Typically this function configures the oscillator (PLL) that is part of the
 * microcontroller device. For systems with variable clock speed it also updates
 * the variable SystemCoreClock. SystemInit is called from startup_device file.
 */
void SystemInit (void);

/**
 * @brief Updates the SystemCoreClock variable.
 *
 * It must be called whenever the core clock is changed during program
 * execution. SystemCoreClockUpdate() evaluates the clock register settings and calculates
 * the current core clock.
 */
void SystemCoreClockUpdate (void);

/**
 * @brief SystemInit function hook.
 *
 * This weak function allows to call specific initialization code during the
 * SystemInit() execution.This can be used when an application specific code needs
 * to be called as close to the reset entry as possible (for example the Multicore
 * Manager MCMGR_EarlyInit() function call).
 * NOTE: No global r/w variables can be used in this hook function because the
 * initialization of these variables happens after this function.
 */
void SystemInitHook (void);

#ifdef __cplusplus
}
#endif

#endif  /* _SYSTEM_LPC54114_cm0plus_H_ */
