/*
** ###################################################################
**     Processors:          MIMXRT1052CVJ5B
**                          MIMXRT1052CVL5B
**                          MIMXRT1052DVJ6B
**                          MIMXRT1052DVL6B
**
**     Compilers:           Freescale C/C++ for Embedded ARM
**                          GNU C Compiler
**                          IAR ANSI C/C++ Compiler for ARM
**                          Keil ARM C/C++ Compiler
**                          MCUXpresso Compiler
**
**     Reference manual:    IMXRT1050RM Rev.2.1, 12/2018
**     Version:             rev. 1.2, 2018-11-27
**     Build:               b181205
**
**     Abstract:
**         Provides a system configuration function and a global variable that
**         contains the system frequency. It configures the device and initializes
**         the oscillator (PLL) that is part of the microcontroller device.
**
**     Copyright 2016 Freescale Semiconductor, Inc.
**     Copyright 2016-2018 NXP
**     All rights reserved.
**
**     SPDX-License-Identifier: BSD-3-Clause
**
**     http:                 www.nxp.com
**     mail:                 support@nxp.com
**
**     Revisions:
**     - rev. 0.1 (2017-01-10)
**         Initial version.
**     - rev. 1.0 (2018-09-21)
**         Update interrupt vector table and dma request source.
**         Update register BEE_ADDR_OFFSET1's bitfield name to ADDR_OFFSET1.
**         Split GPIO_COMBINED_IRQS to GPIO_COMBINED_LOW_IRQS and GPIO_COMBINED_HIGH_IRQS.
**     - rev. 1.1 (2018-11-16)
**         Update header files to align with IMXRT1050RM Rev.1.
**     - rev. 1.2 (2018-11-27)
**         Update header files to align with IMXRT1050RM Rev.2.1.
**
** ###################################################################
*/

/*!
 * @file MIMXRT1052
 * @version 1.2
 * @date 2018-11-27
 * @brief Device specific configuration file for MIMXRT1052 (implementation file)
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

#if defined(__MCUXPRESSO)
    extern uint32_t g_pfnVectors[];  // Vector table defined in startup code
    SCB->VTOR = (uint32_t)g_pfnVectors;
#endif

/* Disable Watchdog Power Down Counter */
WDOG1->WMCR &= ~WDOG_WMCR_PDE_MASK;
WDOG2->WMCR &= ~WDOG_WMCR_PDE_MASK;

/* Watchdog disable */

#if (DISABLE_WDOG)
    if (WDOG1->WCR & WDOG_WCR_WDE_MASK)
    {
        WDOG1->WCR &= ~WDOG_WCR_WDE_MASK;
    }
    if (WDOG2->WCR & WDOG_WCR_WDE_MASK)
    {
        WDOG2->WCR &= ~WDOG_WCR_WDE_MASK;
    }
    RTWDOG->CNT = 0xD928C520U; /* 0xD928C520U is the update key */
    RTWDOG->TOVAL = 0xFFFF;
    RTWDOG->CS = (uint32_t) ((RTWDOG->CS) & ~RTWDOG_CS_EN_MASK) | RTWDOG_CS_UPDATE_MASK;
#endif /* (DISABLE_WDOG) */

    /* Disable Systick which might be enabled by bootrom */
    if (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk)
    {
        SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    }

/* Enable instruction and data caches */
#if defined(__ICACHE_PRESENT) && __ICACHE_PRESENT
    if (SCB_CCR_IC_Msk != (SCB_CCR_IC_Msk & SCB->CCR)) {
        SCB_EnableICache();
    }
#endif
#if defined(__DCACHE_PRESENT) && __DCACHE_PRESENT
    if (SCB_CCR_DC_Msk != (SCB_CCR_DC_Msk & SCB->CCR)) {
        SCB_EnableDCache();
    }
#endif

  SystemInitHook();
}

/* ----------------------------------------------------------------------------
   -- SystemCoreClockUpdate()
   ---------------------------------------------------------------------------- */

