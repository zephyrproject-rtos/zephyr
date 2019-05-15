/*
 * Copyright (c) 2017 - 2018 , NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_common.h"
#include "fsl_clock.h"
#include "fsl_power.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.clock"
#endif
#define NVALMAX (0x100U)
#define PVALMAX (0x20U)
#define MVALMAX (0x10000U)

#define PLL_MAX_N_DIV 0x100U

/*--------------------------------------------------------------------------
!!! If required these #defines can be moved to chip library file
----------------------------------------------------------------------------*/

#define PLL_SSCG1_MDEC_VAL_P (10U)                                  /* MDEC is in bits  16 downto 0 */
#define PLL_SSCG1_MDEC_VAL_M (0x3FFFC00ULL << PLL_SSCG1_MDEC_VAL_P) /* NDEC is in bits  9 downto 0 */
#define PLL_NDEC_VAL_P (0U)                                         /* NDEC is in bits  9:0 */
#define PLL_NDEC_VAL_M (0xFFUL << PLL_NDEC_VAL_P)
#define PLL_PDEC_VAL_P (0U) /*!<  PDEC is in bits 6:0 */
#define PLL_PDEC_VAL_M (0x1FUL << PLL_PDEC_VAL_P)

#define PLL_MIN_CCO_FREQ_MHZ (275000000U)
#define PLL_MAX_CCO_FREQ_MHZ (550000000U)
#define PLL_LOWER_IN_LIMIT (2000U) /*!<  Minimum PLL input rate */
#define PLL_HIGHER_IN_LIMIT (150000000U) /*!<  Maximum PLL input rate */
#define PLL_MIN_IN_SSMODE (3000000U)
#define PLL_MAX_IN_SSMODE (100000000U) /*!<  Not find the value in UM, Just use the maximum frequency which device support */

/* PLL NDEC reg */
#define PLL_NDEC_VAL_SET(value) (((unsigned long)(value) << PLL_NDEC_VAL_P) & PLL_NDEC_VAL_M)
/* PLL PDEC reg */
#define PLL_PDEC_VAL_SET(value) (((unsigned long)(value) << PLL_PDEC_VAL_P) & PLL_PDEC_VAL_M)
/* SSCG control0 */
#define PLL_SSCG1_MDEC_VAL_SET(value) (((unsigned long)(value) << PLL_SSCG1_MDEC_VAL_P) & PLL_SSCG1_MDEC_VAL_M)

/* PLL0 SSCG control1 */
#define PLL0_SSCG_MD_FRACT_P 0U
#define PLL0_SSCG_MD_INT_P 25U
#define PLL0_SSCG_MD_FRACT_M (0x1FFFFFFUL << PLL0_SSCG_MD_FRACT_P)
#define PLL0_SSCG_MD_INT_M ((uint64_t)0xFFUL << PLL0_SSCG_MD_INT_P)

#define PLL0_SSCG_MD_FRACT_SET(value) (((uint64_t)(value) << PLL0_SSCG_MD_FRACT_P) & PLL0_SSCG_MD_FRACT_M)
#define PLL0_SSCG_MD_INT_SET(value) (((uint64_t)(value) << PLL0_SSCG_MD_INT_P) & PLL0_SSCG_MD_INT_M)

/* Saved value of PLL output rate, computed whenever needed to save run-time
   computation on each call to retrive the PLL rate. */
static uint32_t s_Pll0_Freq;
static uint32_t s_Pll1_Freq;

/** External clock rate on the CLKIN pin in Hz. If not used,
    set this to 0. Otherwise, set it to the exact rate in Hz this pin is
    being driven at. */
static uint32_t s_Ext_Clk_Freq = 16000000U;
static uint32_t s_I2S_Mclk_Freq = 0U;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/* Find SELP, SELI, and SELR values for raw M value, max M = MVALMAX */
static void pllFindSel(uint32_t M, uint32_t *pSelP, uint32_t *pSelI, uint32_t *pSelR);
/* Get predivider (N) from PLL0 NDEC setting */
static uint32_t findPll0PreDiv(void);
/* Get predivider (N) from PLL1 NDEC setting */
static uint32_t findPll1PreDiv(void);
/* Get postdivider (P) from PLL0 PDEC setting */
static uint32_t findPll0PostDiv(void);
/* Get multiplier (M) from PLL0 MDEC and SSCG settings */
static float findPll0MMult(void);
/* Get the greatest common divisor */
static uint32_t FindGreatestCommonDivisor(uint32_t m, uint32_t n);
/* Set PLL output based on desired output rate */
static pll_error_t CLOCK_GetPll0Config(
    uint32_t finHz, uint32_t foutHz, pll_setup_t *pSetup, bool useSS);
/* Update local PLL rate variable */
static void CLOCK_GetPLL0OutFromSetupUpdate(pll_setup_t *pSetup);

/*******************************************************************************
 * Code
 ******************************************************************************/

/* Clock Selection for IP */
/**
 * brief   Configure the clock selection muxes.
 * param   connection  : Clock to be configured.
 * return  Nothing
 */
void CLOCK_AttachClk(clock_attach_id_t connection)
{
    uint8_t mux;
    uint8_t sel;
    uint16_t item;
    uint32_t i;
    volatile uint32_t *pClkSel;

    pClkSel = &(SYSCON->SYSTICKCLKSELX[0]);

    if (connection != kNONE_to_NONE)
    {
        for (i = 0U; i < 2U; i++)
        {
            if (connection == 0U)
            {
                break;
            }
            item = (uint16_t)GET_ID_ITEM(connection);
            if (item)
            {
                mux = GET_ID_ITEM_MUX(item);
                sel = GET_ID_ITEM_SEL(item);
                if (mux == CM_RTCOSC32KCLKSEL)
                {
                    PMC->RTCOSC32K |= sel;
                }
                else
                {
                    pClkSel[mux] = sel;
                }
            }
            connection = GET_ID_NEXT_ITEM(connection); /* pick up next descriptor */
        }
    }
}

/* Return the actual clock attach id */
/**
 * brief   Get the actual clock attach id.
 * This fuction uses the offset in input attach id, then it reads the actual source value in
 * the register and combine the offset to obtain an actual attach id.
 * param   attachId  : Clock attach id to get.
 * return  Clock source value.
 */
clock_attach_id_t CLOCK_GetClockAttachId(clock_attach_id_t attachId)
{
    uint8_t mux;
    uint8_t actualSel;
    uint32_t i;
    uint32_t actualAttachId = 0U;
    uint32_t selector = GET_ID_SELECTOR(attachId);
    volatile uint32_t *pClkSel;

    pClkSel = &(SYSCON->SYSTICKCLKSELX[0]);

    if (attachId == kNONE_to_NONE)
    {
        return kNONE_to_NONE;
    }

    for (i = 0U; i < 2U; i++)
    {
        mux = GET_ID_ITEM_MUX(attachId);
        if (attachId)
        {
            if (mux == CM_RTCOSC32KCLKSEL)
            {
                actualSel = PMC->RTCOSC32K;
            }
            else
            {
                actualSel = pClkSel[mux];
            }

            /* Consider the combination of two registers */
            actualAttachId |= CLK_ATTACH_ID(mux, actualSel, i);
        }
        attachId = GET_ID_NEXT_ITEM(attachId); /*!<  pick up next descriptor */
    }

    actualAttachId |= selector;

    return (clock_attach_id_t)actualAttachId;
}

/* Set IP Clock Divider */
/**
 * brief   Setup peripheral clock dividers.
 * param   div_name    : Clock divider name
 * param divided_by_value: Value to be divided
 * param reset :  Whether to reset the divider counter.
 * return  Nothing
 */
void CLOCK_SetClkDiv(clock_div_name_t div_name, uint32_t divided_by_value, bool reset)
{
    volatile uint32_t *pClkDiv;

    pClkDiv = &(SYSCON->SYSTICKCLKDIV0);
    if (reset)
    {
        pClkDiv[div_name] = 1U << 29U;
    }
    if (divided_by_value == 0U) /*!<  halt */
    {
        pClkDiv[div_name] = 1U << 30U;
    }
    else
    {
        pClkDiv[div_name] = (divided_by_value - 1U);
    }
}

/* Set RTC 1KHz Clock Divider */
/**
 * brief   Setup rtc 1khz clock divider.
 * param divided_by_value: Value to be divided
 * return  Nothing
 */
void CLOCK_SetRtc1khzClkDiv(uint32_t divided_by_value)
{
    PMC->RTCOSC32K |= (((divided_by_value - 28U) << PMC_RTCOSC32K_CLK1KHZDIV_SHIFT) | PMC_RTCOSC32K_CLK1KHZDIV_MASK);
}

/* Set RTC 1KHz Clock Divider */
/**
 * brief   Setup rtc 1hz clock divider.
 * param divided_by_value: Value to be divided
 * return  Nothing
 */
