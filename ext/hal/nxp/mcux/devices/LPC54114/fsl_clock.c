/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2016 - 2018 , NXP
 * All rights reserved.
 *
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
#define MVALMAX (0x8000U)

#define PLL_MAX_N_DIV 0x100U

#define INDEX_SECTOR_TRIM48 ((uint32_t *)0x01000444U)
#define INDEX_SECTOR_TRIM96 ((uint32_t *)0x01000430U)
/*--------------------------------------------------------------------------
!!! If required these #defines can be moved to chip library file
----------------------------------------------------------------------------*/

#define PLL_SSCG0_MDEC_VAL_P (0U)                                /* MDEC is in bits  16 downto 0 */
#define PLL_SSCG0_MDEC_VAL_M (0x1FFFFUL << PLL_SSCG0_MDEC_VAL_P) /* NDEC is in bits  9 downto 0 */
#define PLL_NDEC_VAL_P (0U)                                      /* NDEC is in bits  9:0 */
#define PLL_NDEC_VAL_M (0x3FFUL << PLL_NDEC_VAL_P)
#define PLL_PDEC_VAL_P (0U) /* PDEC is in bits 6:0 */
#define PLL_PDEC_VAL_M (0x7FUL << PLL_PDEC_VAL_P)

#define PLL_MIN_CCO_FREQ_MHZ (75000000U)
#define PLL_MAX_CCO_FREQ_MHZ (150000000U)
#define PLL_LOWER_IN_LIMIT (4000U) /*!< Minimum PLL input rate */
#define PLL_MIN_IN_SSMODE (2000000U)
#define PLL_MAX_IN_SSMODE (4000000U)

/* Middle of the range values for spread-spectrum */
#define PLL_SSCG_MF_FREQ_VALUE 4U
#define PLL_SSCG_MC_COMP_VALUE 2U
#define PLL_SSCG_MR_DEPTH_VALUE 4U
#define PLL_SSCG_DITHER_VALUE 0U

/* PLL NDEC reg */
#define PLL_NDEC_VAL_SET(value) (((unsigned long)(value) << PLL_NDEC_VAL_P) & PLL_NDEC_VAL_M)
/* PLL PDEC reg */
#define PLL_PDEC_VAL_SET(value) (((unsigned long)(value) << PLL_PDEC_VAL_P) & PLL_PDEC_VAL_M)
/* SSCG control0 */
#define PLL_SSCG0_MDEC_VAL_SET(value) (((unsigned long)(value) << PLL_SSCG0_MDEC_VAL_P) & PLL_SSCG0_MDEC_VAL_M)

/* SSCG control1 */
#define PLL_SSCG1_MD_FRACT_P 0U
#define PLL_SSCG1_MD_INT_P 11U
#define PLL_SSCG1_MD_FRACT_M (0x7FFUL << PLL_SSCG1_MD_FRACT_P)
#define PLL_SSCG1_MD_INT_M (0xFFUL << PLL_SSCG1_MD_INT_P)

#define PLL_SSCG1_MD_FRACT_SET(value) (((unsigned long)(value) << PLL_SSCG1_MD_FRACT_P) & PLL_SSCG1_MD_FRACT_M)
#define PLL_SSCG1_MD_INT_SET(value) (((unsigned long)(value) << PLL_SSCG1_MD_INT_P) & PLL_SSCG1_MD_INT_M)

/* Saved value of PLL output rate, computed whenever needed to save run-time
   computation on each call to retrive the PLL rate. */
static uint32_t s_Pll_Freq;

/* I2S mclk. */
static uint32_t s_I2S_Mclk_Freq = 0U;

/** External clock rate on the CLKIN pin in Hz. If not used,
    set this to 0. Otherwise, set it to the exact rate in Hz this pin is
    being driven at. */
static const uint32_t s_Ext_Clk_Freq = 0U;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/* Find encoded NDEC value for raw N value, max N = NVALMAX */
static uint32_t pllEncodeN(uint32_t N);
/* Find decoded N value for raw NDEC value */
static uint32_t pllDecodeN(uint32_t NDEC);
/* Find encoded PDEC value for raw P value, max P = PVALMAX */
static uint32_t pllEncodeP(uint32_t P);
/* Find decoded P value for raw PDEC value */
static uint32_t pllDecodeP(uint32_t PDEC);
/* Find encoded MDEC value for raw M value, max M = MVALMAX */
static uint32_t pllEncodeM(uint32_t M);
/* Find decoded M value for raw MDEC value */
static uint32_t pllDecodeM(uint32_t MDEC);
/* Find SELP, SELI, and SELR values for raw M value, max M = MVALMAX */
static void pllFindSel(uint32_t M, bool bypassFBDIV2, uint32_t *pSelP, uint32_t *pSelI, uint32_t *pSelR);
/* Get predivider (N) from PLL NDEC setting */
static uint32_t findPllPreDiv(uint32_t ctrlReg, uint32_t nDecReg);
/* Get postdivider (P) from PLL PDEC setting */
static uint32_t findPllPostDiv(uint32_t ctrlReg, uint32_t pDecReg);
/* Get multiplier (M) from PLL MDEC and BYPASS_FBDIV2 settings */
static uint32_t findPllMMult(uint32_t ctrlReg, uint32_t mDecReg);
/* Get the greatest common divisor */
static uint32_t FindGreatestCommonDivisor(uint32_t m, uint32_t n);
/* Set PLL output based on desired output rate */
static pll_error_t CLOCK_GetPllConfig(
    uint32_t finHz, uint32_t foutHz, pll_setup_t *pSetup, bool useFeedbackDiv2, bool useSS);
/* Update local PLL rate variable */
static void CLOCK_GetSystemPLLOutFromSetupUpdate(pll_setup_t *pSetup);

static const uint8_t wdtFreqLookup[32] = {0,  8,  12, 15, 18, 20, 24, 26, 28, 30, 32, 34, 36, 38, 40, 41,
                                          42, 44, 45, 46, 48, 49, 50, 52, 53, 54, 56, 57, 58, 59, 60, 61};
/*******************************************************************************
 * Code
 ******************************************************************************/

/**
 * brief	Configure the clock selection muxes.
 * param	connection	: Clock to be configured.
 * return	Nothing
 */