void SystemCoreClockUpdate (void) {

    uint32_t freq;
    uint32_t PLL1MainClock;
    uint32_t PLL2MainClock;

    /* Periph_clk2_clk ---> Periph_clk */
    if (CCM->CBCDR & CCM_CBCDR_PERIPH_CLK_SEL_MASK)
    {
        switch (CCM->CBCMR & CCM_CBCMR_PERIPH_CLK2_SEL_MASK)
        {
            /* Pll3_sw_clk ---> Periph_clk2_clk ---> Periph_clk */
            case CCM_CBCMR_PERIPH_CLK2_SEL(0U):
                if(CCM_ANALOG->PLL_USB1 & CCM_ANALOG_PLL_USB1_BYPASS_MASK)
                {
                    freq = (((CCM_ANALOG->PLL_USB1 & CCM_ANALOG_PLL_USB1_BYPASS_CLK_SRC_MASK) >> CCM_ANALOG_PLL_USB1_BYPASS_CLK_SRC_SHIFT) == 0U) ?
                           CPU_XTAL_CLK_HZ : CPU_CLK1_HZ;
                }
                else
                {
                    freq = (CPU_XTAL_CLK_HZ * ((CCM_ANALOG->PLL_USB1 & CCM_ANALOG_PLL_USB1_DIV_SELECT_MASK) ? 22U : 20U));
                }
                break;

            /* Osc_clk ---> Periph_clk2_clk ---> Periph_clk */
            case CCM_CBCMR_PERIPH_CLK2_SEL(1U):
                freq = CPU_XTAL_CLK_HZ;
                break;

            case CCM_CBCMR_PERIPH_CLK2_SEL(2U):
                freq = (((CCM_ANALOG->PLL_SYS & CCM_ANALOG_PLL_SYS_BYPASS_CLK_SRC_MASK) >> CCM_ANALOG_PLL_SYS_BYPASS_CLK_SRC_SHIFT) == 0U) ?
                   CPU_XTAL_CLK_HZ : CPU_CLK1_HZ;

            case CCM_CBCMR_PERIPH_CLK2_SEL(3U):
            default:
                freq = 0U;
                break;
        }

        freq /= (((CCM->CBCDR & CCM_CBCDR_PERIPH_CLK2_PODF_MASK) >> CCM_CBCDR_PERIPH_CLK2_PODF_SHIFT) + 1U);
    }
    /* Pre_Periph_clk ---> Periph_clk */
    else
    {
        /* check if pll is bypassed */
        if(CCM_ANALOG->PLL_ARM & CCM_ANALOG_PLL_ARM_BYPASS_MASK)
        {
            PLL1MainClock = (((CCM_ANALOG->PLL_ARM & CCM_ANALOG_PLL_ARM_BYPASS_CLK_SRC_MASK) >> CCM_ANALOG_PLL_ARM_BYPASS_CLK_SRC_SHIFT) == 0U) ?
                   CPU_XTAL_CLK_HZ : CPU_CLK1_HZ;
        }
        else
        {
            PLL1MainClock = ((CPU_XTAL_CLK_HZ * ((CCM_ANALOG->PLL_ARM & CCM_ANALOG_PLL_ARM_DIV_SELECT_MASK) >>
                                             CCM_ANALOG_PLL_ARM_DIV_SELECT_SHIFT)) >> 1U);
        }

        /* check if pll is bypassed */
        if(CCM_ANALOG->PLL_SYS & CCM_ANALOG_PLL_SYS_BYPASS_MASK)
        {
            PLL2MainClock = (((CCM_ANALOG->PLL_SYS & CCM_ANALOG_PLL_SYS_BYPASS_CLK_SRC_MASK) >> CCM_ANALOG_PLL_SYS_BYPASS_CLK_SRC_SHIFT) == 0U) ?
                   CPU_XTAL_CLK_HZ : CPU_CLK1_HZ;
        }
        else
        {
            PLL2MainClock = (CPU_XTAL_CLK_HZ * ((CCM_ANALOG->PLL_SYS & CCM_ANALOG_PLL_SYS_DIV_SELECT_MASK) ? 22U : 20U));
        }
        PLL2MainClock += ((uint64_t)CPU_XTAL_CLK_HZ * ((uint64_t)(CCM_ANALOG->PLL_SYS_NUM))) / ((uint64_t)(CCM_ANALOG->PLL_SYS_DENOM));


        switch (CCM->CBCMR & CCM_CBCMR_PRE_PERIPH_CLK_SEL_MASK)
        {
            /* PLL2 ---> Pre_Periph_clk ---> Periph_clk */
            case CCM_CBCMR_PRE_PERIPH_CLK_SEL(0U):
                freq = PLL2MainClock;
                break;

            /* PLL2 PFD2 ---> Pre_Periph_clk ---> Periph_clk */
            case CCM_CBCMR_PRE_PERIPH_CLK_SEL(1U):
                freq = PLL2MainClock / ((CCM_ANALOG->PFD_528 & CCM_ANALOG_PFD_528_PFD2_FRAC_MASK) >> CCM_ANALOG_PFD_528_PFD2_FRAC_SHIFT) * 18U;
                break;

            /* PLL2 PFD0 ---> Pre_Periph_clk ---> Periph_clk */
            case CCM_CBCMR_PRE_PERIPH_CLK_SEL(2U):
                freq = PLL2MainClock / ((CCM_ANALOG->PFD_528 & CCM_ANALOG_PFD_528_PFD0_FRAC_MASK) >> CCM_ANALOG_PFD_528_PFD0_FRAC_SHIFT) * 18U;
                break;

            /* PLL1 divided(/2) ---> Pre_Periph_clk ---> Periph_clk */
            case CCM_CBCMR_PRE_PERIPH_CLK_SEL(3U):
                freq = PLL1MainClock / (((CCM->CACRR & CCM_CACRR_ARM_PODF_MASK) >> CCM_CACRR_ARM_PODF_SHIFT) + 1U);
                break;

            default:
                freq = 0U;
                break;
        }
    }

    SystemCoreClock = (freq / (((CCM->CBCDR & CCM_CBCDR_AHB_PODF_MASK) >> CCM_CBCDR_AHB_PODF_SHIFT) + 1U));

}

/* ----------------------------------------------------------------------------
   -- SystemInitHook()
   ---------------------------------------------------------------------------- */

__attribute__ ((weak)) void SystemInitHook (void) {
  /* Void implementation of the weak function. */
}