void CLOCK_SetRtc1hzClkDiv(uint32_t divided_by_value)
{
    if (divided_by_value == 0U) /*!<  halt */
    {
        PMC->RTCOSC32K |= (1U << PMC_RTCOSC32K_CLK1HZDIVHALT_SHIFT);
    }
    else
    {
        PMC->RTCOSC32K |=
            (((divided_by_value - 31744U) << PMC_RTCOSC32K_CLK1HZDIV_SHIFT) | PMC_RTCOSC32K_CLK1HZDIV_MASK);
    }
}

/* Set FRO Clocking */
/**
 * brief   Initialize the Core clock to given frequency (12, 48 or 96 MHz).
 * Turns on FRO and uses default CCO, if freq is 12000000, then high speed output is off, else high speed output is
 * enabled.
 * param   iFreq   : Desired frequency (must be one of #CLK_FRO_12MHZ or #CLK_FRO_48MHZ or #CLK_FRO_96MHZ)
 * return  returns success or fail status.
 */
status_t CLOCK_SetupFROClocking(uint32_t iFreq)
{
    if ((iFreq != 12000000U) && (iFreq != 48000000U) && (iFreq != 96000000U))
    {
        return kStatus_Fail;
    }
    /* Enable Analog Control module */
    SYSCON->PRESETCTRLCLR[2] = (1U << SYSCON_PRESETCTRL2_ANALOG_CTRL_RST_SHIFT);
    SYSCON->AHBCLKCTRLSET[2] = SYSCON_AHBCLKCTRL2_ANALOG_CTRL_MASK;
    /* Power up the FRO192M */
    POWER_DisablePD(kPDRUNCFG_PD_FRO192M);

    if (iFreq == 96000000U)
    {
        ANACTRL->FRO192M_CTRL |= ANACTRL_FRO192M_CTRL_ENA_96MHZCLK(1);
    }
    else if (iFreq == 48000000U)
    {
        ANACTRL->FRO192M_CTRL |= ANACTRL_FRO192M_CTRL_ENA_48MHZCLK(1);
    }
    else
    {
        ANACTRL->FRO192M_CTRL |= ANACTRL_FRO192M_CTRL_ENA_12MHZCLK(1);
    }
    return 0U;
}

/* Set the FLASH wait states for the passed frequency */
/**
 * brief	Set the flash wait states for the input freuqency.
 * param	iFreq	: Input frequency
 * return	Nothing
 */
void CLOCK_SetFLASHAccessCyclesForFreq(uint32_t iFreq)
{
    uint32_t num_wait_states;
    float f_num_wait_states = 0.00000009 * ((float)iFreq);
    /* Rational : timing is closed at 100MHz+10% tolerance, hence the ¡®9¡¯ in the formula above */
    num_wait_states = (uint32_t)f_num_wait_states;

    /*
     * It is guaranteed by design that "num_wait_states = 8"
     * will fit all frequencies (below and including) 100 MHz.
     */
    if (num_wait_states >= 9)
    {
        num_wait_states = 8;
    }

    /* Don't alter other bits */
    SYSCON->FMCCR = (SYSCON->FMCCR & ~SYSCON_FMCCR_FMCTIM_MASK) |
                    ((num_wait_states  << SYSCON_FMCCR_FMCTIM_SHIFT) & SYSCON_FMCCR_FMCTIM_MASK);
}

/* Set EXT OSC Clk */
/**
 * brief   Initialize the external osc clock to given frequency.
 * param   iFreq   : Desired frequency (must be equal to exact rate in Hz)
 * return  returns success or fail status.
 */
status_t CLOCK_SetupExtClocking(uint32_t iFreq)
{
    if (iFreq >= 32000000U)
    {
        return kStatus_Fail;
    }
    /* Turn on power for crystal 32 MHz */
    POWER_DisablePD(kPDRUNCFG_PD_XTAL32M);
    POWER_DisablePD(kPDRUNCFG_PD_LDOXO32M);
    /* Enable clock_in clock for clock module. */
    SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_CLKIN_ENA_MASK;

    s_Ext_Clk_Freq = iFreq;
    return 0U;
}

/* Set I2S MCLK Clk */
/**
 * brief   Initialize the I2S MCLK clock to given frequency.
 * param   iFreq   : Desired frequency (must be equal to exact rate in Hz)
 * return  returns success or fail status.
 */
status_t CLOCK_SetupI2SMClkClocking(uint32_t iFreq)
{
    s_I2S_Mclk_Freq = iFreq;
    return 0U;
}

/* Get CLOCK OUT Clk */
/*! brief  Return Frequency of ClockOut
 *  return Frequency of ClockOut
 */
uint32_t CLOCK_GetClockOutClkFreq(void)
{
    uint32_t freq = 0U;

    switch (SYSCON->CLKOUTSEL)
    {
        case 0U:
            freq = CLOCK_GetCoreSysClkFreq();
            break;

        case 1U:
            freq = CLOCK_GetPll0OutFreq();
            break;

        case 2U:
            freq = CLOCK_GetExtClkFreq();
            break;

        case 3U:
            freq = CLOCK_GetFroHfFreq();
            break;

        case 4U:
            freq = CLOCK_GetFro1MFreq();
            break;

        case 5U:
            freq = CLOCK_GetPll1OutFreq();
            break;

        case 6U:
            freq = CLOCK_GetOsc32KFreq();
            break;

        case 7U:
            freq = 0U;
            break;

        default:
            break;
    }
    return freq / ((SYSCON->CLKOUTDIV & 0xffU) + 1U);
}

/* Get ADC Clk */
/*! brief  Return Frequency of Adc Clock
 *  return Frequency of Adc.
 */
uint32_t CLOCK_GetAdcClkFreq(void)
{
    uint32_t freq = 0U;

    switch (SYSCON->ADCCLKSEL)
    {
        case 0U:
            freq = CLOCK_GetCoreSysClkFreq();
            break;
        case 1U:
            freq = CLOCK_GetPll0OutFreq();
            break;
        case 2U:
            freq = CLOCK_GetFroHfFreq();
            break;
        case 7U:
            freq = 0U;
            break;

        default:
            break;
    }

    return freq / ((SYSCON->ADCCLKDIV & SYSCON_ADCCLKDIV_DIV_MASK) + 1U);
}

/* Get USB0 Clk */
/*! brief  Return Frequency of Usb0 Clock
 *  return Frequency of Usb0 Clock.
 */
uint32_t CLOCK_GetUsb0ClkFreq(void)
{
    uint32_t freq = 0U;

    switch (SYSCON->USB0CLKSEL)
    {
        case 0U:
            freq = CLOCK_GetCoreSysClkFreq();
            break;
        case 1U:
            freq = CLOCK_GetPll0OutFreq();
            break;
        case 3U:
            freq = CLOCK_GetFroHfFreq();
            break;
        case 5U:
            freq = CLOCK_GetPll1OutFreq();
            break;
        case 7U:
            freq = 0U;
            break;

        default:
            break;
    }

    return freq / ((SYSCON->USB0CLKDIV & 0xffU) + 1U);
}

/* Get USB1 Clk */
/*! brief  Return Frequency of Usb1 Clock
 *  return Frequency of Usb1 Clock.
 */
uint32_t CLOCK_GetUsb1ClkFreq(void)
{
    return (ANACTRL->XO32M_CTRL & ANACTRL_XO32M_CTRL_ENABLE_PLL_USB_OUT_MASK) ? s_Ext_Clk_Freq : 0U;
}

/* Get MCLK Clk */
/*! brief  Return Frequency of MClk Clock
 *  return Frequency of MClk Clock.
 */
uint32_t CLOCK_GetMclkClkFreq(void)
{
    uint32_t freq = 0U;

    switch (SYSCON->MCLKCLKSEL)
    {
        case 0U:
            freq = CLOCK_GetFroHfFreq();
            break;
        case 1U:
            freq = CLOCK_GetPll0OutFreq();
            break;
        case 7U:
            freq = 0U;
            break;

        default:
            break;
    }

    return freq / ((SYSCON->MCLKDIV & 0xffU) + 1U);
}

/* Get SCTIMER Clk */
/*! brief  Return Frequency of SCTimer Clock
 *  return Frequency of SCTimer Clock.
 */
uint32_t CLOCK_GetSctClkFreq(void)
{
    uint32_t freq = 0U;

    switch (SYSCON->SCTCLKSEL)
    {
        case 0U:
            freq = CLOCK_GetCoreSysClkFreq();
            break;
        case 1U:
            freq = CLOCK_GetPll0OutFreq();
            break;
        case 2U:
            freq = CLOCK_GetExtClkFreq();
            break;
        case 3U:
            freq = CLOCK_GetFroHfFreq();
            break;
        case 5U:
            freq = CLOCK_GetI2SMClkFreq();
            break;
        case 7U:
            freq = 0U;
            break;

        default:
            break;
    }

    return freq / ((SYSCON->SCTCLKDIV & 0xffU) + 1U);
}

