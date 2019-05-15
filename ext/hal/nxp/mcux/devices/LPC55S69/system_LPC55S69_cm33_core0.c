/*
** ###################################################################
**     Processors:          LPC55S69JBD100_cm33_core0
**                          LPC55S69JET98_cm33_core0
**
**     Compilers:           GNU C Compiler
**                          IAR ANSI C/C++ Compiler for ARM
**                          Keil ARM C/C++ Compiler
**                          MCUXpresso Compiler
**
**     Reference manual:    LPC55xx/LPC55Sxx User manual Rev.0.4  25 Sep 2018
**     Version:             rev. 1.0, 2018-08-22
**     Build:               b181219
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
**     - rev. 1.0 (2018-08-22)
**         Initial version based on v0.2UM
**
** ###################################################################
*/

/*!
 * @file LPC55S69_cm33_core0
 * @version 1.0
 * @date 2018-08-22
 * @brief Device specific configuration file for LPC55S69_cm33_core0
 *        (implementation file)
 *
 * Provides a system configuration function and a global variable that contains
 * the system frequency. It configures the device and initializes the oscillator
 * (PLL) that is part of the microcontroller device.
 */

#include <stdint.h>
#include "fsl_device_registers.h"

/* PLL0 SSCG control1 */
#define PLL_SSCG_MD_FRACT_P 0U
#define PLL_SSCG_MD_INT_P 25U
#define PLL_SSCG_MD_FRACT_M (0x1FFFFFFUL << PLL_SSCG_MD_FRACT_P)
#define PLL_SSCG_MD_INT_M ((uint64_t)0xFFUL << PLL_SSCG_MD_INT_P)

/* Get predivider (N) from PLL0 NDEC setting */
static uint32_t findPll0PreDiv(void)
{
    uint32_t preDiv = 1;

    /* Direct input is not used? */
    if ((SYSCON->PLL0CTRL & SYSCON_PLL0CTRL_BYPASSPREDIV_MASK) == 0)
    {
        preDiv = SYSCON->PLL0NDEC & SYSCON_PLL0NDEC_NDIV_MASK;
        if (preDiv == 0)
        {
            preDiv = 1;
        }
    }
    return preDiv;
}

/* Get postdivider (P) from PLL0 PDEC setting */
static uint32_t findPll0PostDiv(void)
{
    uint32_t postDiv = 1;

    if ((SYSCON->PLL0CTRL & SYSCON_PLL0CTRL_BYPASSPOSTDIV_MASK) == 0)
    {
        if (SYSCON->PLL0CTRL & SYSCON_PLL0CTRL_BYPASSPOSTDIV2_MASK)
        {
            postDiv = SYSCON->PLL0PDEC & SYSCON_PLL0PDEC_PDIV_MASK;
        }
        else
        {
            postDiv = 2 * (SYSCON->PLL0PDEC & SYSCON_PLL0PDEC_PDIV_MASK);
        }
        if (postDiv == 0)
        {
            postDiv = 2;
        }
    }
    return postDiv;
}

/* Get multiplier (M) from PLL0 SSCG and SEL_EXT settings */
static float findPll0MMult(void)
{
    float mMult = 1;
    float mMult_fract;
    uint32_t mMult_int;

    if (SYSCON->PLL0SSCG1 & SYSCON_PLL0SSCG1_SEL_EXT_MASK)
    {
        mMult = (SYSCON->PLL0SSCG1 & SYSCON_PLL0SSCG1_MDIV_EXT_MASK) >> SYSCON_PLL0SSCG1_MDIV_EXT_SHIFT;
    }
    else
    {
        mMult_int = ((SYSCON->PLL0SSCG1 & SYSCON_PLL0SSCG1_MD_MBS_MASK) << 7U) | ((SYSCON->PLL0SSCG0) >> PLL_SSCG_MD_INT_P);
        mMult_fract = ((float)((SYSCON->PLL0SSCG0) & PLL_SSCG_MD_FRACT_M)/(1 << PLL_SSCG_MD_INT_P));
        mMult = (float)mMult_int + mMult_fract;
    }
    if (mMult == 0)
    {
        mMult = 1;
    }
    return mMult;
}

/* Get predivider (N) from PLL1 NDEC setting */
static uint32_t findPll1PreDiv(void)
{
    uint32_t preDiv = 1;

    /* Direct input is not used? */
    if ((SYSCON->PLL1CTRL & SYSCON_PLL1CTRL_BYPASSPREDIV_MASK) == 0)
    {
        preDiv = SYSCON->PLL1NDEC & SYSCON_PLL1NDEC_NDIV_MASK;
        if (preDiv == 0)
        {
            preDiv = 1;
        }
    }
    return preDiv;
}

