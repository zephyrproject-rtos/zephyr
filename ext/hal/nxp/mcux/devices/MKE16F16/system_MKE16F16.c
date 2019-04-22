/*
** ###################################################################
**     Processors:          MKE16F256VLH16
**                          MKE16F256VLL16
**                          MKE16F512VLH16
**                          MKE16F512VLL16
**
**     Compilers:           Keil ARM C/C++ Compiler
**                          Freescale C/C++ for Embedded ARM
**                          GNU C Compiler
**                          IAR ANSI C/C++ Compiler for ARM
**                          MCUXpresso Compiler
**
**     Reference manual:    KE1xFP100M168SF0RM, Rev. 2, Aug. 2016
**     Version:             rev. 4.0, 2016-09-20
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
**     - rev. 1.0 (2015-11-18)
**         Initial version.
**     - rev. 2.0 (2015-12-03)
**         Alpha version based on rev0 RDP.
**     - rev. 3.0 (2016-04-13)
**         Final version based on rev1 RDP.
**     - rev. 4.0 (2016-09-20)
**         Updated based on rev2 RDP.
**
** ###################################################################
*/

/*!
 * @file MKE16F16
 * @version 4.0
 * @date 2016-09-20
 * @brief Device specific configuration file for MKE16F16 (implementation file)
 *
 * Provides a system configuration function and a global variable that contains
 * the system frequency. It configures the device and initializes the oscillator
 * (PLL) that is part of the microcontroller device.
 */

#include <stdint.h>
#include "fsl_device_registers.h"



/* ----------------------------------------------------------------------------
   -- Core clock
   ---------------------------------------------------------------------------- */

uint32_t SystemCoreClock = DEFAULT_SYSTEM_CLOCK;

/* ----------------------------------------------------------------------------
   -- SystemInit()
   ---------------------------------------------------------------------------- */

void SystemInit (void) {
#if ((__FPU_PRESENT == 1) && (__FPU_USED == 1))
  SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));    /* set CP10, CP11 Full Access */
#endif /* ((__FPU_PRESENT == 1) && (__FPU_USED == 1)) */

#if (DISABLE_WDOG)
  WDOG->CNT = WDOG_UPDATE_KEY;
  WDOG->TOVAL = 0xFFFF;
  WDOG->CS = (uint32_t) ((WDOG->CS) & ~WDOG_CS_EN_MASK) | WDOG_CS_UPDATE_MASK;
#endif /* (DISABLE_WDOG) */

  /* Initialize Cache */
  /* Enable Code Bus Cache */
  /* set command to invalidate all ways, enable write buffer
     and write GO bit to initiate command */
  LMEM->PCCCR |= LMEM_PCCCR_INVW1_MASK | LMEM_PCCCR_INVW0_MASK;
  LMEM->PCCCR |= LMEM_PCCCR_GO_MASK;
  /* Wait until the command completes */
  while (LMEM->PCCCR & LMEM_PCCCR_GO_MASK) {
  }
  /* Enable cache, enable write buffer */
  LMEM->PCCCR |= (LMEM_PCCCR_ENWRBUF_MASK | LMEM_PCCCR_ENCACHE_MASK);
  __ISB();

  SystemInitHook();
}

/* ----------------------------------------------------------------------------
   -- SystemCoreClockUpdate()
   ---------------------------------------------------------------------------- */

void SystemCoreClockUpdate (void) {

  uint32_t SCGOUTClock;                                 /* Variable to store output clock frequency of the SCG module */
  uint16_t Divider, prediv, multi;
  Divider = ((SCG->CSR & SCG_CSR_DIVCORE_MASK) >> SCG_CSR_DIVCORE_SHIFT) + 1;

  switch ((SCG->CSR & SCG_CSR_SCS_MASK) >> SCG_CSR_SCS_SHIFT) {
    case 0x1:
      /* System OSC */
      SCGOUTClock = CPU_XTAL_CLK_HZ;
      break;
    case 0x2:
      /* Slow IRC */
      SCGOUTClock = (((SCG->SIRCCFG & SCG_SIRCCFG_RANGE_MASK) >> SCG_SIRCCFG_RANGE_SHIFT) ? 8000000 : 2000000);
      break;
    case 0x3:
      /* Fast IRC */
      SCGOUTClock = 48000000 + ((SCG->FIRCCFG & SCG_FIRCCFG_RANGE_MASK) >> SCG_FIRCCFG_RANGE_SHIFT) * 4000000;
      break;
    case 0x6:
      /* System PLL */
      if ((SCG->SPLLCFG & SCG_SPLLCFG_SOURCE_MASK) >> SCG_SPLLCFG_SOURCE_SHIFT) {
        SCGOUTClock = 48000000 + ((SCG->FIRCCFG & SCG_FIRCCFG_RANGE_MASK) >> SCG_FIRCCFG_RANGE_SHIFT) * 4000000;
      }
      else {
        SCGOUTClock = CPU_XTAL_CLK_HZ;
      }
      prediv = ((SCG->SPLLCFG & SCG_SPLLCFG_PREDIV_MASK) >> SCG_SPLLCFG_PREDIV_SHIFT) + 1;
      multi = ((SCG->SPLLCFG & SCG_SPLLCFG_MULT_MASK) >> SCG_SPLLCFG_MULT_SHIFT) + 16;
      SCGOUTClock = SCGOUTClock * multi / (prediv * 2);
      break;
    default:
      return;
  }
  SystemCoreClock = (SCGOUTClock / Divider);

}

/* ----------------------------------------------------------------------------
   -- SystemInitHook()
   ---------------------------------------------------------------------------- */

__attribute__ ((weak)) void SystemInitHook (void) {
  /* Void implementation of the weak function. */
}