/* Get SDIO Clk */
/*! brief  Return Frequency of SDIO Clock
 *  return Frequency of SDIO Clock.
 */
uint32_t CLOCK_GetSdioClkFreq(void)
{
    uint32_t freq = 0U;

    switch (SYSCON->SDIOCLKSEL)
    {
        case 0U:
            freq = CLOCK_GetCoreSysClkFreq();
            break;
        case 1U:
            freq = CLOCK_GetPll0OutFreq();
            break;
        case 3U:
            freq = CLOCK_GetFroHfFreq();
            break;
        case 5U:
            freq = CLOCK_GetPll1OutFreq();
            break;
        case 7U:
            freq = 0U;
            break;
        default:
            break;
    }

    return freq / ((SYSCON->SDIOCLKDIV & 0xffU) + 1U);
}

/* Get FRO 12M Clk */
/*! brief  Return Frequency of FRO 12MHz
 *  return Frequency of FRO 12MHz
 */
uint32_t CLOCK_GetFro12MFreq(void)
{
    return (PMC->PDRUNCFG0 & PMC_PDRUNCFG0_PDEN_FRO192M_MASK) ?
               0 :
               (ANACTRL->FRO192M_CTRL & ANACTRL_FRO192M_CTRL_ENA_12MHZCLK_MASK) ? 12000000U : 0U;
}

/* Get FRO 1M Clk */
/*! brief  Return Frequency of FRO 1MHz
 *  return Frequency of FRO 1MHz
 */
uint32_t CLOCK_GetFro1MFreq(void)
{
    return (SYSCON->CLOCK_CTRL & SYSCON_CLOCK_CTRL_FRO1MHZ_CLK_ENA_MASK) ? 1000000U : 0U;
}

/* Get EXT OSC Clk */
/*! brief  Return Frequency of External Clock
 *  return Frequency of External Clock. If no external clock is used returns 0.
 */
uint32_t CLOCK_GetExtClkFreq(void)
{
    return (ANACTRL->XO32M_CTRL & ANACTRL_XO32M_CTRL_ENABLE_SYSTEM_CLK_OUT_MASK) ? s_Ext_Clk_Freq : 0U;
}

/* Get WATCH DOG Clk */
/*! brief  Return Frequency of Watchdog
 *  return Frequency of Watchdog
 */
uint32_t CLOCK_GetWdtClkFreq(void)
{
    return CLOCK_GetFro1MFreq() / ((SYSCON->WDTCLKDIV & SYSCON_WDTCLKDIV_DIV_MASK) + 1U);
}

/* Get HF FRO Clk */
/*! brief  Return Frequency of High-Freq output of FRO
 *  return Frequency of High-Freq output of FRO
 */
uint32_t CLOCK_GetFroHfFreq(void)
{
    return (PMC->PDRUNCFG0 & PMC_PDRUNCFG0_PDEN_FRO192M_MASK) ?
               0 :
               (ANACTRL->FRO192M_CTRL & ANACTRL_FRO192M_CTRL_ENA_96MHZCLK_MASK) ? 96000000U : 0U;
}

/* Get SYSTEM PLL Clk */
/*! brief  Return Frequency of PLL
 *  return Frequency of PLL
 */
uint32_t CLOCK_GetPll0OutFreq(void)
{
    return s_Pll0_Freq;
}

/* Get USB PLL Clk */
/*! brief  Return Frequency of USB PLL
 *  return Frequency of PLL
 */
uint32_t CLOCK_GetPll1OutFreq(void)
{
    return s_Pll1_Freq;
}

/* Get RTC OSC Clk */
/*! brief  Return Frequency of 32kHz osc
 *  return Frequency of 32kHz osc
 */
uint32_t CLOCK_GetOsc32KFreq(void)
{
    return ((~(PMC->PDRUNCFG0 & PMC_PDRUNCFG0_PDEN_FRO32K_MASK)) && (PMC->RTCOSC32K & PMC_RTCOSC32K_SEL(0))) ?
               CLK_RTC_32K_CLK :
               ((~(PMC->PDRUNCFG0 & PMC_PDRUNCFG0_PDEN_XTAL32K_MASK)) && (PMC->RTCOSC32K & PMC_RTCOSC32K_SEL(1))) ?
               CLK_RTC_32K_CLK :
               0U;
}

/* Get MAIN Clk */
/*! brief  Return Frequency of Core System
 *  return Frequency of Core System
 */
uint32_t CLOCK_GetCoreSysClkFreq(void)
{
    uint32_t freq = 0U;

    switch (SYSCON->MAINCLKSELB)
    {
        case 0U:
            if (SYSCON->MAINCLKSELA == 0U)
            {
                freq = CLOCK_GetFro12MFreq();
            }
            else if (SYSCON->MAINCLKSELA == 1U)
            {
                freq = CLOCK_GetExtClkFreq();
            }
            else if (SYSCON->MAINCLKSELA == 2U)
            {
                freq = CLOCK_GetFro1MFreq();
            }
            else if (SYSCON->MAINCLKSELA == 3U)
            {
                freq = CLOCK_GetFroHfFreq();
            }
            else
            {
            }
            break;
        case 1U:
            freq = CLOCK_GetPll0OutFreq();
            break;
        case 2U:
            freq = CLOCK_GetPll1OutFreq();
            break;

        case 3U:
            freq = CLOCK_GetOsc32KFreq();
            break;

        default:
            break;
    }

    return freq;
}

/* Get I2S MCLK Clk */
/*! brief  Return Frequency of I2S MCLK Clock
 *  return Frequency of I2S MCLK Clock
 */
uint32_t CLOCK_GetI2SMClkFreq(void)
{
    return s_I2S_Mclk_Freq;
}

/* Get FLEXCOMM input clock */
/*! brief  Return Frequency of flexcomm input clock
 *  param  id     : flexcomm instance id
 *  return Frequency value
 */
uint32_t CLOCK_GetFlexCommInputClock(uint32_t id)
{
    uint32_t freq = 0U;

    switch (SYSCON->FCCLKSELX[id])
    {
        case 0U:
            freq = CLOCK_GetCoreSysClkFreq();
            break;
        case 1U:
            freq = CLOCK_GetPll0OutFreq() / ((SYSCON->PLL0CLKDIV & 0xffU) + 1U);
            break;
        case 2U:
            freq = CLOCK_GetFro12MFreq();
            break;
        case 3U:
            freq = CLOCK_GetFroHfFreq() / ((SYSCON->FROHFDIV & 0xffU) + 1U);
            break;
        case 4U:
            freq = CLOCK_GetFro1MFreq();
            break;
        case 5U:
            freq = CLOCK_GetI2SMClkFreq();
            break;
        case 6U:
            freq = CLOCK_GetOsc32KFreq();
            break;
        case 7U:
            freq = 0U;
            break;

        default:
            break;
    }

    return freq;
}

/* Get FLEXCOMM Clk */
uint32_t CLOCK_GetFlexCommClkFreq(uint32_t id)
{
    uint32_t freq = 0U;

    freq = CLOCK_GetFlexCommInputClock(id);
    return freq / (1 +
                   (SYSCON->FLEXFRGXCTRL[id] & SYSCON_FLEXFRG0CTRL_MULT_MASK) /
                       ((SYSCON->FLEXFRGXCTRL[id] & SYSCON_FLEXFRG0CTRL_DIV_MASK) + 1U));
}

/* Get HS_LPSI Clk */
uint32_t CLOCK_GetHsLspiClkFreq(void)
{
    uint32_t freq = 0U;

    switch (SYSCON->HSLSPICLKSEL)
    {
        case 0U:
            freq = CLOCK_GetCoreSysClkFreq();
            break;
        case 1U:
            freq = CLOCK_GetPll0OutFreq() / ((SYSCON->PLL0CLKDIV & 0xffU) + 1U);
            break;
        case 2U:
            freq = CLOCK_GetFro12MFreq();
            break;
        case 3U:
            freq = CLOCK_GetFroHfFreq() / ((SYSCON->FROHFDIV & 0xffU) + 1U);
            break;
        case 4U:
            freq = CLOCK_GetFro1MFreq();
            break;
        case 6U:
            freq = CLOCK_GetOsc32KFreq();
            break;
        case 7U:
            freq = 0U;
            break;

        default:
            break;
    }

    return freq;
}

/* Get CTimer Clk */
/*! brief  Return Frequency of CTimer functional Clock
 *  return Frequency of CTimer functional Clock
 */
uint32_t CLOCK_GetCTimerClkFreq(uint32_t id)
{
    uint32_t freq = 0U;

    switch (SYSCON->CTIMERCLKSELX[id])
    {
        case 0U:
            freq = CLOCK_GetCoreSysClkFreq();
            break;
        case 1U:
            freq = CLOCK_GetPll0OutFreq();
            break;
        case 3U:
            freq = CLOCK_GetFroHfFreq();
            break;
        case 4U:
            freq = CLOCK_GetFro1MFreq();
            break;
        case 5U:
            freq = CLOCK_GetI2SMClkFreq();
            break;
        case 6U:
            freq = CLOCK_GetOsc32KFreq();
            break;
        case 7U:
            freq = 0U;
            break;

        default:
            break;
    }

    return freq;
}