/* Get postdivider (P) from PLL1 PDEC setting */
static uint32_t findPll1PostDiv(void)
{
    uint32_t postDiv = 1;

    if ((SYSCON->PLL1CTRL & SYSCON_PLL1CTRL_BYPASSPOSTDIV_MASK) == 0)
    {
        if (SYSCON->PLL1CTRL & SYSCON_PLL1CTRL_BYPASSPOSTDIV2_MASK)
        {
            postDiv = SYSCON->PLL1PDEC & SYSCON_PLL1PDEC_PDIV_MASK;
        }
        else
        {
            postDiv = 2 * (SYSCON->PLL1PDEC & SYSCON_PLL1PDEC_PDIV_MASK);
        }
        if (postDiv == 0)
        {
            postDiv = 2;
        }
    }
    return postDiv;
}

/* Get multiplier (M) from PLL1 MDEC settings */
static uint32_t findPll1MMult(void)
{
    uint32_t mMult = 1;

    mMult = SYSCON->PLL1MDEC & SYSCON_PLL1MDEC_MDIV_MASK;

    if (mMult == 0)
    {
        mMult = 1;
    }
    return mMult;
}

/* Get FRO 12M Clk */
/*! brief  Return Frequency of FRO 12MHz
 *  return Frequency of FRO 12MHz
 */
static uint32_t CLOCK_GetFro12MFreq(void)
{
    return (PMC->PDRUNCFG0 & PMC_PDRUNCFG0_PDEN_FRO192M_MASK) ?
               0 :
               (ANACTRL->FRO192M_CTRL & ANACTRL_FRO192M_CTRL_ENA_12MHZCLK_MASK) ? 12000000U : 0U;
}

/* Get FRO 1M Clk */
/*! brief  Return Frequency of FRO 1MHz
 *  return Frequency of FRO 1MHz
 */
static uint32_t CLOCK_GetFro1MFreq(void)
{
    return (SYSCON->CLOCK_CTRL & SYSCON_CLOCK_CTRL_FRO1MHZ_CLK_ENA_MASK) ? 1000000U : 0U;
}

/* Get EXT OSC Clk */
/*! brief  Return Frequency of External Clock
 *  return Frequency of External Clock. If no external clock is used returns 0.
 */
static uint32_t CLOCK_GetExtClkFreq(void)
{
    return (ANACTRL->XO32M_CTRL & ANACTRL_XO32M_CTRL_ENABLE_SYSTEM_CLK_OUT_MASK) ? CLK_CLK_IN : 0U;
}

/* Get HF FRO Clk */
/*! brief  Return Frequency of High-Freq output of FRO
 *  return Frequency of High-Freq output of FRO
 */
static uint32_t CLOCK_GetFroHfFreq(void)
{
    return (PMC->PDRUNCFG0 & PMC_PDRUNCFG0_PDEN_FRO192M_MASK) ?
               0 :
               (ANACTRL->FRO192M_CTRL & ANACTRL_FRO192M_CTRL_ENA_96MHZCLK_MASK) ? 96000000U : 0U;
}

/* Get RTC OSC Clk */
/*! brief  Return Frequency of 32kHz osc
 *  return Frequency of 32kHz osc
 */
static uint32_t CLOCK_GetOsc32KFreq(void)
{
    return ((~(PMC->PDRUNCFG0 & PMC_PDRUNCFG0_PDEN_FRO32K_MASK)) && (PMC->RTCOSC32K & PMC_RTCOSC32K_SEL(0))) ?
               CLK_RTC_32K_CLK :
               ((~(PMC->PDRUNCFG0 & PMC_PDRUNCFG0_PDEN_XTAL32K_MASK)) && (PMC->RTCOSC32K & PMC_RTCOSC32K_SEL(1))) ?
               CLK_RTC_32K_CLK :
               0U;
}



/* ----------------------------------------------------------------------------
   -- Core clock
   ---------------------------------------------------------------------------- */

uint32_t SystemCoreClock = DEFAULT_SYSTEM_CLOCK;

/* ----------------------------------------------------------------------------
   -- SystemInit()
   ---------------------------------------------------------------------------- */

__attribute__ ((weak)) void SystemInit (void) {
#if ((__FPU_PRESENT == 1) && (__FPU_USED == 1))
  SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));    /* set CP10, CP11 Full Access */
#endif /* ((__FPU_PRESENT == 1) && (__FPU_USED == 1)) */

  SCB->CPACR |= ((3UL << 0*2) | (3UL << 1*2));    /* set CP0, CP1 Full Access (enable PowerQuad) */

  SCB->NSACR |= ((3UL << 0) | (3UL << 10));   /* enable CP0, CP1, CP10, CP11 Non-secure Access */

#if defined(__MCUXPRESSO)
    extern void(*const g_pfnVectors[]) (void);
    SCB->VTOR = (uint32_t) &g_pfnVectors;
#else
    extern void *__Vectors;
    SCB->VTOR = (uint32_t) &__Vectors;
#endif
    SYSCON->TRACECLKDIV = 0;
/* Optionally enable RAM banks that may be off by default at reset */
#if !defined(DONT_ENABLE_DISABLED_RAMBANKS)
    SYSCON->AHBCLKCTRLSET[0] = SYSCON_AHBCLKCTRL0_SRAM_CTRL1_MASK | SYSCON_AHBCLKCTRL0_SRAM_CTRL2_MASK
                          | SYSCON_AHBCLKCTRL0_SRAM_CTRL3_MASK | SYSCON_AHBCLKCTRL0_SRAM_CTRL4_MASK;