void CLOCK_AttachClk(clock_attach_id_t connection)
{
    uint8_t mux;
    uint8_t sel;
    uint16_t item;
    uint32_t i;
    volatile uint32_t *pClkSel;

    pClkSel = &(SYSCON->MAINCLKSELA);

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
                if (mux == CM_ASYNCAPB)
                {
                    ASYNC_SYSCON->ASYNCAPBCLKSELA = sel;
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

    pClkSel = &(SYSCON->MAINCLKSELA);

    if (attachId == kNONE_to_NONE)
    {
        return kNONE_to_NONE;
    }

    for (i = 0U; i < 2U; i++)
    {
        mux = GET_ID_ITEM_MUX(attachId);
        if (attachId)
        {
            if (mux == CM_ASYNCAPB)
            {
                actualSel = ASYNC_SYSCON->ASYNCAPBCLKSELA;
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

/**
 * brief	Setup peripheral clock dividers.
 * param	div_name	: Clock divider name
 * param divided_by_value: Value to be divided
 * param reset :  Whether to reset the divider counter.
 * return	Nothing
 */
void CLOCK_SetClkDiv(clock_div_name_t div_name, uint32_t divided_by_value, bool reset)
{
    volatile uint32_t *pClkDiv;

    pClkDiv = &(SYSCON->SYSTICKCLKDIV);
    if (reset)
    {
        pClkDiv[div_name] = 1U << 29U;
    }
    if (divided_by_value == 0U) /* halt */
    {
        pClkDiv[div_name] = 1U << 30U;
    }
    else
    {
        pClkDiv[div_name] = (divided_by_value - 1U);
    }
}

/* Set FRO Clocking */
/**
 * brief	Initialize the Core clock to given frequency (12, 48 or 96 MHz).
 * Turns on FRO and uses default CCO, if freq is 12000000, then high speed output is off, else high speed output is
 * enabled.
 * param	iFreq	: Desired frequency (must be one of CLK_FRO_12MHZ or CLK_FRO_48MHZ or CLK_FRO_96MHZ)
 * return	returns success or fail status.
 */
status_t CLOCK_SetupFROClocking(uint32_t iFreq)
{
    uint32_t usb_adj;
    if ((iFreq != 12000000U) && (iFreq != 48000000U) && (iFreq != 96000000U))
    {
        return kStatus_Fail;
    }
    /* Power up the FRO and set this as the base clock */
    POWER_DisablePD(kPDRUNCFG_PD_FRO_EN);
    /* back up the value of whether USB adj is selected, in which case we will have a value of 1 else 0 */
    usb_adj = ((SYSCON->FROCTRL) & SYSCON_FROCTRL_USBCLKADJ_MASK) >> SYSCON_FROCTRL_USBCLKADJ_SHIFT;
    if (iFreq > 12000000U)
    {
        if (iFreq == 96000000U)
        {
            SYSCON->FROCTRL = ((SYSCON_FROCTRL_TRIM_MASK | SYSCON_FROCTRL_FREQTRIM_MASK) & *INDEX_SECTOR_TRIM96) |
                              SYSCON_FROCTRL_SEL(1) | SYSCON_FROCTRL_WRTRIM(1) | SYSCON_FROCTRL_USBCLKADJ(usb_adj) |
                              SYSCON_FROCTRL_HSPDCLK(1);
        }
        else
        {
            SYSCON->FROCTRL = ((SYSCON_FROCTRL_TRIM_MASK | SYSCON_FROCTRL_FREQTRIM_MASK) & *INDEX_SECTOR_TRIM48) |
                              SYSCON_FROCTRL_SEL(0) | SYSCON_FROCTRL_WRTRIM(1) | SYSCON_FROCTRL_USBCLKADJ(usb_adj) |
                              SYSCON_FROCTRL_HSPDCLK(1);
        }
    }
    else
    {
        SYSCON->FROCTRL &= ~SYSCON_FROCTRL_HSPDCLK(1);
    }

    return 0U;
}

/*! brief	Return Frequency of FRO 12MHz
 *  return	Frequency of FRO 12MHz
 */
uint32_t CLOCK_GetFro12MFreq(void)
{
    return (SYSCON->PDRUNCFG[0] & SYSCON_PDRUNCFG_PDEN_FRO_MASK) ? 0U : 12000000U;
}

/*! brief	Return Frequency of External Clock
 *  return	Frequency of External Clock. If no external clock is used returns 0.
 */
uint32_t CLOCK_GetExtClkFreq(void)
{
    return (s_Ext_Clk_Freq);
}
/*! brief	Return Frequency of Watchdog Oscillator
 *  return	Frequency of Watchdog Oscillator
 */
uint32_t CLOCK_GetWdtOscFreq(void)
{
    uint8_t freq_sel, div_sel;
    if (SYSCON->PDRUNCFG[kPDRUNCFG_PD_WDT_OSC >> 8UL] & (1UL << (kPDRUNCFG_PD_WDT_OSC & 0xffU)))
    {
        return 0U;
    }
    else
    {
        div_sel = ((SYSCON->WDTOSCCTRL & 0x1f) + 1) << 1;
        freq_sel =
            wdtFreqLookup[((SYSCON->WDTOSCCTRL & SYSCON_WDTOSCCTRL_FREQSEL_MASK) >> SYSCON_WDTOSCCTRL_FREQSEL_SHIFT)];
        return ((uint32_t)freq_sel * 50000U) / ((uint32_t)div_sel);
    }
}

/* Get HF FRO Clk */
/*! brief	Return Frequency of High-Freq output of FRO
 *  return	Frequency of High-Freq output of FRO
 */
uint32_t CLOCK_GetFroHfFreq(void)
{
    if ((SYSCON->PDRUNCFG[0] & SYSCON_PDRUNCFG_PDEN_FRO_MASK) || !(SYSCON->FROCTRL & SYSCON_FROCTRL_HSPDCLK_MASK))
    {
        return 0U;
    }

    if (SYSCON->FROCTRL & SYSCON_FROCTRL_SEL_MASK)
    {
        return 96000000U;
    }
    else
    {
        return 48000000U;
    }
}

/*! brief	Return Frequency of PLL
 *  return	Frequency of PLL
 */
uint32_t CLOCK_GetPllOutFreq(void)
{
    return s_Pll_Freq;
}

/*! brief	Return Frequency of 32kHz osc
 *  return	Frequency of 32kHz osc
 */
uint32_t CLOCK_GetOsc32KFreq(void)
{
    return CLK_RTC_32K_CLK; /* Needs to be corrected to check that RTC Clock is enabled */
}
/*! brief	Return Frequency of Core System
 *  return	Frequency of Core System
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
                freq = CLOCK_GetWdtOscFreq();
            }
            else if (SYSCON->MAINCLKSELA == 3U)
            {
                freq = CLOCK_GetFroHfFreq();
            }
            else
            {
            }
            break;
        case 2U:
            freq = CLOCK_GetPllOutFreq();
            break;

        case 3U:
            freq = CLOCK_GetOsc32KFreq();
            break;

        default:
            break;
    }

    return freq;
}
/*! brief	Return Frequency of I2S MCLK Clock
 *  return	Frequency of I2S MCLK Clock
 */
uint32_t CLOCK_GetI2SMClkFreq(void)
{
    return s_I2S_Mclk_Freq;
}

/*! brief	Return Frequency of Asynchronous APB Clock
 *  return	Frequency of Asynchronous APB Clock Clock
 */
uint32_t CLOCK_GetAsyncApbClkFreq(void)
{
    async_clock_src_t clkSrc;
    uint32_t clkRate;

    clkSrc = CLOCK_GetAsyncApbClkSrc();

    switch (clkSrc)
    {
        case kCLOCK_AsyncMainClk:
            clkRate = CLOCK_GetCoreSysClkFreq();
            break;
        case kCLOCK_AsyncFro12Mhz:
            clkRate = CLK_FRO_12MHZ;
            break;
        default:
            clkRate = 0U;
            break;
    }

    return clkRate;
}

/* Get FLEXCOMM Clk */
/*! brief	Return Frequency of Flexcomm functional Clock
 *  return	Frequency of Flexcomm functional Clock
 */
uint32_t CLOCK_GetFlexCommClkFreq(uint32_t id)
{
    uint32_t freq = 0U;

    switch (SYSCON->FXCOMCLKSEL[id])
    {
        case 0U:
            freq = CLOCK_GetFro12MFreq();
            break;
        case 1U:
            freq = CLOCK_GetFroHfFreq();
            break;
        case 2U:
            freq = CLOCK_GetPllOutFreq();
            break;
        case 3U:
            freq = CLOCK_GetI2SMClkFreq();
            break;
        case 4U:
            freq = CLOCK_GetFrgClkFreq();
            break;

        default:
            break;
    }

    return freq;
}

/* Get FRG Clk */
/*! brief	Return Input frequency for the Fractional baud rate generator
 *  return	Input Frequency for FRG
 */
uint32_t CLOCK_GetFRGInputClock(void)
{
    uint32_t freq = 0U;

    switch (SYSCON->FRGCLKSEL)
    {
        case 0U:
            freq = CLOCK_GetCoreSysClkFreq();
            break;
        case 1U:
            freq = CLOCK_GetPllOutFreq();
            break;
        case 2U:
            freq = CLOCK_GetFro12MFreq();
            break;
        case 3U:
            freq = CLOCK_GetFroHfFreq();
            break;

        default:
            break;
    }

    return freq;
}

/* Get DMIC Clk */
/*! brief  Return Input frequency for the DMIC
 *  return Input Frequency for DMIC
 */
uint32_t CLOCK_GetDmicClkFreq(void)
{
    uint32_t freq = 0U;

    switch (SYSCON->DMICCLKSEL)
    {
        case 0U:
            freq = CLOCK_GetFro12MFreq();
            break;
        case 1U:
            freq = CLOCK_GetFroHfFreq();
            break;
        case 2U:
            freq = CLOCK_GetPllOutFreq();
            break;
        case 3U:
            freq = CLOCK_GetI2SMClkFreq();
            break;
        case 4U:
            freq = CLOCK_GetCoreSysClkFreq();
            break;
        case 5U:
            freq = CLOCK_GetWdtOscFreq();
            break;
        default:
            break;
    }

    return freq / ((SYSCON->DMICCLKDIV & 0xffU) + 1U);
    ;
}

/*! brief	Set output of the Fractional baud rate generator
 * param	freq	: Desired output frequency
 * return	Error Code 0 - fail 1 - success
 */
uint32_t CLOCK_SetFRGClock(uint32_t freq)
{
    uint32_t input = CLOCK_GetFRGInputClock();
    uint32_t mul;

    if ((freq > 48000000) || (freq > input) || (input / freq >= 2))
    {
        /* FRG output frequency should be less than equal to 48MHz */
        return 0;
    }
    else
    {
        mul = ((uint64_t)(input - freq) * 256) / ((uint64_t)freq);
        SYSCON->FRGCTRL = (mul << SYSCON_FRGCTRL_MULT_SHIFT) | SYSCON_FRGCTRL_DIV_MASK;
        return 1;
    }
}

/* Get FRG Clk */
/*! brief  Return Input frequency for the FRG
 *  return Input Frequency for FRG
 */
uint32_t CLOCK_GetFrgClkFreq(void)
{
    uint32_t freq = 0U;

    if ((SYSCON->FRGCTRL & SYSCON_FRGCTRL_DIV_MASK) == SYSCON_FRGCTRL_DIV_MASK)
    {
        freq = ((uint64_t)CLOCK_GetFRGInputClock() * (SYSCON_FRGCTRL_DIV_MASK + 1)) /
               ((SYSCON_FRGCTRL_DIV_MASK + 1) +
                ((SYSCON->FRGCTRL & SYSCON_FRGCTRL_MULT_MASK) >> SYSCON_FRGCTRL_MULT_SHIFT));
    }
    else
    {
        freq = 0U;
    }

    return freq;
}

/*! brief  Return Frequency of USB
 *  return Frequency of USB
 */
uint32_t CLOCK_GetUsbClkFreq(void)
{
    uint32_t freq = 0U;

    if (SYSCON->USBCLKSEL == 0U)
    {
        freq = CLOCK_GetFroHfFreq();
    }
    else if (SYSCON->USBCLKSEL == 1)
    {
        freq = CLOCK_GetPllOutFreq();
    }
    else
    {
    }

    return freq / ((SYSCON->USBCLKDIV & 0xffU) + 1U);
}

/*! brief	Return Frequency of selected clock
 *  return	Frequency of selected clock
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
        case kCLOCK_FroHf:
            freq = CLOCK_GetFroHfFreq();
            break;
        case kCLOCK_Fro12M:
            freq = CLOCK_GetFro12MFreq();
            break;
        case kCLOCK_PllOut:
            freq = CLOCK_GetPllOutFreq();
            break;
        case kCLOCK_UsbClk:
            freq = CLOCK_GetUsbClkFreq();
            break;
        case kCLOCK_WdtOsc:
            freq = CLOCK_GetWdtOscFreq();
            break;
        case kCLOCK_Frg:
            freq = CLOCK_GetFrgClkFreq();
            break;
        case kCLOCK_Dmic:
            freq = CLOCK_GetDmicClkFreq();
            break;

        case kCLOCK_AsyncApbClk:
            freq = CLOCK_GetAsyncApbClkFreq();
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
        default:
            freq = 0U;
            break;
    }

    return freq;
}

/* Set the FLASH wait states for the passed frequency */
/**
 * brief	Set the flash wait states for the input freuqency.
 * param	iFreq	: Input frequency
 * return	Nothing
 */
void CLOCK_SetFLASHAccessCyclesForFreq(uint32_t iFreq)
{
    if (iFreq <= 12000000U)
    {
        CLOCK_SetFLASHAccessCycles(kCLOCK_Flash1Cycle);
    }
    else if (iFreq <= 30000000U)
    {
        CLOCK_SetFLASHAccessCycles(kCLOCK_Flash2Cycle);
    }
    else if (iFreq <= 60000000U)
    {
        CLOCK_SetFLASHAccessCycles(kCLOCK_Flash3Cycle);
    }
    else if (iFreq <= 85000000U)
    {
        CLOCK_SetFLASHAccessCycles(kCLOCK_Flash4Cycle);
    }
    else
    {
        CLOCK_SetFLASHAccessCycles(kCLOCK_Flash5Cycle);
    }
}

/* Find encoded NDEC value for raw N value, max N = NVALMAX */
static uint32_t pllEncodeN(uint32_t N)
{
    uint32_t x, i;

    /* Find NDec */
    switch (N)
    {
        case 0U:
            x = 0x3FFU;
            break;

        case 1U:
            x = 0x302U;
            break;

        case 2U:
            x = 0x202U;
            break;

        default:
            x = 0x080U;
            for (i = N; i <= NVALMAX; i++)
            {
                x = (((x ^ (x >> 2U) ^ (x >> 3U) ^ (x >> 4U)) & 1U) << 7U) | ((x >> 1U) & 0x7FU);
            }
            break;
    }

    return x & (PLL_NDEC_VAL_M >> PLL_NDEC_VAL_P);
}

/* Find decoded N value for raw NDEC value */
static uint32_t pllDecodeN(uint32_t NDEC)
{
    uint32_t n, x, i;

    /* Find NDec */
    switch (NDEC)
    {
        case 0x3FFU:
            n = 0U;
            break;

        case 0x302U:
            n = 1U;
            break;

        case 0x202U:
            n = 2U;
            break;

        default:
            x = 0x080U;
            n = 0xFFFFFFFFU;
            for (i = NVALMAX; ((i >= 3U) && (n == 0xFFFFFFFFU)); i--)
            {
                x = (((x ^ (x >> 2U) ^ (x >> 3U) ^ (x >> 4U)) & 1U) << 7U) | ((x >> 1U) & 0x7FU);
                if ((x & (PLL_NDEC_VAL_M >> PLL_NDEC_VAL_P)) == NDEC)
                {
                    /* Decoded value of NDEC */
                    n = i;
                }
            }
            break;
    }

    return n;
}

/* Find encoded PDEC value for raw P value, max P = PVALMAX */
static uint32_t pllEncodeP(uint32_t P)
{
    uint32_t x, i;

    /* Find PDec */
    switch (P)
    {
        case 0U:
            x = 0x7FU;
            break;

        case 1U:
            x = 0x62U;
            break;

        case 2U:
            x = 0x42U;
            break;

        default:
            x = 0x10U;
            for (i = P; i <= PVALMAX; i++)
            {
                x = (((x ^ (x >> 2U)) & 1U) << 4U) | ((x >> 1U) & 0xFU);
            }
            break;
    }

    return x & (PLL_PDEC_VAL_M >> PLL_PDEC_VAL_P);
}

/* Find decoded P value for raw PDEC value */
static uint32_t pllDecodeP(uint32_t PDEC)
{
    uint32_t p, x, i;

    /* Find PDec */
    switch (PDEC)
    {
        case 0x7FU:
            p = 0U;
            break;

        case 0x62U:
            p = 1U;
            break;

        case 0x42U:
            p = 2U;
            break;

        default:
            x = 0x10U;
            p = 0xFFFFFFFFU;
            for (i = PVALMAX; ((i >= 3U) && (p == 0xFFFFFFFFU)); i--)
            {
                x = (((x ^ (x >> 2U)) & 1U) << 4U) | ((x >> 1U) & 0xFU);
                if ((x & (PLL_PDEC_VAL_M >> PLL_PDEC_VAL_P)) == PDEC)
                {
                    /* Decoded value of PDEC */
                    p = i;
                }
            }
            break;
    }

    return p;
}

/* Find encoded MDEC value for raw M value, max M = MVALMAX */
static uint32_t pllEncodeM(uint32_t M)
{
    uint32_t i, x;

    /* Find MDec */
    switch (M)
    {
        case 0U:
            x = 0x1FFFFU;
            break;

        case 1U:
            x = 0x18003U;
            break;

        case 2U:
            x = 0x10003U;
            break;

        default:
            x = 0x04000U;
            for (i = M; i <= MVALMAX; i++)
            {
                x = (((x ^ (x >> 1U)) & 1U) << 14U) | ((x >> 1U) & 0x3FFFU);
            }
            break;
    }

    return x & (PLL_SSCG0_MDEC_VAL_M >> PLL_SSCG0_MDEC_VAL_P);
}

/* Find decoded M value for raw MDEC value */
static uint32_t pllDecodeM(uint32_t MDEC)
{
    uint32_t m, i, x;

    /* Find MDec */
    switch (MDEC)
    {
        case 0x1FFFFU:
            m = 0U;
            break;

        case 0x18003U:
            m = 1U;
            break;

        case 0x10003U:
            m = 2U;
            break;

        default:
            x = 0x04000U;
            m = 0xFFFFFFFFU;
            for (i = MVALMAX; ((i >= 3U) && (m == 0xFFFFFFFFU)); i--)
            {
                x = (((x ^ (x >> 1U)) & 1) << 14U) | ((x >> 1U) & 0x3FFFU);
                if ((x & (PLL_SSCG0_MDEC_VAL_M >> PLL_SSCG0_MDEC_VAL_P)) == MDEC)
                {
                    /* Decoded value of MDEC */
                    m = i;
                }
            }
            break;
    }

    return m;
}

/* Find SELP, SELI, and SELR values for raw M value, max M = MVALMAX */
static void pllFindSel(uint32_t M, bool bypassFBDIV2, uint32_t *pSelP, uint32_t *pSelI, uint32_t *pSelR)
{
    /* bandwidth: compute selP from Multiplier */
    if (M < 60U)
    {
        *pSelP = (M >> 1U) + 1U;
    }
    else
    {
        *pSelP = PVALMAX - 1U;
    }

    /* bandwidth: compute selI from Multiplier */
    if (M > 16384U)
    {
        *pSelI = 1U;
    }
    else if (M > 8192U)
    {
        *pSelI = 2U;
    }
    else if (M > 2048U)
    {
        *pSelI = 4U;
    }
    else if (M >= 501U)
    {
        *pSelI = 8U;
    }
    else if (M >= 60U)
    {
        *pSelI = 4U * (1024U / (M + 9U));
    }
    else
    {
        *pSelI = (M & 0x3CU) + 4U;
    }

    if (*pSelI > ((0x3FUL << SYSCON_SYSPLLCTRL_SELI_SHIFT) >> SYSCON_SYSPLLCTRL_SELI_SHIFT))
    {
        *pSelI = ((0x3FUL << SYSCON_SYSPLLCTRL_SELI_SHIFT) >> SYSCON_SYSPLLCTRL_SELI_SHIFT);
    }

    *pSelR = 0U;
}

/* Get predivider (N) from PLL NDEC setting */
static uint32_t findPllPreDiv(uint32_t ctrlReg, uint32_t nDecReg)
{
    uint32_t preDiv = 1;

    /* Direct input is not used? */
    if ((ctrlReg & (1UL << SYSCON_SYSPLLCTRL_DIRECTI_SHIFT)) == 0U)
    {
        /* Decode NDEC value to get (N) pre divider */
        preDiv = pllDecodeN(nDecReg & 0x3FFU);
        if (preDiv == 0U)
        {
            preDiv = 1U;
        }
    }

    /* Adjusted by 1, directi is used to bypass */
    return preDiv;
}

/* Get postdivider (P) from PLL PDEC setting */
static uint32_t findPllPostDiv(uint32_t ctrlReg, uint32_t pDecReg)
{
    uint32_t postDiv = 1U;

    /* Direct input is not used? */
    if ((ctrlReg & SYSCON_SYSPLLCTRL_DIRECTO_MASK) == 0U)
    {
        /* Decode PDEC value to get (P) post divider */
        postDiv = 2U * pllDecodeP(pDecReg & 0x7FU);
        if (postDiv == 0U)
        {
            postDiv = 2U;
        }
    }

    /* Adjusted by 1, directo is used to bypass */
    return postDiv;
}

/* Get multiplier (M) from PLL MDEC and BYPASS_FBDIV2 settings */
static uint32_t findPllMMult(uint32_t ctrlReg, uint32_t mDecReg)
{
    uint32_t mMult = 1U;

    /* Decode MDEC value to get (M) multiplier */
    mMult = pllDecodeM(mDecReg & 0x1FFFFU);

    /* Extra multiply by 2 needed? */
    if ((ctrlReg & (SYSCON_SYSPLLCTRL_BYPASSCCODIV2_MASK)) == 0U)
    {
        mMult = mMult << 1U;
    }

    if (mMult == 0U)
    {
        mMult = 1U;
    }

    return mMult;
}

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
 * Set PLL output based on desired output rate.
 * In this function, the it calculates the PLL setting for output frequency from input clock
 * frequency. The calculation would cost a few time. So it is not recommaned to use it frequently.
 * the "pllctrl", "pllndec", "pllpdec", "pllmdec" would updated in this function.
 */
static pll_error_t CLOCK_GetPllConfigInternal(
    uint32_t finHz, uint32_t foutHz, pll_setup_t *pSetup, bool useFeedbackDiv2, bool useSS)
{
    uint32_t nDivOutHz, fccoHz, multFccoDiv;
    uint32_t pllPreDivider, pllMultiplier, pllBypassFBDIV2, pllPostDivider;
    uint32_t pllDirectInput, pllDirectOutput;
    uint32_t pllSelP, pllSelI, pllSelR, bandsel, uplimoff;

    /* Baseline parameters (no input or output dividers) */
    pllPreDivider = 1U;  /* 1 implies pre-divider will be disabled */
    pllPostDivider = 0U; /* 0 implies post-divider will be disabled */
    pllDirectOutput = 1U;
    if (useFeedbackDiv2)
    {
        /* Using feedback divider for M, so disable bypass */
        pllBypassFBDIV2 = 0U;
    }
    else
    {
        pllBypassFBDIV2 = 1U;
    }
    multFccoDiv = (2U - pllBypassFBDIV2);

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

    /* If using SS mode, input clock needs to be between 2MHz and 4MHz */
    if (useSS)
    {
        /* Verify input rate parameter */
        if (finHz < PLL_MIN_IN_SSMODE)
        {
            /* Input clock into the PLL cannot be lower than this */
            return kStatus_PLL_InputTooLow;
        }
        /* PLL input in SS mode must be under 4MHz */
        pllPreDivider = finHz / ((PLL_MIN_IN_SSMODE + PLL_MAX_IN_SSMODE) / 2);
        if (pllPreDivider > NVALMAX)
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
        fccoHz = foutHz * (pllPostDivider * 2U);
        pllDirectOutput = 0U;
    }

    /* Determine if a pre-divider is needed to get the best frequency */
    if ((finHz > PLL_LOWER_IN_LIMIT) && (fccoHz >= finHz) && (useSS == false))
    {
        uint32_t a = FindGreatestCommonDivisor(fccoHz, (multFccoDiv * finHz));

        if (a > 20000U)
        {
            a = (multFccoDiv * finHz) / a;
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
    pllMultiplier = (fccoHz / nDivOutHz) / multFccoDiv;

    /* Find optimal values for filter */
    if (useSS == false)
    {
        /* Will bumping up M by 1 get us closer to the desired CCO frequency? */
        if ((nDivOutHz * ((multFccoDiv * pllMultiplier * 2U) + 1U)) < (fccoHz * 2U))
        {
            pllMultiplier++;
        }

        /* Setup filtering */
        pllFindSel(pllMultiplier, pllBypassFBDIV2, &pllSelP, &pllSelI, &pllSelR);
        bandsel = 1U;
        uplimoff = 0U;

        /* Get encoded value for M (mult) and use manual filter, disable SS mode */
        pSetup->syspllssctrl[0] =
            (PLL_SSCG0_MDEC_VAL_SET(pllEncodeM(pllMultiplier)) | (1U << SYSCON_SYSPLLSSCTRL0_SEL_EXT_SHIFT));

        /* Power down SSC, not used */
        pSetup->syspllssctrl[1] = (1U << SYSCON_SYSPLLSSCTRL1_PD_SHIFT);
    }
    else
    {
        uint64_t fc;

        /* Filtering will be handled by SSC */
        pllSelR = pllSelI = pllSelP = 0U;
        bandsel = 0U;
        uplimoff = 1U;

        /* The PLL multiplier will get very close and slightly under the
           desired target frequency. A small fractional component can be
           added to fine tune the frequency upwards to the target. */
        fc = ((uint64_t)(fccoHz % (multFccoDiv * nDivOutHz)) << 11U) / (multFccoDiv * nDivOutHz);

        /* MDEC set by SSC */
        pSetup->syspllssctrl[0U] = 0U;

        /* Set multiplier */
        pSetup->syspllssctrl[1] = PLL_SSCG1_MD_INT_SET(pllMultiplier) | PLL_SSCG1_MD_FRACT_SET((uint32_t)fc);
    }

    /* Get encoded values for N (prediv) and P (postdiv) */
    pSetup->syspllndec = PLL_NDEC_VAL_SET(pllEncodeN(pllPreDivider));
    pSetup->syspllpdec = PLL_PDEC_VAL_SET(pllEncodeP(pllPostDivider));

    /* PLL control */
    pSetup->syspllctrl = (pllSelR << SYSCON_SYSPLLCTRL_SELR_SHIFT) |                  /* Filter coefficient */
                         (pllSelI << SYSCON_SYSPLLCTRL_SELI_SHIFT) |                  /* Filter coefficient */
                         (pllSelP << SYSCON_SYSPLLCTRL_SELP_SHIFT) |                  /* Filter coefficient */
                         (0 << SYSCON_SYSPLLCTRL_BYPASS_SHIFT) |                      /* PLL bypass mode disabled */
                         (pllBypassFBDIV2 << SYSCON_SYSPLLCTRL_BYPASSCCODIV2_SHIFT) | /* Extra M / 2 divider? */
                         (uplimoff << SYSCON_SYSPLLCTRL_UPLIMOFF_SHIFT) |             /* SS/fractional mode disabled */
                         (bandsel << SYSCON_SYSPLLCTRL_BANDSEL_SHIFT) |        /* Manual bandwidth selection enabled */
                         (pllDirectInput << SYSCON_SYSPLLCTRL_DIRECTI_SHIFT) | /* Bypass pre-divider? */
                         (pllDirectOutput << SYSCON_SYSPLLCTRL_DIRECTO_SHIFT); /* Bypass post-divider? */

    return kStatus_PLL_Success;
}

#if (defined(CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT) && CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT)
/* Alloct the static buffer for cache. */
static pll_setup_t s_PllSetupCacheStruct[CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT];
static uint32_t s_FinHzCache[CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT] = {0};
static uint32_t s_FoutHzCache[CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT] = {0};
static bool s_UseFeedbackDiv2Cache[CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT] = {false};
static bool s_UseSSCache[CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT] = {false};
static uint32_t s_PllSetupCacheIdx = 0U;
#endif /* CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT */

/*
 * Calculate the PLL setting values from input clock freq to output freq.
 */
static pll_error_t CLOCK_GetPllConfig(
    uint32_t finHz, uint32_t foutHz, pll_setup_t *pSetup, bool useFeedbackDiv2, bool useSS)
{
    pll_error_t retErr;
#if (defined(CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT) && CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT)
    uint32_t i;

    for (i = 0U; i < CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT; i++)
    {
        if ((finHz == s_FinHzCache[i]) && (foutHz == s_FoutHzCache[i]) &&
            (useFeedbackDiv2 == s_UseFeedbackDiv2Cache[i]) && (useSS == s_UseSSCache[i]))
        {
            /* Hit the target in cache buffer. */
            pSetup->syspllctrl = s_PllSetupCacheStruct[i].syspllctrl;
            pSetup->syspllndec = s_PllSetupCacheStruct[i].syspllndec;
            pSetup->syspllpdec = s_PllSetupCacheStruct[i].syspllpdec;
            pSetup->syspllssctrl[0] = s_PllSetupCacheStruct[i].syspllssctrl[0];
            pSetup->syspllssctrl[1] = s_PllSetupCacheStruct[i].syspllssctrl[1];
            retErr = kStatus_PLL_Success;
            break;
        }
    }

    if (i < CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT)
    {
        return retErr;
    }
#endif /* CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT */

    retErr = CLOCK_GetPllConfigInternal(finHz, foutHz, pSetup, useFeedbackDiv2, useSS);

#if (defined(CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT) && CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT)
    /* Cache the most recent calulation result into buffer. */
    s_FinHzCache[s_PllSetupCacheIdx] = finHz;
    s_FoutHzCache[s_PllSetupCacheIdx] = foutHz;
    s_UseFeedbackDiv2Cache[s_PllSetupCacheIdx] = useFeedbackDiv2;
    s_UseSSCache[s_PllSetupCacheIdx] = useSS;

    s_PllSetupCacheStruct[s_PllSetupCacheIdx].syspllctrl = pSetup->syspllctrl;
    s_PllSetupCacheStruct[s_PllSetupCacheIdx].syspllndec = pSetup->syspllndec;
    s_PllSetupCacheStruct[s_PllSetupCacheIdx].syspllpdec = pSetup->syspllpdec;
    s_PllSetupCacheStruct[s_PllSetupCacheIdx].syspllssctrl[0] = pSetup->syspllssctrl[0];
    s_PllSetupCacheStruct[s_PllSetupCacheIdx].syspllssctrl[1] = pSetup->syspllssctrl[1];
    /* Update the index for next available buffer. */
    s_PllSetupCacheIdx = (s_PllSetupCacheIdx + 1U) % CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT;
#endif /* CLOCK_USR_CFG_PLL_CONFIG_CACHE_COUNT */

    return retErr;
}

/* Update local PLL rate variable */
static void CLOCK_GetSystemPLLOutFromSetupUpdate(pll_setup_t *pSetup)
{
    s_Pll_Freq = CLOCK_GetSystemPLLOutFromSetup(pSetup);
}

/* Return System PLL input clock rate */
/*! brief	Return System PLL input clock rate
 *  return	System PLL input clock rate
 */
uint32_t CLOCK_GetSystemPLLInClockRate(void)
{
    uint32_t clkRate = 0U;

    switch ((SYSCON->SYSPLLCLKSEL & SYSCON_SYSPLLCLKSEL_SEL_MASK))
    {
        case 0x00U:
            clkRate = CLK_FRO_12MHZ;
            break;

        case 0x01U:
            clkRate = CLOCK_GetExtClkFreq();
            break;

        case 0x02U:
            clkRate = CLOCK_GetWdtOscFreq();
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

/* Return System PLL output clock rate from setup structure */
/*! brief	Return System PLL output clock rate from setup structure
 *  param	pSetup	: Pointer to a PLL setup structure
 *  return	System PLL output clock rate calculated from the setup structure
 */
uint32_t CLOCK_GetSystemPLLOutFromSetup(pll_setup_t *pSetup)
{
    uint32_t prediv, postdiv, mMult, inPllRate;
    uint64_t workRate;

    /* Get the input clock frequency of PLL. */
    inPllRate = CLOCK_GetSystemPLLInClockRate();

    /*
    * If the PLL is bypassed, PLL would not be used and the output of PLL module would just be the input clock.
    */
    if ((pSetup->syspllctrl & (SYSCON_SYSPLLCTRL_BYPASS_MASK)) == 0U)
    {
        /* PLL is not in bypass mode, get pre-divider, and M divider, post-divider. */
        /*
         * 1. Pre-divider
         * Pre-divider is only available when the DIRECTI is disabled.
         */
        if (0U == (pSetup->syspllctrl & SYSCON_SYSPLLCTRL_DIRECTI_MASK))
        {
            prediv = findPllPreDiv(pSetup->syspllctrl, pSetup->syspllndec);
        }
        else
        {
            prediv = 1U; /* The pre-divider is bypassed. */
        }
        /* Adjust input clock */
        inPllRate = inPllRate / prediv;

        /*
         * 2. M divider
         * If using the SS, use the multiplier.
         */
        if (pSetup->syspllssctrl[1] & (SYSCON_SYSPLLSSCTRL1_PD_MASK))
        {
            /* MDEC used for rate */
            mMult = findPllMMult(pSetup->syspllctrl, pSetup->syspllssctrl[0]);
            workRate = (uint64_t)inPllRate * (uint64_t)mMult;
        }
        else
        {
            uint64_t fract;

            /* SS multipler used for rate */
            mMult = (pSetup->syspllssctrl[1] & PLL_SSCG1_MD_INT_M) >> PLL_SSCG1_MD_INT_P;
            workRate = (uint64_t)inPllRate * (uint64_t)mMult;

            /* Adjust by fractional */
            fract = (uint64_t)(pSetup->syspllssctrl[1] & PLL_SSCG1_MD_FRACT_M) >> PLL_SSCG1_MD_FRACT_P;
            workRate = workRate + ((inPllRate * fract) / 0x800U);
        }

        /*
         * 3. Post-divider
         * Post-divider is only available when the DIRECTO is disabled.
         */
        if (0U == (pSetup->syspllctrl & SYSCON_SYSPLLCTRL_DIRECTO_MASK))
        {
            postdiv = findPllPostDiv(pSetup->syspllctrl, pSetup->syspllpdec);
        }
        else
        {
            postdiv = 1U; /* The post-divider is bypassed. */
        }
        workRate = workRate / ((uint64_t)postdiv);
    }
    else
    {
        /* In bypass mode */
        workRate = (uint64_t)inPllRate;
    }

    return (uint32_t)workRate;
}

/* Set the current PLL Rate */
/*! brief Store the current PLL rate
 *  param	rate: Current rate of the PLL
 *  return	Nothing
 **/
void CLOCK_SetStoredPLLClockRate(uint32_t rate)
{
    s_Pll_Freq = rate;
}

/* Return System PLL output clock rate */
/*! brief	Return System PLL output clock rate
 *  param	recompute	: Forces a PLL rate recomputation if true
 *  return	System PLL output clock rate
 *  note	The PLL rate is cached in the driver in a variable as
 *  the rate computation function can take some time to perform. It
 *  is recommended to use 'false' with the 'recompute' parameter.
 */
uint32_t CLOCK_GetSystemPLLOutClockRate(bool recompute)
{
    pll_setup_t Setup;
    uint32_t rate;

    if ((recompute) || (s_Pll_Freq == 0U))
    {
        Setup.syspllctrl = SYSCON->SYSPLLCTRL;
        Setup.syspllndec = SYSCON->SYSPLLNDEC;
        Setup.syspllpdec = SYSCON->SYSPLLPDEC;
        Setup.syspllssctrl[0] = SYSCON->SYSPLLSSCTRL0;
        Setup.syspllssctrl[1] = SYSCON->SYSPLLSSCTRL1;

        CLOCK_GetSystemPLLOutFromSetupUpdate(&Setup);
    }

    rate = s_Pll_Freq;

    return rate;
}

/* Set PLL output based on the passed PLL setup data */
/*! brief	Set PLL output based on the passed PLL setup data
 *  param	pControl	: Pointer to populated PLL control structure to generate setup with
 *  param	pSetup		: Pointer to PLL setup structure to be filled
 *  return	PLL_ERROR_SUCCESS on success, or PLL setup error code
 *  note	Actual frequency for setup may vary from the desired frequency based on the
 *  accuracy of input clocks, rounding, non-fractional PLL mode, etc.
 */
pll_error_t CLOCK_SetupPLLData(pll_config_t *pControl, pll_setup_t *pSetup)
{
    uint32_t inRate;
    bool useSS = (bool)((pControl->flags & PLL_CONFIGFLAG_FORCENOFRACT) == 0U);
    bool useFbDiv2;

    pll_error_t pllError;

    /* Determine input rate for the PLL */
    if ((pControl->flags & PLL_CONFIGFLAG_USEINRATE) != 0U)
    {
        inRate = pControl->inputRate;
    }
    else
    {
        inRate = CLOCK_GetSystemPLLInClockRate();
    }

    if ((pSetup->flags & PLL_SETUPFLAG_USEFEEDBACKDIV2) != 0U)
    {
        useFbDiv2 = true;
    }
    else
    {
        useFbDiv2 = false;
    }

    /* PLL flag options */
    pllError = CLOCK_GetPllConfig(inRate, pControl->desiredRate, pSetup, useFbDiv2, useSS);
    if ((useSS) && (pllError == kStatus_PLL_Success))
    {
        /* If using SS mode, then some tweaks are made to the generated setup */
        pSetup->syspllssctrl[1] |= (uint32_t)pControl->ss_mf | (uint32_t)pControl->ss_mr | (uint32_t)pControl->ss_mc;
        if (pControl->mfDither)
        {
            pSetup->syspllssctrl[1] |= (1U << SYSCON_SYSPLLSSCTRL1_DITHER_SHIFT);
        }
    }

    return pllError;
}

/* Set PLL output from PLL setup structure */
/*! brief	Set PLL output from PLL setup structure (precise frequency)
 * param	pSetup	: Pointer to populated PLL setup structure
* param flagcfg : Flag configuration for PLL config structure
 * return	PLL_ERROR_SUCCESS on success, or PLL setup error code
 * note	This function will power off the PLL, setup the PLL with the
 * new setup data, and then optionally powerup the PLL, wait for PLL lock,
 * and adjust system voltages to the new PLL rate. The function will not
 * alter any source clocks (ie, main systen clock) that may use the PLL,
 * so these should be setup prior to and after exiting the function.
 */
pll_error_t CLOCK_SetupSystemPLLPrec(pll_setup_t *pSetup, uint32_t flagcfg)
{
    /* Power off PLL during setup changes */
    POWER_EnablePD(kPDRUNCFG_PD_SYS_PLL0);

    pSetup->flags = flagcfg;

    /* Write PLL setup data */
    SYSCON->SYSPLLCTRL = pSetup->syspllctrl;
    SYSCON->SYSPLLNDEC = pSetup->syspllndec;
    SYSCON->SYSPLLNDEC = pSetup->syspllndec | (1U << SYSCON_SYSPLLNDEC_NREQ_SHIFT); /* latch */
    SYSCON->SYSPLLPDEC = pSetup->syspllpdec;
    SYSCON->SYSPLLPDEC = pSetup->syspllpdec | (1U << SYSCON_SYSPLLPDEC_PREQ_SHIFT); /* latch */
    SYSCON->SYSPLLSSCTRL0 = pSetup->syspllssctrl[0];
    SYSCON->SYSPLLSSCTRL0 = pSetup->syspllssctrl[0] | (1U << SYSCON_SYSPLLSSCTRL0_MREQ_SHIFT); /* latch */
    SYSCON->SYSPLLSSCTRL1 = pSetup->syspllssctrl[1];
    SYSCON->SYSPLLSSCTRL1 = pSetup->syspllssctrl[1] | (1U << SYSCON_SYSPLLSSCTRL1_MDREQ_SHIFT); /* latch */

    /* Flags for lock or power on */
    if ((pSetup->flags & (PLL_SETUPFLAG_POWERUP | PLL_SETUPFLAG_WAITLOCK)) != 0U)
    {
        /* If turning the PLL back on, perform the following sequence to accelerate PLL lock */
        volatile uint32_t delayX;
        uint32_t maxCCO = (1U << 18U) | 0x5dd2U; /* CCO = 1.6Ghz + MDEC enabled*/
        uint32_t curSSCTRL = SYSCON->SYSPLLSSCTRL0 & ~(1U << 17U);

        /* Initialize  and power up PLL */
        SYSCON->SYSPLLSSCTRL0 = maxCCO;
        POWER_DisablePD(kPDRUNCFG_PD_SYS_PLL0);

        /* Set mreq to activate */
        SYSCON->SYSPLLSSCTRL0 = maxCCO | (1U << 17U);

        /* Delay for 72 uSec @ 12Mhz */
        for (delayX = 0U; delayX < 172U; ++delayX)
        {
        }

        /* clear mreq to prepare for restoring mreq */
        SYSCON->SYSPLLSSCTRL0 = curSSCTRL;

        /* set original value back and activate */
        SYSCON->SYSPLLSSCTRL0 = curSSCTRL | (1U << 17U);

        /* Enable peripheral states by setting low */
        POWER_DisablePD(kPDRUNCFG_PD_SYS_PLL0);
    }
    if ((pSetup->flags & PLL_SETUPFLAG_WAITLOCK) != 0U)
    {
        while (CLOCK_IsSystemPLLLocked() == false)
        {
        }
    }

    /* Update current programmed PLL rate var */
    CLOCK_GetSystemPLLOutFromSetupUpdate(pSetup);

    /* System voltage adjustment, occurs prior to setting main system clock */
    if ((pSetup->flags & PLL_SETUPFLAG_ADGVOLT) != 0U)
    {
        POWER_SetVoltageForFreq(s_Pll_Freq);
    }

    return kStatus_PLL_Success;
}

/* Setup PLL Frequency from pre-calculated value */
/**
 * brief	Set PLL output from PLL setup structure (precise frequency)
 * param	pSetup	: Pointer to populated PLL setup structure
 * return	kStatus_PLL_Success on success, or PLL setup error code
 * note	This function will power off the PLL, setup the PLL with the
 * new setup data, and then optionally powerup the PLL, wait for PLL lock,
 * and adjust system voltages to the new PLL rate. The function will not
 * alter any source clocks (ie, main systen clock) that may use the PLL,
 * so these should be setup prior to and after exiting the function.
 */
pll_error_t CLOCK_SetPLLFreq(const pll_setup_t *pSetup)
{
    /* Power off PLL during setup changes */
    POWER_EnablePD(kPDRUNCFG_PD_SYS_PLL0);

    /* Write PLL setup data */
    SYSCON->SYSPLLCTRL = pSetup->syspllctrl;
    SYSCON->SYSPLLNDEC = pSetup->syspllndec;
    SYSCON->SYSPLLNDEC = pSetup->syspllndec | (1U << SYSCON_SYSPLLNDEC_NREQ_SHIFT); /* latch */
    SYSCON->SYSPLLPDEC = pSetup->syspllpdec;
    SYSCON->SYSPLLPDEC = pSetup->syspllpdec | (1U << SYSCON_SYSPLLPDEC_PREQ_SHIFT); /* latch */
    SYSCON->SYSPLLSSCTRL0 = pSetup->syspllssctrl[0];
    SYSCON->SYSPLLSSCTRL0 = pSetup->syspllssctrl[0] | (1U << SYSCON_SYSPLLSSCTRL0_MREQ_SHIFT); /* latch */
    SYSCON->SYSPLLSSCTRL1 = pSetup->syspllssctrl[1];
    SYSCON->SYSPLLSSCTRL1 = pSetup->syspllssctrl[1] | (1U << SYSCON_SYSPLLSSCTRL1_MDREQ_SHIFT); /* latch */

    /* Flags for lock or power on */
    if ((pSetup->flags & (PLL_SETUPFLAG_POWERUP | PLL_SETUPFLAG_WAITLOCK)) != 0)
    {
        /* If turning the PLL back on, perform the following sequence to accelerate PLL lock */
        volatile uint32_t delayX;
        uint32_t maxCCO = (1U << 18U) | 0x5dd2U; /* CCO = 1.6Ghz + MDEC enabled*/
        uint32_t curSSCTRL = SYSCON->SYSPLLSSCTRL0 & ~(1U << 17U);

        /* Initialize  and power up PLL */
        SYSCON->SYSPLLSSCTRL0 = maxCCO;
        POWER_DisablePD(kPDRUNCFG_PD_SYS_PLL0);

        /* Set mreq to activate */
        SYSCON->SYSPLLSSCTRL0 = maxCCO | (1U << 17U);

        /* Delay for 72 uSec @ 12Mhz */
        for (delayX = 0U; delayX < 172U; ++delayX)
        {
        }

        /* clear mreq to prepare for restoring mreq */
        SYSCON->SYSPLLSSCTRL0 = curSSCTRL;

        /* set original value back and activate */
        SYSCON->SYSPLLSSCTRL0 = curSSCTRL | (1U << 17U);

        /* Enable peripheral states by setting low */
        POWER_DisablePD(kPDRUNCFG_PD_SYS_PLL0);
    }
    if ((pSetup->flags & PLL_SETUPFLAG_WAITLOCK) != 0U)
    {
        while (CLOCK_IsSystemPLLLocked() == false)
        {
        }
    }

    /* Update current programmed PLL rate var */
    s_Pll_Freq = pSetup->pllRate;

    return kStatus_PLL_Success;
}

/* Set System PLL clock based on the input frequency and multiplier */
/*! brief	Set PLL output based on the multiplier and input frequency
 * param	multiply_by	: multiplier
 * param	input_freq	: Clock input frequency of the PLL
 * return	Nothing
 * note	Unlike the Chip_Clock_SetupSystemPLLPrec() function, this
 * function does not disable or enable PLL power, wait for PLL lock,
 * or adjust system voltages. These must be done in the application.
 * The function will not alter any source clocks (ie, main systen clock)
 * that may use the PLL, so these should be setup prior to and after
 * exiting the function.
 */
void CLOCK_SetupSystemPLLMult(uint32_t multiply_by, uint32_t input_freq)
{
    uint32_t cco_freq = input_freq * multiply_by;
    uint32_t pdec = 1U;
    uint32_t selr;
    uint32_t seli;
    uint32_t selp;
    uint32_t mdec, ndec;

    uint32_t directo = SYSCON_SYSPLLCTRL_DIRECTO(1);

    while (cco_freq < 75000000U)
    {
        multiply_by <<= 1U; /* double value in each iteration */
        pdec <<= 1U;        /* correspondingly double pdec to cancel effect of double msel */
        cco_freq = input_freq * multiply_by;
    }
    selr = 0U;
    if (multiply_by < 60U)
    {
        seli = (multiply_by & 0x3cU) + 4U;
        selp = (multiply_by >> 1U) + 1U;
    }
    else
    {
        selp = 31U;
        if (multiply_by > 16384U)
        {
            seli = 1U;
        }
        else if (multiply_by > 8192U)
        {
            seli = 2U;
        }
        else if (multiply_by > 2048U)
        {
            seli = 4U;
        }
        else if (multiply_by >= 501U)
        {
            seli = 8U;
        }
        else
        {
            seli = 4U * (1024U / (multiply_by + 9U));
        }
    }

    if (pdec > 1U)
    {
        directo = 0U;     /* use post divider */
        pdec = pdec / 2U; /* Account for minus 1 encoding */
                          /* Translate P value */
        switch (pdec)
        {
            case 1U:
                pdec = 0x62U; /* 1  * 2 */
                break;
            case 2U:
                pdec = 0x42U; /* 2  * 2 */
                break;
            case 4U:
                pdec = 0x02U; /* 4  * 2 */
                break;
            case 8U:
                pdec = 0x0bU; /* 8  * 2 */
                break;
            case 16U:
                pdec = 0x11U; /* 16 * 2 */
                break;
            case 32U:
                pdec = 0x08U; /* 32 * 2 */
                break;
            default:
                pdec = 0x08U;
                break;
        }
    }

    mdec = PLL_SSCG0_MDEC_VAL_SET(pllEncodeM(multiply_by));
    ndec = 0x302U; /* pre divide by 1 (hardcoded) */

    SYSCON->SYSPLLCTRL = SYSCON_SYSPLLCTRL_BANDSEL(1) | directo | SYSCON_SYSPLLCTRL_BYPASSCCODIV2(1) |
                         (selr << SYSCON_SYSPLLCTRL_SELR_SHIFT) | (seli << SYSCON_SYSPLLCTRL_SELI_SHIFT) |
                         (selp << SYSCON_SYSPLLCTRL_SELP_SHIFT);
    SYSCON->SYSPLLPDEC = pdec | (1U << 7U);  /* set Pdec value and assert preq */
    SYSCON->SYSPLLNDEC = ndec | (1U << 10U); /* set Pdec value and assert preq */
    SYSCON->SYSPLLSSCTRL0 =
        (1U << 18U) | (1U << 17U) | mdec; /* select non sscg MDEC value, assert mreq and select mdec value */
}
bool CLOCK_EnableUsbfs0Clock(clock_usb_src_t src, uint32_t freq)
{
    bool ret = true;

    CLOCK_DisableClock(kCLOCK_Usbd0);

    if (kCLOCK_UsbSrcFro == src)
    {
        switch (freq)
        {
            case 96000000U:
                CLOCK_SetClkDiv(kCLOCK_DivUsbClk, 2, false); /*!< Div by 2 to get 48MHz, no divider reset */
                break;
            case 48000000U:
                CLOCK_SetClkDiv(kCLOCK_DivUsbClk, 1, false); /*!< Div by 1 to get 48MHz, no divider reset */
                break;
            default:
                ret = false;
                break;
        }
        /* Turn ON FRO HF and let it adjust TRIM value based on USB SOF */
        SYSCON->FROCTRL = (SYSCON->FROCTRL & ~((0x01U << 15U) | (0xFU << 26U))) | SYSCON_FROCTRL_HSPDCLK_MASK |
                          SYSCON_FROCTRL_USBCLKADJ_MASK;
        /* select FRO 96 or 48 MHz */
        CLOCK_AttachClk(kFRO_HF_to_USB_CLK);
    }
    else
    {
        /*TODO , we only implement FRO as usb clock source*/
        ret = false;
    }

    CLOCK_EnableClock(kCLOCK_Usbd0);

    return ret;
}