/* Get Systick Clk */
/*! brief  Return Frequency of SystickClock
 *  return Frequency of Systick Clock
 */
uint32_t CLOCK_GetSystickClkFreq(uint32_t id)
{
    volatile uint32_t *pSystickClkDiv;
    pSystickClkDiv = &(SYSCON->SYSTICKCLKDIV0);
    uint32_t freq = 0U;

    switch (SYSCON->SYSTICKCLKSELX[id])
    {
        case 0U:
            freq = CLOCK_GetCoreSysClkFreq() / ((pSystickClkDiv[id] & 0xffU) + 1U);
            break;
        case 1U:
            freq = CLOCK_GetFro1MFreq();
            break;
        case 2U:
            freq = CLOCK_GetOsc32KFreq();
            break;
        case 7U:
            freq = 0U;
            break;

        default:
            break;
    }

    return freq;
}

/* Set FlexComm Clock */
/**
 * brief   Set the flexcomm output frequency.
 * param   id      : flexcomm instance id
 *          freq    : output frequency
 * return  0   : the frequency range is out of range.
 *          1   : switch successfully.
 */
uint32_t CLOCK_SetFlexCommClock(uint32_t id, uint32_t freq)
{
    uint32_t input = CLOCK_GetFlexCommClkFreq(id);
    uint32_t mul;

    if ((freq > 48000000) || (freq > input) || (input / freq >= 2))
    {
        /* FRG output frequency should be less than equal to 48MHz */
        return 0;
    }
    else
    {
        mul = ((uint64_t)(input - freq) * 256) / ((uint64_t)freq);
        SYSCON->FLEXFRGXCTRL[id] = (mul << 8U) | 0xFFU;
        return 1;
    }
}

/* Get IP Clk */
/*! brief  Return Frequency of selected clock
 *  return Frequency of selected clock
 */
uint32_t CLOCK_GetFreq(clock_name_t clockName)
{
    uint32_t freq;
    switch (clockName)
    {
        case kCLOCK_CoreSysClk:
            freq = CLOCK_GetCoreSysClkFreq();
            break;
        case kCLOCK_BusClk:
            freq = CLOCK_GetCoreSysClkFreq() / ((SYSCON->AHBCLKDIV & 0xffU) + 1U);
            break;
        case kCLOCK_ClockOut:
            freq = CLOCK_GetClockOutClkFreq();
            break;
        case kCLOCK_Adc:
            freq = CLOCK_GetAdcClkFreq();
            break;
        case kCLOCK_Usb0:
            freq = CLOCK_GetUsb0ClkFreq();
            break;
        case kCLOCK_Usb1:
            freq = CLOCK_GetUsb1ClkFreq();
            break;
        case kCLOCK_Pll1Out:
            freq = CLOCK_GetPll1OutFreq();
            break;
        case kCLOCK_Mclk:
            freq = CLOCK_GetMclkClkFreq();
            break;
        case kCLOCK_FroHf:
            freq = CLOCK_GetFroHfFreq();
            break;
        case kCLOCK_Fro12M:
            freq = CLOCK_GetFro12MFreq();
            break;
        case kCLOCK_ExtClk:
            freq = CLOCK_GetExtClkFreq();
            break;
        case kCLOCK_Pll0Out:
            freq = CLOCK_GetPll0OutFreq();
            break;
        case kCLOCK_WdtClk:
            freq = CLOCK_GetWdtClkFreq();
            break;
        case kCLOCK_Sct:
            freq = CLOCK_GetSctClkFreq();
            break;
        case kCLOCK_SDio:
            freq = CLOCK_GetSdioClkFreq();
            break;
        case kCLOCK_FlexI2S:
            freq = CLOCK_GetI2SMClkFreq();
            break;
        case kCLOCK_Flexcomm0:
            freq = CLOCK_GetFlexCommClkFreq(0U);
            break;
        case kCLOCK_Flexcomm1:
            freq = CLOCK_GetFlexCommClkFreq(1U);
            break;
        case kCLOCK_Flexcomm2:
            freq = CLOCK_GetFlexCommClkFreq(2U);
            break;
        case kCLOCK_Flexcomm3:
            freq = CLOCK_GetFlexCommClkFreq(3U);
            break;
        case kCLOCK_Flexcomm4:
            freq = CLOCK_GetFlexCommClkFreq(4U);
            break;
        case kCLOCK_Flexcomm5:
            freq = CLOCK_GetFlexCommClkFreq(5U);
            break;
        case kCLOCK_Flexcomm6:
            freq = CLOCK_GetFlexCommClkFreq(6U);
            break;
        case kCLOCK_Flexcomm7:
            freq = CLOCK_GetFlexCommClkFreq(7U);
            break;
        case kCLOCK_HsLspi:
            freq = CLOCK_GetHsLspiClkFreq();
            break;
        case kCLOCK_CTmier0:
            freq = CLOCK_GetCTimerClkFreq(0U);
            break;
        case kCLOCK_CTmier1:
            freq = CLOCK_GetCTimerClkFreq(1U);
            break;
        case kCLOCK_CTmier2:
            freq = CLOCK_GetCTimerClkFreq(2U);
            break;
        case kCLOCK_CTmier3:
            freq = CLOCK_GetCTimerClkFreq(3U);
            break;
        case kCLOCK_CTmier4:
            freq = CLOCK_GetCTimerClkFreq(4U);
            break;
        case kCLOCK_Systick0:
            freq = CLOCK_GetSystickClkFreq(0U);
            break;
        case kCLOCK_Systick1:
            freq = CLOCK_GetSystickClkFreq(1U);
            break;
        default:
            freq = 0U;
            break;
    }
    return freq;
}

/* Find SELP, SELI, and SELR values for raw M value, max M = MVALMAX */
static void pllFindSel(uint32_t M, uint32_t *pSelP, uint32_t *pSelI, uint32_t *pSelR)
{
    uint32_t seli, selp;
    /* bandwidth: compute selP from Multiplier */
    if (SYSCON->PLL0SSCG1 & SYSCON_PLL0SSCG1_MDIV_EXT_MASK)
    {
        selp = (M >> 2U) + 1U;
        if (selp >= 31U)
        {
            selp = 31U;
        }
        *pSelP = selp;

        if (M >= 32768)
        {
            seli = 1;
        }
        else if (M >= 16384)
        {
            seli = 2;
        }
        else if (M >= 4096)
        {
            seli = 4;
        }
        else if (M >= 1002)
        {
            seli = 8;
        }
        else if (M >= 120)
        {
            seli = 4 * ((1024/(M/2 + 9)) + 1);
        }
        else
        {
            seli = 4 * (M/8 + 1);
        } 

        if (seli >= 63)
        {
            seli = 63;
        }
        *pSelI = seli;

        *pSelR = 0U;
    }
    else
    {
        *pSelP = 3U;
        *pSelI = 4U;
        *pSelR = 4U;
    }
}

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
        mMult_int = ((SYSCON->PLL0SSCG1 & SYSCON_PLL0SSCG1_MD_MBS_MASK) << 7U) | ((SYSCON->PLL0SSCG0) >> PLL0_SSCG_MD_INT_P);
        mMult_fract = ((float)((SYSCON->PLL0SSCG0) & PLL0_SSCG_MD_FRACT_M)/(1 << PLL0_SSCG_MD_INT_P));
        mMult = (float)mMult_int + mMult_fract;
    }
    if (mMult == 0)
    {
        mMult = 1;
    }
    return mMult;
}


/* Find greatest common divisor between m and n */
static uint32_t FindGreatestCommonDivisor(uint32_t m, uint32_t n)
{
    uint32_t tmp;

    while (n != 0U)
    {
        tmp = n;
        n = m % n;
        m = tmp;
    }

    return m;
}

/*
 * Set PLL0 output based on desired output rate.
 * In this function, the it calculates the PLL0 setting for output frequency from input clock
 * frequency. The calculation would cost a few time. So it is not recommaned to use it frequently.
 * the "pllctrl", "pllndec", "pllpdec", "pllmdec" would updated in this function.
 */