#endif
  SystemInitHook();
}

/* ----------------------------------------------------------------------------
   -- SystemCoreClockUpdate()
   ---------------------------------------------------------------------------- */

void SystemCoreClockUpdate (void) {
    uint32_t clkRate = 0;
    uint32_t prediv, postdiv;
    float workRate;
    uint64_t workRate1;

    switch (SYSCON->MAINCLKSELB & SYSCON_MAINCLKSELB_SEL_MASK)
    {
        case 0x00: /* MAINCLKSELA clock (main_clk_a)*/
            switch (SYSCON->MAINCLKSELA & SYSCON_MAINCLKSELA_SEL_MASK)
            {
                case 0x00: /* FRO 12 MHz (fro_12m) */
                    clkRate = CLOCK_GetFro12MFreq();
                    break;
                case 0x01: /* CLKIN (clk_in) */
                    clkRate = CLOCK_GetExtClkFreq();
                    break;
                case 0x02: /* Fro 1MHz (fro_1m) */
                    clkRate = CLOCK_GetFro1MFreq();
                    break;
                default: /* = 0x03 = FRO 96 MHz (fro_hf) */
                    clkRate = CLOCK_GetFroHfFreq();
                    break;
            }
            break;
        case 0x01: /* PLL0 clock (pll0_clk)*/
            switch (SYSCON->PLL0CLKSEL & SYSCON_PLL0CLKSEL_SEL_MASK)
            {
                case 0x00: /* FRO 12 MHz (fro_12m) */
                    clkRate = CLOCK_GetFro12MFreq();
                    break;
                case 0x01: /* CLKIN (clk_in) */
                    clkRate = CLOCK_GetExtClkFreq();
                    break;
                case 0x02: /* Fro 1MHz (fro_1m) */
                    clkRate = CLOCK_GetFro1MFreq();
                    break;
                case 0x03: /* RTC oscillator 32 kHz output (32k_clk) */
                    clkRate = CLOCK_GetOsc32KFreq();
                    break;
                default:
                    break;
            }
            if (((SYSCON->PLL0CTRL & SYSCON_PLL0CTRL_BYPASSPLL_MASK) == 0) && (SYSCON->PLL0CTRL & SYSCON_PLL0CTRL_CLKEN_MASK) && ((PMC->PDRUNCFG0 & PMC_PDRUNCFG0_PDEN_PLL0_MASK) == 0) && ((PMC->PDRUNCFG0 & PMC_PDRUNCFG0_PDEN_PLL0_SSCG_MASK) == 0))
            {
                prediv = findPll0PreDiv();
                postdiv = findPll0PostDiv();
                /* Adjust input clock */
                clkRate = clkRate / prediv;
                /* MDEC used for rate */
                workRate = (float)clkRate * (float)findPll0MMult();
                clkRate = (uint32_t)(workRate / ((float)postdiv));
            }
            break;
        case 0x02: /* PLL1 clock (pll1_clk)*/
            switch (SYSCON->PLL1CLKSEL & SYSCON_PLL1CLKSEL_SEL_MASK)
            {
                case 0x00: /* FRO 12 MHz (fro_12m) */
                    clkRate = CLOCK_GetFro12MFreq();
                    break;
                case 0x01: /* CLKIN (clk_in) */
                    clkRate = CLOCK_GetExtClkFreq();
                    break;
                case 0x02: /* Fro 1MHz (fro_1m) */
                    clkRate = CLOCK_GetFro1MFreq();
                    break;
                case 0x03: /* RTC oscillator 32 kHz output (32k_clk) */
                    clkRate = CLOCK_GetOsc32KFreq();
                    break;
                default:
                    break;
            }
            if (((SYSCON->PLL1CTRL & SYSCON_PLL1CTRL_BYPASSPLL_MASK) == 0) && (SYSCON->PLL1CTRL & SYSCON_PLL1CTRL_CLKEN_MASK) && ((PMC->PDRUNCFG0 & PMC_PDRUNCFG0_PDEN_PLL1_MASK) == 0))
            {
                /* PLL is not in bypass mode, get pre-divider, post-divider, and M divider */
                prediv = findPll1PreDiv();
                postdiv = findPll1PostDiv();
                /* Adjust input clock */
                clkRate = clkRate / prediv;

                /* MDEC used for rate */
                workRate1 = (uint64_t)clkRate * (uint64_t)findPll1MMult();
                clkRate = workRate1 / ((uint64_t)postdiv);
            }
            break;
        case 0x03: /* RTC oscillator 32 kHz output (32k_clk) */
            clkRate = CLOCK_GetOsc32KFreq();
            break;
        default:
            break;
    }
    SystemCoreClock = clkRate / ((SYSCON->AHBCLKDIV & 0xFF) + 1);
}

/* ----------------------------------------------------------------------------
   -- SystemInitHook()
   ---------------------------------------------------------------------------- */

__attribute__ ((weak)) void SystemInitHook (void) {
  /* Void implementation of the weak function. */
}