static pll_error_t CLOCK_GetPll0ConfigInternal(
    uint32_t finHz, uint32_t foutHz, pll_setup_t *pSetup, bool useSS)
{
    uint32_t nDivOutHz, fccoHz;
    uint32_t pllPreDivider, pllMultiplier, pllPostDivider;
    uint32_t pllDirectInput, pllDirectOutput;
    uint32_t pllSelP, pllSelI, pllSelR, uplimoff;

    /* Baseline parameters (no input or output dividers) */
    pllPreDivider = 1U;  /* 1 implies pre-divider will be disabled */
    pllPostDivider = 1U; /* 1 implies post-divider will be disabled */
    pllDirectOutput = 1U;

    /* Verify output rate parameter */
    if (foutHz > PLL_MAX_CCO_FREQ_MHZ)
    {
        /* Maximum PLL output with post divider=1 cannot go above this frequency */
        return kStatus_PLL_OutputTooHigh;
    }
    if (foutHz < (PLL_MIN_CCO_FREQ_MHZ / (PVALMAX << 1U)))
    {
        /* Minmum PLL output with maximum post divider cannot go below this frequency */
        return kStatus_PLL_OutputTooLow;
    }

    /* If using SS mode, input clock needs to be between 3MHz and 20MHz */
    if (useSS)
    {
        /* Verify input rate parameter */
        if (finHz < PLL_MIN_IN_SSMODE)
        {
            /* Input clock into the PLL cannot be lower than this */
            return kStatus_PLL_InputTooLow;
        }
        /* PLL input in SS mode must be under 20MHz */
        if (finHz > (PLL_MAX_IN_SSMODE * NVALMAX))
        {
            return kStatus_PLL_InputTooHigh;
        }
    }
    else
    {
        /* Verify input rate parameter */
        if (finHz < PLL_LOWER_IN_LIMIT)
        {
            /* Input clock into the PLL cannot be lower than this */
            return kStatus_PLL_InputTooLow;
        }
        if (finHz > PLL_HIGHER_IN_LIMIT)
        {
            /* Input clock into the PLL cannot be higher than this */
            return kStatus_PLL_InputTooHigh;
        }
    }

    /* Find the optimal CCO frequency for the output and input that
       will keep it inside the PLL CCO range. This may require
       tweaking the post-divider for the PLL. */
    fccoHz = foutHz;
    while (fccoHz < PLL_MIN_CCO_FREQ_MHZ)
    {
        /* CCO output is less than minimum CCO range, so the CCO output
           needs to be bumped up and the post-divider is used to bring
           the PLL output back down. */
        pllPostDivider++;
        if (pllPostDivider > PVALMAX)
        {
            return kStatus_PLL_OutsideIntLimit;
        }

        /* Target CCO goes up, PLL output goes down */
        /* divide-by-2 divider in the post-divider is always work*/
        fccoHz = foutHz * (pllPostDivider * 2U);
        pllDirectOutput = 0U;
    }

    /* Determine if a pre-divider is needed to get the best frequency */
    if ((finHz > PLL_LOWER_IN_LIMIT) && (fccoHz >= finHz) && (useSS == false))
    {
        uint32_t a = FindGreatestCommonDivisor(fccoHz, finHz);

        if (a > PLL_LOWER_IN_LIMIT)
        {
            a = finHz / a;
            if ((a != 0U) && (a < PLL_MAX_N_DIV))
            {
                pllPreDivider = a;
            }
        }
    }

    /* Bypass pre-divider hardware if pre-divider is 1 */
    if (pllPreDivider > 1U)
    {
        pllDirectInput = 0U;
    }
    else
    {
        pllDirectInput = 1U;
    }

    /* Determine PLL multipler */
    nDivOutHz = (finHz / pllPreDivider);
    pllMultiplier = (fccoHz / nDivOutHz);

    /* Find optimal values for filter */
    if (useSS == false)
    {
        /* Will bumping up M by 1 get us closer to the desired CCO frequency? */
        if ((nDivOutHz * ((pllMultiplier * 2U) + 1U)) < (fccoHz * 2U))
        {
            pllMultiplier++;
        }

        /* Setup filtering */
        pllFindSel(pllMultiplier, &pllSelP, &pllSelI, &pllSelR);
        uplimoff = 0U;

        /* Get encoded value for M (mult) and use manual filter, disable SS mode */
        pSetup->pllsscg[1] = (PLL_SSCG1_MDEC_VAL_SET(pllMultiplier)) | (1U << SYSCON_PLL0SSCG1_SEL_EXT_SHIFT);
    }
    else
    {
        uint64_t fc;

        /* Filtering will be handled by SSC */
        pllSelR = pllSelI = pllSelP = 0U;
        uplimoff = 1U;

        /* The PLL multiplier will get very close and slightly under the
           desired target frequency. A small fractional component can be
           added to fine tune the frequency upwards to the target. */
        fc = ((uint64_t)(fccoHz % nDivOutHz) << 25U) / nDivOutHz;

        /* Set multiplier */
        pSetup->pllsscg[0] = (uint32_t)(PLL0_SSCG_MD_INT_SET(pllMultiplier) | PLL0_SSCG_MD_FRACT_SET((uint32_t)fc));
        pSetup->pllsscg[1] = PLL0_SSCG_MD_INT_SET(pllMultiplier) >> 32U;
    }

    /* Get encoded values for N (prediv) and P (postdiv) */
    pSetup->pllndec = PLL_NDEC_VAL_SET(pllPreDivider);
    pSetup->pllpdec = PLL_PDEC_VAL_SET(pllPostDivider);

    /* PLL control */
    pSetup->pllctrl = (pllSelR << SYSCON_PLL0CTRL_SELR_SHIFT) |                   /* Filter coefficient */
                      (pllSelI << SYSCON_PLL0CTRL_SELI_SHIFT) |                   /* Filter coefficient */
                      (pllSelP << SYSCON_PLL0CTRL_SELP_SHIFT) |                   /* Filter coefficient */
                      (0 << SYSCON_PLL0CTRL_BYPASSPLL_SHIFT) |                    /* PLL bypass mode disabled */
                      (uplimoff << SYSCON_PLL0CTRL_LIMUPOFF_SHIFT) |              /* SS/fractional mode disabled */
                      (pllDirectInput << SYSCON_PLL0CTRL_BYPASSPREDIV_SHIFT) |    /* Bypass pre-divider? */
                      (pllDirectOutput << SYSCON_PLL0CTRL_BYPASSPOSTDIV_SHIFT) |  /* Bypass post-divider? */
                      (1 << SYSCON_PLL0CTRL_CLKEN_SHIFT);                         /* Ensure the PLL clock output */

    return kStatus_PLL_Success;
}

#if (defined(CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT) && CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT)
/* Alloct the static buffer for cache. */
static pll_setup_t s_PllSetupCacheStruct[CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT];
static uint32_t s_FinHzCache[CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT] = {0};
static uint32_t s_FoutHzCache[CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT] = {0};
static bool s_UseSSCache[CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT] = {false};
static uint32_t s_PllSetupCacheIdx = 0U;
#endif /* CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT */

/*
 * Calculate the PLL setting values from input clock freq to output freq.
 */
static pll_error_t CLOCK_GetPll0Config(
    uint32_t finHz, uint32_t foutHz, pll_setup_t *pSetup, bool useSS)
{
    pll_error_t retErr;
#if (defined(CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT) && CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT)
    uint32_t i;

    for (i = 0U; i < CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT; i++)
    {
        if ((finHz == s_FinHzCache[i]) && (foutHz == s_FoutHzCache[i]) && (useSS == s_UseSSCache[i]))
        {
            /* Hit the target in cache buffer. */
            pSetup->pllctrl = s_PllSetupCacheStruct[i].pllctrl;
            pSetup->pllndec = s_PllSetupCacheStruct[i].pllndec;
            pSetup->pllpdec = s_PllSetupCacheStruct[i].pllpdec;
            pSetup->pllsscg[0] = s_PllSetupCacheStruct[i].pllsscg[0];
            pSetup->pllsscg[1] = s_PllSetupCacheStruct[i].pllsscg[1];
            retErr = kStatus_PLL_Success;
            break;
        }
    }

    if (i < CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT)
    {
        return retErr;
    }
#endif /* CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT */

    retErr = CLOCK_GetPll0ConfigInternal(finHz, foutHz, pSetup, useSS);

#if (defined(CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT) && CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT)
    /* Cache the most recent calulation result into buffer. */
    s_FinHzCache[s_PllSetupCacheIdx] = finHz;
    s_FoutHzCache[s_PllSetupCacheIdx] = foutHz;
    s_UseSSCache[s_PllSetupCacheIdx] = useSS;

    s_PllSetupCacheStruct[s_PllSetupCacheIdx].pllctrl = pSetup->pllctrl;
    s_PllSetupCacheStruct[s_PllSetupCacheIdx].pllndec = pSetup->pllndec;
    s_PllSetupCacheStruct[s_PllSetupCacheIdx].pllpdec = pSetup->pllpdec;
    s_PllSetupCacheStruct[s_PllSetupCacheIdx].pllsscg[0] = pSetup->pllsscg[0];
    s_PllSetupCacheStruct[s_PllSetupCacheIdx].pllsscg[1] = pSetup->pllsscg[1];
    /* Update the index for next available buffer. */
    s_PllSetupCacheIdx = (s_PllSetupCacheIdx + 1U) % CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT;
#endif /* CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT */

    return retErr;
}

/* Update local PLL rate variable */
static void CLOCK_GetPLL0OutFromSetupUpdate(pll_setup_t *pSetup)
{
    s_Pll0_Freq = CLOCK_GetPLL0OutFromSetup(pSetup);
}

/* Return System PLL input clock rate */
/*! brief    Return  PLL0 input clock rate
*  return    PLL0 input clock rate
*/
uint32_t CLOCK_GetPLL0InClockRate(void)
{
    uint32_t clkRate = 0U;

    switch ((SYSCON->PLL0CLKSEL & SYSCON_PLL0CLKSEL_SEL_MASK))
    {
        case 0x00U:
            clkRate = CLK_FRO_12MHZ;
            break;

        case 0x01U:
            clkRate = CLOCK_GetExtClkFreq();
            break;

        case 0x02U:
            clkRate = CLOCK_GetFro1MFreq();
            break;

        case 0x03U:
            clkRate = CLOCK_GetOsc32KFreq();
            break;

        default:
            clkRate = 0U;
            break;
    }

    return clkRate;
}

/* Return PLL1 input clock rate */
uint32_t CLOCK_GetPLL1InClockRate(void)
{
    uint32_t clkRate = 0U;

    switch ((SYSCON->PLL1CLKSEL & SYSCON_PLL1CLKSEL_SEL_MASK))
    {
        case 0x00U:
            clkRate = CLK_FRO_12MHZ;
            break;

        case 0x01U:
            clkRate = CLOCK_GetExtClkFreq();
            break;

        case 0x02U:
            clkRate = CLOCK_GetFro1MFreq();
            break;

        case 0x03U:
            clkRate = CLOCK_GetOsc32KFreq();
            break;

        default:
            clkRate = 0U;
            break;
    }

    return clkRate;
}

/* Return PLL0 output clock rate from setup structure */
/*! brief    Return PLL0 output clock rate from setup structure
*  param    pSetup : Pointer to a PLL setup structure
*  return   PLL0 output clock rate the setup structure will generate
*/
uint32_t CLOCK_GetPLL0OutFromSetup(pll_setup_t *pSetup)
{
    uint32_t clkRate = 0;
    uint32_t prediv, postdiv;
    float workRate = 0;

    /* Get the input clock frequency of PLL. */
    clkRate = CLOCK_GetPLL0InClockRate();

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

    return (uint32_t)workRate;
}

/* Set the current PLL0 Rate */
/*! brief Store the current PLL rate
*  param    rate: Current rate of the PLL
*  return   Nothing
**/
void CLOCK_SetStoredPLL0ClockRate(uint32_t rate)
{
    s_Pll0_Freq = rate;
}

/* Return PLL0 output clock rate */
/*! brief    Return  PLL0 output clock rate
*  param    recompute   : Forces a PLL rate recomputation if true
*  return    PLL0 output clock rate
*  note The PLL rate is cached in the driver in a variable as
*  the rate computation function can take some time to perform. It
*  is recommended to use 'false' with the 'recompute' parameter.
*/
uint32_t CLOCK_GetPLL0OutClockRate(bool recompute)
{
    pll_setup_t Setup;
    uint32_t rate;

    if ((recompute) || (s_Pll0_Freq == 0U))
    {
        Setup.pllctrl = SYSCON->PLL0CTRL;
        Setup.pllndec = SYSCON->PLL0NDEC;
        Setup.pllpdec = SYSCON->PLL0PDEC;
        Setup.pllsscg[0] = SYSCON->PLL0SSCG0;
        Setup.pllsscg[1] = SYSCON->PLL0SSCG1;

        CLOCK_GetPLL0OutFromSetupUpdate(&Setup);
    }

    rate = s_Pll0_Freq;

    return rate;
}

/* Set PLL0 output based on the passed PLL setup data */
/*! brief    Set PLL output based on the passed PLL setup data
*  param    pControl    : Pointer to populated PLL control structure to generate setup with
*  param    pSetup      : Pointer to PLL setup structure to be filled
*  return   PLL_ERROR_SUCCESS on success, or PLL setup error code
*  note Actual frequency for setup may vary from the desired frequency based on the
*  accuracy of input clocks, rounding, non-fractional PLL mode, etc.
*/
pll_error_t CLOCK_SetupPLL0Data(pll_config_t *pControl, pll_setup_t *pSetup)
{
    uint32_t inRate;
    bool useSS = (bool)((pControl->flags & PLL_CONFIGFLAG_FORCENOFRACT) == 0U);

    pll_error_t pllError;

    /* Determine input rate for the PLL */
    if ((pControl->flags & PLL_CONFIGFLAG_USEINRATE) != 0U)
    {
        inRate = pControl->inputRate;
    }
    else
    {
        inRate = CLOCK_GetPLL0InClockRate();
    }

    /* PLL flag options */
    pllError = CLOCK_GetPll0Config(inRate, pControl->desiredRate, pSetup, useSS);
    if ((useSS) && (pllError == kStatus_PLL_Success))
    {
        /* If using SS mode, then some tweaks are made to the generated setup */
        pSetup->pllsscg[1] |= (uint32_t)pControl->ss_mf | (uint32_t)pControl->ss_mr | (uint32_t)pControl->ss_mc;
        if (pControl->mfDither)
        {
            pSetup->pllsscg[1] |= (1U << SYSCON_PLL0SSCG1_DITHER_SHIFT);
        }
    }

    return pllError;
}

/* Set PLL0 output from PLL setup structure */
/*! brief    Set PLL output from PLL setup structure (precise frequency)
* param pSetup  : Pointer to populated PLL setup structure
* param flagcfg : Flag configuration for PLL config structure
* return    PLL_ERROR_SUCCESS on success, or PLL setup error code
* note  This function will power off the PLL, setup the PLL with the
* new setup data, and then optionally powerup the PLL, wait for PLL lock,
* and adjust system voltages to the new PLL rate. The function will not
* alter any source clocks (ie, main systen clock) that may use the PLL,
* so these should be setup prior to and after exiting the function.
*/
pll_error_t CLOCK_SetupPLL0Prec(pll_setup_t *pSetup, uint32_t flagcfg)
{   
    uint32_t inRate, clkRate, prediv;

    /* Power off PLL during setup changes */
    POWER_EnablePD(kPDRUNCFG_PD_PLL0);
    POWER_EnablePD(kPDRUNCFG_PD_PLL0_SSCG);

    pSetup->flags = flagcfg;

    /* Write PLL setup data */
    SYSCON->PLL0CTRL = pSetup->pllctrl;
    SYSCON->PLL0NDEC = pSetup->pllndec;
    SYSCON->PLL0NDEC = pSetup->pllndec | (1U << SYSCON_PLL0NDEC_NREQ_SHIFT); /* latch */
    SYSCON->PLL0PDEC = pSetup->pllpdec;
    SYSCON->PLL0PDEC = pSetup->pllpdec | (1U << SYSCON_PLL0PDEC_PREQ_SHIFT); /* latch */
    SYSCON->PLL0SSCG0 = pSetup->pllsscg[0];
    SYSCON->PLL0SSCG1 = pSetup->pllsscg[1];
    SYSCON->PLL0SSCG1 =
        pSetup->pllsscg[1] | (1U << SYSCON_PLL0SSCG1_MREQ_SHIFT) | (1U << SYSCON_PLL0SSCG1_MD_REQ_SHIFT); /* latch */

    POWER_DisablePD(kPDRUNCFG_PD_PLL0);
    POWER_DisablePD(kPDRUNCFG_PD_PLL0_SSCG);

    if ((pSetup->flags & PLL_SETUPFLAG_WAITLOCK) != 0U)
    {
        inRate = CLOCK_GetPLL0InClockRate();
        prediv = findPll0PreDiv();
        /* Adjust input clock */
        clkRate = inRate / prediv;
        /* The lock signal is only reliable between fref[2] :100 kHz to 20 MHz. */
        if ((clkRate >= 100000) && (clkRate <= 20000000))
        {
            while (CLOCK_IsPLL0Locked() == false)
            {
            }
        }
    }

    /* Update current programmed PLL rate var */
    CLOCK_GetPLL0OutFromSetupUpdate(pSetup);

    /* System voltage adjustment, occurs prior to setting main system clock */
    if ((pSetup->flags & PLL_SETUPFLAG_ADGVOLT) != 0U)
    {
        POWER_SetVoltageForFreq(s_Pll0_Freq);
    }

    return kStatus_PLL_Success;
}

/* Setup PLL Frequency from pre-calculated value */
/**
* brief Set PLL0 output from PLL setup structure (precise frequency)
* param pSetup  : Pointer to populated PLL setup structure
* return    kStatus_PLL_Success on success, or PLL setup error code
* note  This function will power off the PLL, setup the PLL with the
* new setup data, and then optionally powerup the PLL, wait for PLL lock,
* and adjust system voltages to the new PLL rate. The function will not
* alter any source clocks (ie, main systen clock) that may use the PLL,
* so these should be setup prior to and after exiting the function.
*/
pll_error_t CLOCK_SetPLL0Freq(const pll_setup_t *pSetup)
{
    uint32_t inRate, clkRate, prediv;
    /* Power off PLL during setup changes */
    POWER_EnablePD(kPDRUNCFG_PD_PLL0);
    POWER_EnablePD(kPDRUNCFG_PD_PLL0_SSCG);

    /* Write PLL setup data */
    SYSCON->PLL0CTRL = pSetup->pllctrl;
    SYSCON->PLL0NDEC = pSetup->pllndec;
    SYSCON->PLL0NDEC = pSetup->pllndec | (1U << SYSCON_PLL0NDEC_NREQ_SHIFT); /* latch */
    SYSCON->PLL0PDEC = pSetup->pllpdec;
    SYSCON->PLL0PDEC = pSetup->pllpdec | (1U << SYSCON_PLL0PDEC_PREQ_SHIFT); /* latch */
    SYSCON->PLL0SSCG0 = pSetup->pllsscg[0];
    SYSCON->PLL0SSCG1 = pSetup->pllsscg[1];
    SYSCON->PLL0SSCG1 =
        pSetup->pllsscg[1] | (1U << SYSCON_PLL0SSCG1_MD_REQ_SHIFT) | (1U << SYSCON_PLL0SSCG1_MREQ_SHIFT); /* latch */

    POWER_DisablePD(kPDRUNCFG_PD_PLL0);
    POWER_DisablePD(kPDRUNCFG_PD_PLL0_SSCG);

    if ((pSetup->flags & PLL_SETUPFLAG_WAITLOCK) != 0U)
    {
        inRate = CLOCK_GetPLL0InClockRate();
        prediv = findPll0PreDiv();
        /* Adjust input clock */
        clkRate = inRate / prediv;
        /* The lock signal is only reliable between fref[2] :100 kHz to 20 MHz. */
        if ((clkRate >= 100000) && (clkRate <= 20000000))
        {
            while (CLOCK_IsPLL0Locked() == false)
            {
            }
        }
    }

    /* Update current programmed PLL rate var */
    s_Pll0_Freq = pSetup->pllRate;

    return kStatus_PLL_Success;
}

/* Setup PLL1 Frequency from pre-calculated value */
/**
* brief Set PLL1 output from PLL setup structure (precise frequency)
* param pSetup  : Pointer to populated PLL setup structure
* return    kStatus_PLL_Success on success, or PLL setup error code
* note  This function will power off the PLL, setup the PLL with the
* new setup data, and then optionally powerup the PLL, wait for PLL lock,
* and adjust system voltages to the new PLL rate. The function will not
* alter any source clocks (ie, main systen clock) that may use the PLL,
* so these should be setup prior to and after exiting the function.
*/
pll_error_t CLOCK_SetPLL1Freq(const pll_setup_t *pSetup)
{
    uint32_t inRate, clkRate, prediv;
    /* Power off PLL during setup changes */
    POWER_EnablePD(kPDRUNCFG_PD_PLL1);

    /* Write PLL setup data */
    SYSCON->PLL1CTRL = pSetup->pllctrl;
    SYSCON->PLL1NDEC = pSetup->pllndec;
    SYSCON->PLL1NDEC = pSetup->pllndec | (1U << SYSCON_PLL1NDEC_NREQ_SHIFT); /* latch */
    SYSCON->PLL1PDEC = pSetup->pllpdec;
    SYSCON->PLL1PDEC = pSetup->pllpdec | (1U << SYSCON_PLL1PDEC_PREQ_SHIFT); /* latch */
    SYSCON->PLL1MDEC = pSetup->pllmdec;
    SYSCON->PLL1MDEC = pSetup->pllmdec | (1U << SYSCON_PLL1MDEC_MREQ_SHIFT); /* latch */

    POWER_DisablePD(kPDRUNCFG_PD_PLL1);

    if ((pSetup->flags & PLL_SETUPFLAG_WAITLOCK) != 0U)
    {
        inRate = CLOCK_GetPLL1InClockRate();
        prediv = findPll1PreDiv();
        /* Adjust input clock */
        clkRate = inRate / prediv;
        /* The lock signal is only reliable between fref[2] :100 kHz to 20 MHz. */
        if ((clkRate >= 100000) && (clkRate <= 20000000))
        {
            while (CLOCK_IsPLL1Locked() == false)
            {
            }
        }
    }

    /* Update current programmed PLL rate var */
    s_Pll0_Freq = pSetup->pllRate;

    return kStatus_PLL_Success;
}

/* Set PLL0 clock based on the input frequency and multiplier */
/*! brief    Set PLL0 output based on the multiplier and input frequency
* param multiply_by : multiplier
* param input_freq  : Clock input frequency of the PLL
* return    Nothing
* note  Unlike the Chip_Clock_SetupSystemPLLPrec() function, this
* function does not disable or enable PLL power, wait for PLL lock,
* or adjust system voltages. These must be done in the application.
* The function will not alter any source clocks (ie, main systen clock)
* that may use the PLL, so these should be setup prior to and after
* exiting the function.
*/
void CLOCK_SetupPLL0Mult(uint32_t multiply_by, uint32_t input_freq)
{
    uint32_t cco_freq = input_freq * multiply_by;
    uint32_t pdec = 1U;
    uint32_t selr;
    uint32_t seli;
    uint32_t selp;
    uint32_t mdec, ndec;

    while (cco_freq < 275000000U)
    {
        multiply_by <<= 1U; /* double value in each iteration */
        pdec <<= 1U;        /* correspondingly double pdec to cancel effect of double msel */
        cco_freq = input_freq * multiply_by;
    }

    selr = 0U;

    if (multiply_by >= 32768)
    {
        seli = 1;
    }
    else if (multiply_by >= 16384)
    {
        seli = 2;
    }
    else if (multiply_by >= 4096)
    {
        seli = 4;
    }
    else if (multiply_by >= 1002)
    {
        seli = 8;
    }
    else if (multiply_by >= 120)
    {
        seli = 4 * ((1024/(multiply_by/2 + 9)) + 1);
    }
    else
    {
        seli = 4 * (multiply_by/8 + 1);
    } 

    if (seli >= 63U)
    {
        seli = 63U;
    }
    selp = (multiply_by >> 2U) + 1U;
    {
        selp = 31U;
    }

    if (pdec > 1U)
    {
        pdec = pdec / 2U; /* Account for minus 1 encoding */
                          /* Translate P value */
    }

    mdec = PLL_SSCG1_MDEC_VAL_SET(multiply_by);
    ndec = 0x1U; /* pre divide by 1 (hardcoded) */

    SYSCON->PLL0CTRL = SYSCON_PLL0CTRL_CLKEN_MASK |SYSCON_PLL0CTRL_BYPASSPOSTDIV(0) | SYSCON_PLL0CTRL_BYPASSPOSTDIV2(0) |
                       (selr << SYSCON_PLL0CTRL_SELR_SHIFT) | (seli << SYSCON_PLL0CTRL_SELI_SHIFT) |
                       (selp << SYSCON_PLL0CTRL_SELP_SHIFT);
    SYSCON->PLL0PDEC = pdec | (1U << SYSCON_PLL0PDEC_PREQ_SHIFT);   /* set Pdec value and assert preq */
    SYSCON->PLL0NDEC = ndec | (1U << SYSCON_PLL0NDEC_NREQ_SHIFT);   /* set Pdec value and assert preq */
    SYSCON->PLL0SSCG1 = mdec | (1U << SYSCON_PLL0SSCG1_MREQ_SHIFT); /* select non sscg MDEC value, assert mreq and select mdec value */
}

/* Enable USB DEVICE FULL SPEED clock */
/*! brief Enable USB Device FS clock.
* param src : clock source
* param freq: clock frequency
* Enable USB Device Full Speed clock.
*/
bool CLOCK_EnableUsbfs0DeviceClock(clock_usbfs_src_t src, uint32_t freq)
{
    bool ret = true;

    CLOCK_DisableClock(kCLOCK_Usbd0);

    if (kCLOCK_UsbfsSrcFro == src)
    {
        switch (freq)
        {
            case 96000000U:
                CLOCK_SetClkDiv(kCLOCK_DivUsb0Clk, 2, false); /*!< Div by 2 to get 48MHz, no divider reset */
                break;

            default:
                ret = false;
                break;
        }
        /* Turn ON FRO HF */
        POWER_DisablePD(kPDRUNCFG_PD_FRO192M);
        /* Enable FRO 96MHz output */
        ANACTRL->FRO192M_CTRL = ANACTRL->FRO192M_CTRL | ANACTRL_FRO192M_CTRL_ENA_96MHZCLK_MASK;
        /* Select FRO 96 or 48 MHz */
        CLOCK_AttachClk(kFRO_HF_to_USB0_CLK);
    }
    else
    {
        /*Set the USB PLL as the Usb0 CLK*/
        POWER_DisablePD(kPDRUNCFG_PD_PLL1);
        POWER_DisablePD(kPDRUNCFG_PD_XTAL32M);                        /*!< Ensure XTAL32K is on  */
        POWER_DisablePD(kPDRUNCFG_PD_LDOXO32M);                       /*!< Ensure XTAL32K is on  */
        SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_CLKIN_ENA_MASK; /*!< Ensure CLK_IN is on  */
        ANACTRL->XO32M_CTRL |= ANACTRL_XO32M_CTRL_ENABLE_SYSTEM_CLK_OUT_MASK;

        CLOCK_AttachClk(kEXT_CLK_to_PLL1); /*!< Switch PLL0 clock source selector to XTAL16M */

        const pll_setup_t pll1Setup = {
            .pllctrl = SYSCON_PLL1CTRL_CLKEN_MASK | SYSCON_PLL1CTRL_SELI(16U) | SYSCON_PLL1CTRL_SELP(7U),
            .pllndec = SYSCON_PLL1NDEC_NDIV(1U),
            .pllpdec = SYSCON_PLL1PDEC_PDIV(4U),
            .pllmdec = SYSCON_PLL1MDEC_MDIV(24U),
            .pllRate = 48000000U,
            .flags = PLL_SETUPFLAG_WAITLOCK,
        };

        CLOCK_SetPLL1Freq(&pll1Setup); /*!< Configure PLL1 to the desired values */

        CLOCK_SetClkDiv(kCLOCK_DivUsb0Clk, 1U, false);
        CLOCK_AttachClk(kPLL1_to_USB0_CLK);
        uint32_t delay = 100000;
        while (delay--)
        {
            __asm("nop");
        }
    }
    CLOCK_EnableClock(kCLOCK_Usbd0);
    CLOCK_EnableClock(kCLOCK_UsbRam1);

    return ret;
}

/* Enable USB HOST FULL SPEED clock */
/*! brief Enable USB HOST FS clock.
* param src : clock source
* param freq: clock frequency
* Enable USB HOST Full Speed clock.
*/
bool CLOCK_EnableUsbfs0HostClock(clock_usbfs_src_t src, uint32_t freq)
{
    bool ret = true;

    CLOCK_DisableClock(kCLOCK_Usbhmr0);
    CLOCK_DisableClock(kCLOCK_Usbhsl0);

    if (kCLOCK_UsbfsSrcFro == src)
    {
        switch (freq)
        {
            case 96000000U:
                CLOCK_SetClkDiv(kCLOCK_DivUsb0Clk, 2, false); /*!< Div by 2 to get 48MHz, no divider reset */
                break;

            default:
                ret = false;
                break;
        }
        /* Turn ON FRO HF */
        POWER_DisablePD(kPDRUNCFG_PD_FRO192M);
        /* Enable FRO 96MHz output */
        ANACTRL->FRO192M_CTRL = ANACTRL->FRO192M_CTRL | ANACTRL_FRO192M_CTRL_ENA_96MHZCLK_MASK;
        /* Select FRO 96 MHz */
        CLOCK_AttachClk(kFRO_HF_to_USB0_CLK);
    }
    else
    {
        /*Set the USB PLL as the Usb0 CLK*/
        POWER_DisablePD(kPDRUNCFG_PD_PLL1);
        POWER_DisablePD(kPDRUNCFG_PD_XTAL32M);                        /*!< Ensure XTAL32K is on  */
        POWER_DisablePD(kPDRUNCFG_PD_LDOXO32M);                       /*!< Ensure XTAL32K is on  */
        SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_CLKIN_ENA_MASK; /*!< Ensure CLK_IN is on  */
        ANACTRL->XO32M_CTRL |= ANACTRL_XO32M_CTRL_ENABLE_SYSTEM_CLK_OUT_MASK;

        CLOCK_AttachClk(kEXT_CLK_to_PLL1); /*!< Switch PLL0 clock source selector to XTAL16M */

        const pll_setup_t pll1Setup = {
            .pllctrl = SYSCON_PLL1CTRL_CLKEN_MASK | SYSCON_PLL1CTRL_SELI(16U) | SYSCON_PLL1CTRL_SELP(7U),
            .pllndec = SYSCON_PLL1NDEC_NDIV(1U),
            .pllpdec = SYSCON_PLL1PDEC_PDIV(4U),
            .pllmdec = SYSCON_PLL1MDEC_MDIV(24U),
            .pllRate = 48000000U,
            .flags = PLL_SETUPFLAG_WAITLOCK,
        };

        CLOCK_SetPLL1Freq(&pll1Setup); /*!< Configure PLL1 to the desired values */

        CLOCK_SetClkDiv(kCLOCK_DivUsb0Clk, 1U, false);
        CLOCK_AttachClk(kPLL1_to_USB0_CLK);
        uint32_t delay = 100000;
        while (delay--)
        {
            __asm("nop");
        }
    }
    CLOCK_EnableClock(kCLOCK_Usbhmr0);
    CLOCK_EnableClock(kCLOCK_Usbhsl0);
    CLOCK_EnableClock(kCLOCK_UsbRam1);

    return ret;
}

/* Enable USB PHY clock */
bool CLOCK_EnableUsbhs0PhyPllClock(clock_usb_phy_src_t src, uint32_t freq)
{
    volatile uint32_t i;

    POWER_DisablePD(kPDRUNCFG_PD_XTAL32M);
    POWER_DisablePD(kPDRUNCFG_PD_LDOXO32M);
    POWER_DisablePD(kPDRUNCFG_PD_FRO32K);   /*!< Ensure FRO32k is on  */
    POWER_DisablePD(kPDRUNCFG_PD_XTAL32K);  /*!< Ensure xtal32k is on  */
    POWER_DisablePD(kPDRUNCFG_PD_USB1_PHY); /*!< Ensure xtal32k is on  */
    POWER_DisablePD(kPDRUNCFG_PD_LDOUSBHS); /*!< Ensure xtal32k is on  */

    /* wait to make sure PHY power is fully up */
    i = 100000;
    while (i--)
    {
        __asm("nop");
    }

    SYSCON->AHBCLKCTRLSET[2] = SYSCON_AHBCLKCTRL2_ANALOG_CTRL(1);
    SYSCON->AHBCLKCTRLSET[2] = SYSCON_AHBCLKCTRL2_USB1_PHY(1);

    USBPHY->CTRL_CLR = USBPHY_CTRL_SFTRST_MASK;
    USBPHY->PLL_SIC = (USBPHY->PLL_SIC & ~USBPHY_PLL_SIC_PLL_DIV_SEL(0x7)) | USBPHY_PLL_SIC_PLL_DIV_SEL(0x06);
    USBPHY->PLL_SIC_SET = USBPHY_PLL_SIC_SET_PLL_REG_ENABLE_MASK;
    USBPHY->PLL_SIC_CLR = USBPHY_PLL_SIC_SET_PLL_BYPASS_MASK;
    USBPHY->PLL_SIC_SET = USBPHY_PLL_SIC_SET_PLL_POWER_MASK;
    USBPHY->PLL_SIC_SET = USBPHY_PLL_SIC_SET_PLL_EN_USB_CLKS_MASK;
    USBPHY->PLL_SIC_SET =
        USBPHY_PLL_SIC_SET_MISC2_CONTROL0_MASK; /* enables auto power down of PHY PLL during suspend */

    USBPHY->CTRL_CLR = USBPHY_CTRL_CLR_CLKGATE_MASK;
    USBPHY->PWD_SET = 0x0;

    return true;
}

/* Enable USB DEVICE HIGH SPEED clock */
bool CLOCK_EnableUsbhs0DeviceClock(clock_usbhs_src_t src, uint32_t freq)
{
    SYSCON->AHBCLKCTRLSET[2] = SYSCON_AHBCLKCTRL2_USB1_RAM(1);
    SYSCON->AHBCLKCTRLSET[2] = SYSCON_AHBCLKCTRL2_USB1_DEV(1);

    /* 16 MHz will be driven by the tb on the xtal1 pin of XTAL32M */
    SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_CLKIN_ENA_MASK; /* Enable clock_in clock for clock module. */
    ANACTRL->XO32M_CTRL |= ANACTRL_XO32M_CTRL_ENABLE_PLL_USB_OUT(1);
    return true;
}

/* Enable USB HOST HIGH SPEED clock */
bool CLOCK_EnableUsbhs0HostClock(clock_usbhs_src_t src, uint32_t freq)
{
    SYSCON->AHBCLKCTRLSET[2] = SYSCON_AHBCLKCTRL2_USB1_RAM(1);
    SYSCON->AHBCLKCTRLSET[2] = SYSCON_AHBCLKCTRL2_USB1_HOST(1);

    /* 16 MHz will be driven by the tb on the xtal1 pin of XTAL32M */
    SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_CLKIN_ENA_MASK; /* Enable clock_in clock for clock module. */
    ANACTRL->XO32M_CTRL |= ANACTRL_XO32M_CTRL_ENABLE_PLL_USB_OUT(1);

    return true;
}
