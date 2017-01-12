/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_clock.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Macro definition remap workaround. */
#if (defined(MCG_C2_EREFS_MASK) && !(defined(MCG_C2_EREFS0_MASK)))
#define MCG_C2_EREFS0_MASK MCG_C2_EREFS_MASK
#endif
#if (defined(MCG_C2_HGO_MASK) && !(defined(MCG_C2_HGO0_MASK)))
#define MCG_C2_HGO0_MASK MCG_C2_HGO_MASK
#endif
#if (defined(MCG_C2_RANGE_MASK) && !(defined(MCG_C2_RANGE0_MASK)))
#define MCG_C2_RANGE0_MASK MCG_C2_RANGE_MASK
#endif
#if (defined(MCG_C6_CME_MASK) && !(defined(MCG_C6_CME0_MASK)))
#define MCG_C6_CME0_MASK MCG_C6_CME_MASK
#endif

/* PLL fixed multiplier when there is not PRDIV and VDIV. */
#define PLL_FIXED_MULT (375U)
/* Max frequency of the reference clock used for internal clock trim. */
#define TRIM_REF_CLK_MIN (8000000U)
/* Min frequency of the reference clock used for internal clock trim. */
#define TRIM_REF_CLK_MAX (16000000U)
/* Max trim value of fast internal reference clock. */
#define TRIM_FIRC_MAX (5000000U)
/* Min trim value of fast internal reference clock. */
#define TRIM_FIRC_MIN (3000000U)
/* Max trim value of fast internal reference clock. */
#define TRIM_SIRC_MAX (39063U)
/* Min trim value of fast internal reference clock. */
#define TRIM_SIRC_MIN (31250U)

#define MCG_S_IRCST_VAL ((MCG->S & MCG_S_IRCST_MASK) >> MCG_S_IRCST_SHIFT)
#define MCG_S_CLKST_VAL ((MCG->S & MCG_S_CLKST_MASK) >> MCG_S_CLKST_SHIFT)
#define MCG_S_IREFST_VAL ((MCG->S & MCG_S_IREFST_MASK) >> MCG_S_IREFST_SHIFT)
#define MCG_S_PLLST_VAL ((MCG->S & MCG_S_PLLST_MASK) >> MCG_S_PLLST_SHIFT)
#define MCG_C1_FRDIV_VAL ((MCG->C1 & MCG_C1_FRDIV_MASK) >> MCG_C1_FRDIV_SHIFT)
#define MCG_C2_LP_VAL ((MCG->C2 & MCG_C2_LP_MASK) >> MCG_C2_LP_SHIFT)
#define MCG_C2_RANGE_VAL ((MCG->C2 & MCG_C2_RANGE_MASK) >> MCG_C2_RANGE_SHIFT)
#define MCG_SC_FCRDIV_VAL ((MCG->SC & MCG_SC_FCRDIV_MASK) >> MCG_SC_FCRDIV_SHIFT)
#define MCG_S2_PLLCST_VAL ((MCG->S2 & MCG_S2_PLLCST_MASK) >> MCG_S2_PLLCST_SHIFT)
#define MCG_C7_OSCSEL_VAL ((MCG->C7 & MCG_C7_OSCSEL_MASK) >> MCG_C7_OSCSEL_SHIFT)
#define MCG_C4_DMX32_VAL ((MCG->C4 & MCG_C4_DMX32_MASK) >> MCG_C4_DMX32_SHIFT)
#define MCG_C4_DRST_DRS_VAL ((MCG->C4 & MCG_C4_DRST_DRS_MASK) >> MCG_C4_DRST_DRS_SHIFT)
#define MCG_C7_PLL32KREFSEL_VAL ((MCG->C7 & MCG_C7_PLL32KREFSEL_MASK) >> MCG_C7_PLL32KREFSEL_SHIFT)
#define MCG_C5_PLLREFSEL0_VAL ((MCG->C5 & MCG_C5_PLLREFSEL0_MASK) >> MCG_C5_PLLREFSEL0_SHIFT)
#define MCG_C11_PLLREFSEL1_VAL ((MCG->C11 & MCG_C11_PLLREFSEL1_MASK) >> MCG_C11_PLLREFSEL1_SHIFT)
#define MCG_C11_PRDIV1_VAL ((MCG->C11 & MCG_C11_PRDIV1_MASK) >> MCG_C11_PRDIV1_SHIFT)
#define MCG_C12_VDIV1_VAL ((MCG->C12 & MCG_C12_VDIV1_MASK) >> MCG_C12_VDIV1_SHIFT)
#define MCG_C5_PRDIV0_VAL ((MCG->C5 & MCG_C5_PRDIV0_MASK) >> MCG_C5_PRDIV0_SHIFT)
#define MCG_C6_VDIV0_VAL ((MCG->C6 & MCG_C6_VDIV0_MASK) >> MCG_C6_VDIV0_SHIFT)

#define OSC_MODE_MASK (MCG_C2_EREFS0_MASK | MCG_C2_HGO0_MASK | MCG_C2_RANGE0_MASK)

#define SIM_CLKDIV1_OUTDIV1_VAL ((SIM->CLKDIV1 & SIM_CLKDIV1_OUTDIV1_MASK) >> SIM_CLKDIV1_OUTDIV1_SHIFT)
#define SIM_CLKDIV1_OUTDIV4_VAL ((SIM->CLKDIV1 & SIM_CLKDIV1_OUTDIV4_MASK) >> SIM_CLKDIV1_OUTDIV4_SHIFT)
#define SIM_SOPT1_OSC32KSEL_VAL ((SIM->SOPT1 & SIM_SOPT1_OSC32KSEL_MASK) >> SIM_SOPT1_OSC32KSEL_SHIFT)

/* MCG_S_CLKST definition. */
enum _mcg_clkout_stat
{
    kMCG_ClkOutStatFll, /* FLL.            */
    kMCG_ClkOutStatInt, /* Internal clock. */
    kMCG_ClkOutStatExt, /* External clock. */
    kMCG_ClkOutStatPll  /* PLL.            */
};

/* MCG_S_PLLST definition. */
enum _mcg_pllst
{
    kMCG_PllstFll, /* FLL is used. */
    kMCG_PllstPll  /* PLL is used. */
};

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* Slow internal reference clock frequency. */
static uint32_t s_slowIrcFreq = 32768U;
/* Fast internal reference clock frequency. */
static uint32_t s_fastIrcFreq = 4000000U;

/* External XTAL0 (OSC0) clock frequency. */
uint32_t g_xtal0Freq;
/* External XTAL32K clock frequency. */
uint32_t g_xtal32Freq;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief Get the MCG external reference clock frequency.
 *
 * Get the current MCG external reference clock frequency in Hz. It is
 * the frequency select by MCG_C7[OSCSEL]. This is an internal function.
 *
 * @return MCG external reference clock frequency in Hz.
 */
static uint32_t CLOCK_GetMcgExtClkFreq(void);

/*!
 * @brief Get the MCG FLL external reference clock frequency.
 *
 * Get the current MCG FLL external reference clock frequency in Hz. It is
 * the frequency after by MCG_C1[FRDIV]. This is an internal function.
 *
 * @return MCG FLL external reference clock frequency in Hz.
 */
static uint32_t CLOCK_GetFllExtRefClkFreq(void);

/*!
 * @brief Get the MCG FLL reference clock frequency.
 *
 * Get the current MCG FLL reference clock frequency in Hz. It is
 * the frequency select by MCG_C1[IREFS]. This is an internal function.
 *
 * @return MCG FLL reference clock frequency in Hz.
 */
static uint32_t CLOCK_GetFllRefClkFreq(void);

/*!
 * @brief Get the frequency of clock selected by MCG_C2[IRCS].
 *
 * This clock's two output:
 *  1. MCGOUTCLK when MCG_S[CLKST]=0.
 *  2. MCGIRCLK when MCG_C1[IRCLKEN]=1.
 *
 * @return The frequency in Hz.
 */
static uint32_t CLOCK_GetInternalRefClkSelectFreq(void);

/*!
 * @brief Calculate the RANGE value base on crystal frequency.
 *
 * To setup external crystal oscillator, must set the register bits RANGE
 * base on the crystal frequency. This function returns the RANGE base on the
 * input frequency. This is an internal function.
 *
 * @param freq Crystal frequency in Hz.
 * @return The RANGE value.
 */
static uint8_t CLOCK_GetOscRangeFromFreq(uint32_t freq);

/*!
 * @brief Delay function to wait FLL stable.
 *
 * Delay function to wait FLL stable in FEI mode or FEE mode, should wait at least
 * 1ms. Every time changes FLL setting, should wait this time for FLL stable.
 */
static void CLOCK_FllStableDelay(void);

/*******************************************************************************
 * Code
 ******************************************************************************/

static uint32_t CLOCK_GetMcgExtClkFreq(void)
{
    uint32_t freq;

    switch (MCG_C7_OSCSEL_VAL)
    {
        case 0U:
            /* Please call CLOCK_SetXtal0Freq base on board setting before using OSC0 clock. */
            assert(g_xtal0Freq);
            freq = g_xtal0Freq;
            break;
        case 1U:
            /* Please call CLOCK_SetXtal32Freq base on board setting before using XTAL32K/RTC_CLKIN clock. */
            assert(g_xtal32Freq);
            freq = g_xtal32Freq;
            break;
        default:
            freq = 0U;
            break;
    }

    return freq;
}

static uint32_t CLOCK_GetFllExtRefClkFreq(void)
{
    /* FllExtRef = McgExtRef / FllExtRefDiv */
    uint8_t frdiv;
    uint8_t range;
    uint8_t oscsel;

    uint32_t freq = CLOCK_GetMcgExtClkFreq();

    if (!freq)
    {
        return freq;
    }

    frdiv = MCG_C1_FRDIV_VAL;
    freq >>= frdiv;

    range = MCG_C2_RANGE_VAL;
    oscsel = MCG_C7_OSCSEL_VAL;

    /*
       When should use divider 32, 64, 128, 256, 512, 1024, 1280, 1536.
       1. MCG_C7[OSCSEL] selects IRC48M.
       2. MCG_C7[OSCSEL] selects OSC0 and MCG_C2[RANGE] is not 0.
    */
    if (((0U != range) && (kMCG_OscselOsc == oscsel)))
    {
        switch (frdiv)
        {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
                freq >>= 5u;
                break;
            case 6:
                /* 64*20=1280 */
                freq /= 20u;
                break;
            case 7:
                /* 128*12=1536 */
                freq /= 12u;
                break;
            default:
                freq = 0u;
                break;
        }
    }

    return freq;
}

static uint32_t CLOCK_GetInternalRefClkSelectFreq(void)
{
    if (kMCG_IrcSlow == MCG_S_IRCST_VAL)
    {
        /* Slow internal reference clock selected*/
        return s_slowIrcFreq;
    }
    else
    {
        /* Fast internal reference clock selected*/
        return s_fastIrcFreq >> MCG_SC_FCRDIV_VAL;
    }
}

static uint32_t CLOCK_GetFllRefClkFreq(void)
{
    /* If use external reference clock. */
    if (kMCG_FllSrcExternal == MCG_S_IREFST_VAL)
    {
        return CLOCK_GetFllExtRefClkFreq();
    }
    /* If use internal reference clock. */
    else
    {
        return s_slowIrcFreq;
    }
}

static uint8_t CLOCK_GetOscRangeFromFreq(uint32_t freq)
{
    uint8_t range;

    if (freq <= 39063U)
    {
        range = 0U;
    }
    else if (freq <= 8000000U)
    {
        range = 1U;
    }
    else
    {
        range = 2U;
    }

    return range;
}

static void CLOCK_FllStableDelay(void)
{
    /*
       Should wait at least 1ms. Because in these modes, the core clock is 100MHz
       at most, so this function could obtain the 1ms delay.
     */
    volatile uint32_t i = 30000U;
    while (i--)
    {
        __NOP();
    }
}

uint32_t CLOCK_GetEr32kClkFreq(void)
{
    uint32_t freq;

    switch (SIM_SOPT1_OSC32KSEL_VAL)
    {
        case 0U: /* OSC 32k clock  */
            freq = (g_xtal0Freq == 32768U) ? 32768U : 0U;
            break;
        case 2U: /* RTC 32k clock  */
            /* Please call CLOCK_SetXtal32Freq base on board setting before using XTAL32K/RTC_CLKIN clock. */
            assert(g_xtal32Freq);
            freq = g_xtal32Freq;
            break;
        case 3U: /* LPO clock      */
            freq = LPO_CLK_FREQ;
            break;
        default:
            freq = 0U;
            break;
    }
    return freq;
}

uint32_t CLOCK_GetOsc0ErClkFreq(void)
{
    /* Please call CLOCK_SetXtal0Freq base on board setting before using OSC0 clock. */
    assert(g_xtal0Freq);
    return g_xtal0Freq;
}

uint32_t CLOCK_GetPlatClkFreq(void)
{
    return CLOCK_GetOutClkFreq() / (SIM_CLKDIV1_OUTDIV1_VAL + 1);
}

uint32_t CLOCK_GetFlashClkFreq(void)
{
    uint32_t freq;

    freq = CLOCK_GetOutClkFreq() / (SIM_CLKDIV1_OUTDIV1_VAL + 1);
    freq /= (SIM_CLKDIV1_OUTDIV4_VAL + 1);

    return freq;
}

uint32_t CLOCK_GetBusClkFreq(void)
{
    uint32_t freq;

    freq = CLOCK_GetOutClkFreq() / (SIM_CLKDIV1_OUTDIV1_VAL + 1);
    freq /= (SIM_CLKDIV1_OUTDIV4_VAL + 1);

    return freq;
}

uint32_t CLOCK_GetCoreSysClkFreq(void)
{
    return CLOCK_GetOutClkFreq() / (SIM_CLKDIV1_OUTDIV1_VAL + 1);
}

uint32_t CLOCK_GetFreq(clock_name_t clockName)
{
    uint32_t freq;

    switch (clockName)
    {
        case kCLOCK_CoreSysClk:
        case kCLOCK_PlatClk:
            freq = CLOCK_GetOutClkFreq() / (SIM_CLKDIV1_OUTDIV1_VAL + 1);
            break;
        case kCLOCK_BusClk:
        case kCLOCK_FlashClk:
            freq = CLOCK_GetOutClkFreq() / (SIM_CLKDIV1_OUTDIV1_VAL + 1);
            freq /= (SIM_CLKDIV1_OUTDIV4_VAL + 1);
            break;
        case kCLOCK_Er32kClk:
            freq = CLOCK_GetEr32kClkFreq();
            break;
        case kCLOCK_McgFixedFreqClk:
            freq = CLOCK_GetFixedFreqClkFreq();
            break;
        case kCLOCK_McgInternalRefClk:
            freq = CLOCK_GetInternalRefClkFreq();
            break;
        case kCLOCK_McgFllClk:
            freq = CLOCK_GetFllFreq();
            break;
        case kCLOCK_LpoClk:
            freq = LPO_CLK_FREQ;
            break;
        case kCLOCK_Osc0ErClk:
            /* Please call CLOCK_SetXtal0Freq base on board setting before using OSC0 clock. */
            assert(g_xtal0Freq);
            freq = g_xtal0Freq;
            break;
        default:
            freq = 0U;
            break;
    }

    return freq;
}

void CLOCK_SetSimConfig(sim_clock_config_t const *config)
{
    SIM->CLKDIV1 = config->clkdiv1;
    CLOCK_SetEr32kClock(config->er32kSrc);
}

uint32_t CLOCK_GetOutClkFreq(void)
{
    uint32_t mcgoutclk;
    uint32_t clkst = MCG_S_CLKST_VAL;

    switch (clkst)
    {
        case kMCG_ClkOutStatFll:
            mcgoutclk = CLOCK_GetFllFreq();
            break;
        case kMCG_ClkOutStatInt:
            mcgoutclk = CLOCK_GetInternalRefClkSelectFreq();
            break;
        case kMCG_ClkOutStatExt:
            mcgoutclk = CLOCK_GetMcgExtClkFreq();
            break;
        default:
            mcgoutclk = 0U;
            break;
    }
    return mcgoutclk;
}

uint32_t CLOCK_GetFllFreq(void)
{
    static const uint16_t fllFactorTable[4][2] = {{640, 732}, {1280, 1464}, {1920, 2197}, {2560, 2929}};

    uint8_t drs, dmx32;
    uint32_t freq;

    /* If FLL is not enabled currently, then return 0U. */
    if ((MCG->C2 & MCG_C2_LP_MASK))
    {
        return 0U;
    }

    /* Get FLL reference clock frequency. */
    freq = CLOCK_GetFllRefClkFreq();
    if (!freq)
    {
        return freq;
    }

    drs = MCG_C4_DRST_DRS_VAL;
    dmx32 = MCG_C4_DMX32_VAL;

    return freq * fllFactorTable[drs][dmx32];
}

uint32_t CLOCK_GetInternalRefClkFreq(void)
{
    /* If MCGIRCLK is gated. */
    if (!(MCG->C1 & MCG_C1_IRCLKEN_MASK))
    {
        return 0U;
    }

    return CLOCK_GetInternalRefClkSelectFreq();
}

uint32_t CLOCK_GetFixedFreqClkFreq(void)
{
    uint32_t freq = CLOCK_GetFllRefClkFreq();

    /* MCGFFCLK must be no more than MCGOUTCLK/8. */
    if ((freq) && (freq <= (CLOCK_GetOutClkFreq() / 8U)))
    {
        return freq;
    }
    else
    {
        return 0U;
    }
}

status_t CLOCK_SetExternalRefClkConfig(mcg_oscsel_t oscsel)
{
    bool needDelay;
    uint32_t i;

#if (defined(MCG_CONFIG_CHECK_PARAM) && MCG_CONFIG_CHECK_PARAM)
    /* If change MCG_C7[OSCSEL] and external reference clock is system clock source, return error. */
    if ((MCG_C7_OSCSEL_VAL != oscsel) && (!(MCG->S & MCG_S_IREFST_MASK)))
    {
        return kStatus_MCG_SourceUsed;
    }
#endif /* MCG_CONFIG_CHECK_PARAM */

    if (MCG_C7_OSCSEL_VAL != oscsel)
    {
        /* If change OSCSEL, need to delay, ERR009878. */
        needDelay = true;
    }
    else
    {
        needDelay = false;
    }

    MCG->C7 = (MCG->C7 & ~MCG_C7_OSCSEL_MASK) | MCG_C7_OSCSEL(oscsel);
    if (kMCG_OscselOsc == oscsel)
    {
        if (MCG->C2 & MCG_C2_EREFS_MASK)
        {
            while (!(MCG->S & MCG_S_OSCINIT0_MASK))
            {
            }
        }
    }

    if (needDelay)
    {
        /* ERR009878 Delay at least 50 micro-seconds for external clock change valid. */
        i = 1500U;
        while (i--)
        {
            __NOP();
        }
    }

    return kStatus_Success;
}

status_t CLOCK_SetInternalRefClkConfig(uint8_t enableMode, mcg_irc_mode_t ircs, uint8_t fcrdiv)
{
    uint32_t mcgOutClkState = MCG_S_CLKST_VAL;
    mcg_irc_mode_t curIrcs = (mcg_irc_mode_t)MCG_S_IRCST_VAL;
    uint8_t curFcrdiv = MCG_SC_FCRDIV_VAL;

#if (defined(MCG_CONFIG_CHECK_PARAM) && MCG_CONFIG_CHECK_PARAM)
    /* If MCGIRCLK is used as system clock source. */
    if (kMCG_ClkOutStatInt == mcgOutClkState)
    {
        /* If need to change MCGIRCLK source or driver, return error. */
        if (((kMCG_IrcFast == curIrcs) && (fcrdiv != curFcrdiv)) || (ircs != curIrcs))
        {
            return kStatus_MCG_SourceUsed;
        }
    }
#endif

    /* If need to update the FCRDIV. */
    if (fcrdiv != curFcrdiv)
    {
        /* If fast IRC is in use currently, change to slow IRC. */
        if ((kMCG_IrcFast == curIrcs) && ((mcgOutClkState == kMCG_ClkOutStatInt) || (MCG->C1 & MCG_C1_IRCLKEN_MASK)))
        {
            MCG->C2 = ((MCG->C2 & ~MCG_C2_IRCS_MASK) | (MCG_C2_IRCS(kMCG_IrcSlow)));
            while (MCG_S_IRCST_VAL != kMCG_IrcSlow)
            {
            }
        }
        /* Update FCRDIV. */
        MCG->SC = (MCG->SC & ~(MCG_SC_FCRDIV_MASK | MCG_SC_ATMF_MASK | MCG_SC_LOCS0_MASK)) | MCG_SC_FCRDIV(fcrdiv);
    }

    /* Set internal reference clock selection. */
    MCG->C2 = (MCG->C2 & ~MCG_C2_IRCS_MASK) | (MCG_C2_IRCS(ircs));
    MCG->C1 = (MCG->C1 & ~(MCG_C1_IRCLKEN_MASK | MCG_C1_IREFSTEN_MASK)) | (uint8_t)enableMode;

    /* If MCGIRCLK is used, need to wait for MCG_S_IRCST. */
    if ((mcgOutClkState == kMCG_ClkOutStatInt) || (enableMode & kMCG_IrclkEnable))
    {
        while (MCG_S_IRCST_VAL != ircs)
        {
        }
    }

    return kStatus_Success;
}

void CLOCK_SetOsc0MonitorMode(mcg_monitor_mode_t mode)
{
    /* Clear the previous flag, MCG_SC[LOCS0]. */
    MCG->SC &= ~MCG_SC_ATMF_MASK;

    if (kMCG_MonitorNone == mode)
    {
        MCG->C6 &= ~MCG_C6_CME0_MASK;
    }
    else
    {
        if (kMCG_MonitorInt == mode)
        {
            MCG->C2 &= ~MCG_C2_LOCRE0_MASK;
        }
        else
        {
            MCG->C2 |= MCG_C2_LOCRE0_MASK;
        }
        MCG->C6 |= MCG_C6_CME0_MASK;
    }
}

void CLOCK_SetRtcOscMonitorMode(mcg_monitor_mode_t mode)
{
    uint8_t mcg_c8 = MCG->C8;

    mcg_c8 &= ~(MCG_C8_CME1_MASK | MCG_C8_LOCRE1_MASK);

    if (kMCG_MonitorNone != mode)
    {
        if (kMCG_MonitorReset == mode)
        {
            mcg_c8 |= MCG_C8_LOCRE1_MASK;
        }
        mcg_c8 |= MCG_C8_CME1_MASK;
    }
    MCG->C8 = mcg_c8;
}

uint32_t CLOCK_GetStatusFlags(void)
{
    uint32_t ret = 0U;

    if (MCG->C8 & MCG_C8_LOCS1_MASK)
    {
        ret |= kMCG_RtcOscLostFlag;
    }
    return ret;
}

void CLOCK_ClearStatusFlags(uint32_t mask)
{
    uint8_t reg;

    if (mask & kMCG_RtcOscLostFlag)
    {
        reg = MCG->C8;
        MCG->C8 = reg;
    }
}

void CLOCK_InitOsc0(osc_config_t const *config)
{
    uint8_t range = CLOCK_GetOscRangeFromFreq(config->freq);

    MCG->C2 = ((MCG->C2 & ~OSC_MODE_MASK) | MCG_C2_RANGE(range) | (uint8_t)config->workMode);

    if ((kOSC_ModeExt != config->workMode))
    {
        /* Wait for stable. */
        while (!(MCG->S & MCG_S_OSCINIT0_MASK))
        {
        }
    }
}

void CLOCK_DeinitOsc0(void)
{
    MCG->C2 &= ~OSC_MODE_MASK;
}

status_t CLOCK_TrimInternalRefClk(uint32_t extFreq, uint32_t desireFreq, uint32_t *actualFreq, mcg_atm_select_t atms)
{
    uint32_t multi; /* extFreq / desireFreq */
    uint32_t actv;  /* Auto trim value. */
    uint8_t mcg_sc;

    static const uint32_t trimRange[2][2] = {
        /*     Min           Max      */
        {TRIM_SIRC_MIN, TRIM_SIRC_MAX}, /* Slow IRC. */
        {TRIM_FIRC_MIN, TRIM_FIRC_MAX}  /* Fast IRC. */
    };

    if ((extFreq > TRIM_REF_CLK_MAX) || (extFreq < TRIM_REF_CLK_MIN))
    {
        return kStatus_MCG_AtmBusClockInvalid;
    }

    /* Check desired frequency range. */
    if ((desireFreq < trimRange[atms][0]) || (desireFreq > trimRange[atms][1]))
    {
        return kStatus_MCG_AtmDesiredFreqInvalid;
    }

    /*
       Make sure internal reference clock is not used to generate bus clock.
       Here only need to check (MCG_S_IREFST == 1).
     */
    if (MCG_S_IREFST(kMCG_FllSrcInternal) == (MCG->S & MCG_S_IREFST_MASK))
    {
        return kStatus_MCG_AtmIrcUsed;
    }

    multi = extFreq / desireFreq;
    actv = multi * 21U;

    if (kMCG_AtmSel4m == atms)
    {
        actv *= 128U;
    }

    /* Now begin to start trim. */
    MCG->ATCVL = (uint8_t)actv;
    MCG->ATCVH = (uint8_t)(actv >> 8U);

    mcg_sc = MCG->SC;
    mcg_sc &= ~(MCG_SC_ATMS_MASK | MCG_SC_LOCS0_MASK);
    mcg_sc |= (MCG_SC_ATMF_MASK | MCG_SC_ATMS(atms));
    MCG->SC = (mcg_sc | MCG_SC_ATME_MASK);

    /* Wait for finished. */
    while (MCG->SC & MCG_SC_ATME_MASK)
    {
    }

    /* Error occurs? */
    if (MCG->SC & MCG_SC_ATMF_MASK)
    {
        /* Clear the failed flag. */
        MCG->SC = mcg_sc;
        return kStatus_MCG_AtmHardwareFail;
    }

    *actualFreq = extFreq / multi;

    if (kMCG_AtmSel4m == atms)
    {
        s_fastIrcFreq = *actualFreq;
    }
    else
    {
        s_slowIrcFreq = *actualFreq;
    }

    return kStatus_Success;
}

mcg_mode_t CLOCK_GetMode(void)
{
    mcg_mode_t mode = kMCG_ModeError;
    uint32_t clkst = MCG_S_CLKST_VAL;
    uint32_t irefst = MCG_S_IREFST_VAL;
    uint32_t lp = MCG_C2_LP_VAL;

    /*------------------------------------------------------------------
                           Mode and Registers
    ____________________________________________________________________

      Mode   |   CLKST    |   IREFST   |   PLLST   |      LP
    ____________________________________________________________________

      FEI    |  00(FLL)   |   1(INT)   |   0(FLL)  |      X
    ____________________________________________________________________

      FEE    |  00(FLL)   |   0(EXT)   |   0(FLL)  |      X
    ____________________________________________________________________

      FBE    |  10(EXT)   |   0(EXT)   |   0(FLL)  |   0(NORMAL)
    ____________________________________________________________________

      FBI    |  01(INT)   |   1(INT)   |   0(FLL)  |   0(NORMAL)
    ____________________________________________________________________

      BLPI   |  01(INT)   |   1(INT)   |   0(FLL)  |   1(LOW POWER)
    ____________________________________________________________________

      BLPE   |  10(EXT)   |   0(EXT)   |     X     |   1(LOW POWER)
    ____________________________________________________________________

      PEE    |  11(PLL)   |   0(EXT)   |   1(PLL)  |      X
    ____________________________________________________________________

      PBE    |  10(EXT)   |   0(EXT)   |   1(PLL)  |   O(NORMAL)
    ____________________________________________________________________

      PBI    |  01(INT)   |   1(INT)   |   1(PLL)  |   0(NORMAL)
    ____________________________________________________________________

      PEI    |  11(PLL)   |   1(INT)   |   1(PLL)  |      X
    ____________________________________________________________________

    ----------------------------------------------------------------------*/

    switch (clkst)
    {
        case kMCG_ClkOutStatFll:
            if (kMCG_FllSrcExternal == irefst)
            {
                mode = kMCG_ModeFEE;
            }
            else
            {
                mode = kMCG_ModeFEI;
            }
            break;
        case kMCG_ClkOutStatInt:
            if (lp)
            {
                mode = kMCG_ModeBLPI;
            }
            else
            {
                {
                    mode = kMCG_ModeFBI;
                }
            }
            break;
        case kMCG_ClkOutStatExt:
            if (lp)
            {
                mode = kMCG_ModeBLPE;
            }
            else
            {
                {
                    mode = kMCG_ModeFBE;
                }
            }
            break;
        default:
            break;
    }

    return mode;
}

status_t CLOCK_SetFeiMode(mcg_dmx32_t dmx32, mcg_drs_t drs, void (*fllStableDelay)(void))
{
    uint8_t mcg_c4;
    bool change_drs = false;

#if (defined(MCG_CONFIG_CHECK_PARAM) && MCG_CONFIG_CHECK_PARAM)
    mcg_mode_t mode = CLOCK_GetMode();
    if (!((kMCG_ModeFEI == mode) || (kMCG_ModeFBI == mode) || (kMCG_ModeFBE == mode) || (kMCG_ModeFEE == mode)))
    {
        return kStatus_MCG_ModeUnreachable;
    }
#endif
    mcg_c4 = MCG->C4;

    /*
       Errata: ERR007993
       Workaround: Invert MCG_C4[DMX32] or change MCG_C4[DRST_DRS] before
       reference clock source changes, then reset to previous value after
       reference clock changes.
     */
    if (kMCG_FllSrcExternal == MCG_S_IREFST_VAL)
    {
        change_drs = true;
        /* Change the LSB of DRST_DRS. */
        MCG->C4 ^= (1U << MCG_C4_DRST_DRS_SHIFT);
    }

    /* Set CLKS and IREFS. */
    MCG->C1 =
        ((MCG->C1 & ~(MCG_C1_CLKS_MASK | MCG_C1_IREFS_MASK))) | (MCG_C1_CLKS(kMCG_ClkOutSrcOut)        /* CLKS = 0 */
                                                                 | MCG_C1_IREFS(kMCG_FllSrcInternal)); /* IREFS = 1 */

    /* Wait and check status. */
    while (kMCG_FllSrcInternal != MCG_S_IREFST_VAL)
    {
    }

    /* Errata: ERR007993 */
    if (change_drs)
    {
        MCG->C4 = mcg_c4;
    }

    /* In FEI mode, the MCG_C4[DMX32] is set to 0U. */
    MCG->C4 = (mcg_c4 & ~(MCG_C4_DMX32_MASK | MCG_C4_DRST_DRS_MASK)) | (MCG_C4_DMX32(dmx32) | MCG_C4_DRST_DRS(drs));

    /* Check MCG_S[CLKST] */
    while (kMCG_ClkOutStatFll != MCG_S_CLKST_VAL)
    {
    }

    /* Wait for FLL stable time. */
    if (fllStableDelay)
    {
        fllStableDelay();
    }

    return kStatus_Success;
}

status_t CLOCK_SetFeeMode(uint8_t frdiv, mcg_dmx32_t dmx32, mcg_drs_t drs, void (*fllStableDelay)(void))
{
    uint8_t mcg_c4;
    bool change_drs = false;

#if (defined(MCG_CONFIG_CHECK_PARAM) && MCG_CONFIG_CHECK_PARAM)
    mcg_mode_t mode = CLOCK_GetMode();
    if (!((kMCG_ModeFEE == mode) || (kMCG_ModeFBI == mode) || (kMCG_ModeFBE == mode) || (kMCG_ModeFEI == mode)))
    {
        return kStatus_MCG_ModeUnreachable;
    }
#endif
    mcg_c4 = MCG->C4;

    /*
       Errata: ERR007993
       Workaround: Invert MCG_C4[DMX32] or change MCG_C4[DRST_DRS] before
       reference clock source changes, then reset to previous value after
       reference clock changes.
     */
    if (kMCG_FllSrcInternal == MCG_S_IREFST_VAL)
    {
        change_drs = true;
        /* Change the LSB of DRST_DRS. */
        MCG->C4 ^= (1U << MCG_C4_DRST_DRS_SHIFT);
    }

    /* Set CLKS and IREFS. */
    MCG->C1 = ((MCG->C1 & ~(MCG_C1_CLKS_MASK | MCG_C1_FRDIV_MASK | MCG_C1_IREFS_MASK)) |
               (MCG_C1_CLKS(kMCG_ClkOutSrcOut)         /* CLKS = 0 */
                | MCG_C1_FRDIV(frdiv)                  /* FRDIV */
                | MCG_C1_IREFS(kMCG_FllSrcExternal))); /* IREFS = 0 */

    /* Wait and check status. */
    while (kMCG_FllSrcExternal != MCG_S_IREFST_VAL)
    {
    }

    /* Errata: ERR007993 */
    if (change_drs)
    {
        MCG->C4 = mcg_c4;
    }

    /* Set DRS and DMX32. */
    mcg_c4 = ((mcg_c4 & ~(MCG_C4_DMX32_MASK | MCG_C4_DRST_DRS_MASK)) | (MCG_C4_DMX32(dmx32) | MCG_C4_DRST_DRS(drs)));
    MCG->C4 = mcg_c4;

    /* Wait for DRST_DRS update. */
    while (MCG->C4 != mcg_c4)
    {
    }

    /* Check MCG_S[CLKST] */
    while (kMCG_ClkOutStatFll != MCG_S_CLKST_VAL)
    {
    }

    /* Wait for FLL stable time. */
    if (fllStableDelay)
    {
        fllStableDelay();
    }

    return kStatus_Success;
}

status_t CLOCK_SetFbiMode(mcg_dmx32_t dmx32, mcg_drs_t drs, void (*fllStableDelay)(void))
{
    uint8_t mcg_c4;
    bool change_drs = false;

#if (defined(MCG_CONFIG_CHECK_PARAM) && MCG_CONFIG_CHECK_PARAM)
    mcg_mode_t mode = CLOCK_GetMode();

    if (!((kMCG_ModeFEE == mode) || (kMCG_ModeFBI == mode) || (kMCG_ModeFBE == mode) || (kMCG_ModeFEI == mode) ||
          (kMCG_ModeBLPI == mode)))

    {
        return kStatus_MCG_ModeUnreachable;
    }
#endif

    mcg_c4 = MCG->C4;

    MCG->C2 &= ~MCG_C2_LP_MASK; /* Disable lowpower. */

    /*
       Errata: ERR007993
       Workaround: Invert MCG_C4[DMX32] or change MCG_C4[DRST_DRS] before
       reference clock source changes, then reset to previous value after
       reference clock changes.
     */
    if (kMCG_FllSrcExternal == MCG_S_IREFST_VAL)
    {
        change_drs = true;
        /* Change the LSB of DRST_DRS. */
        MCG->C4 ^= (1U << MCG_C4_DRST_DRS_SHIFT);
    }

    /* Set CLKS and IREFS. */
    MCG->C1 =
        ((MCG->C1 & ~(MCG_C1_CLKS_MASK | MCG_C1_IREFS_MASK)) | (MCG_C1_CLKS(kMCG_ClkOutSrcInternal)    /* CLKS = 1 */
                                                                | MCG_C1_IREFS(kMCG_FllSrcInternal))); /* IREFS = 1 */

    /* Wait and check status. */
    while (kMCG_FllSrcInternal != MCG_S_IREFST_VAL)
    {
    }

    /* Errata: ERR007993 */
    if (change_drs)
    {
        MCG->C4 = mcg_c4;
    }

    while (kMCG_ClkOutStatInt != MCG_S_CLKST_VAL)
    {
    }

    MCG->C4 = (mcg_c4 & ~(MCG_C4_DMX32_MASK | MCG_C4_DRST_DRS_MASK)) | (MCG_C4_DMX32(dmx32) | MCG_C4_DRST_DRS(drs));

    /* Wait for FLL stable time. */
    if (fllStableDelay)
    {
        fllStableDelay();
    }

    return kStatus_Success;
}

status_t CLOCK_SetFbeMode(uint8_t frdiv, mcg_dmx32_t dmx32, mcg_drs_t drs, void (*fllStableDelay)(void))
{
    uint8_t mcg_c4;
    bool change_drs = false;

#if (defined(MCG_CONFIG_CHECK_PARAM) && MCG_CONFIG_CHECK_PARAM)
    mcg_mode_t mode = CLOCK_GetMode();
    if (!((kMCG_ModeFEE == mode) || (kMCG_ModeFBI == mode) || (kMCG_ModeFBE == mode) || (kMCG_ModeFEI == mode) ||
          (kMCG_ModeBLPE == mode)))
    {
        return kStatus_MCG_ModeUnreachable;
    }
#endif

    /* Set LP bit to enable the FLL */
    MCG->C2 &= ~MCG_C2_LP_MASK;

    mcg_c4 = MCG->C4;

    /*
       Errata: ERR007993
       Workaround: Invert MCG_C4[DMX32] or change MCG_C4[DRST_DRS] before
       reference clock source changes, then reset to previous value after
       reference clock changes.
     */
    if (kMCG_FllSrcInternal == MCG_S_IREFST_VAL)
    {
        change_drs = true;
        /* Change the LSB of DRST_DRS. */
        MCG->C4 ^= (1U << MCG_C4_DRST_DRS_SHIFT);
    }

    /* Set CLKS and IREFS. */
    MCG->C1 = ((MCG->C1 & ~(MCG_C1_CLKS_MASK | MCG_C1_FRDIV_MASK | MCG_C1_IREFS_MASK)) |
               (MCG_C1_CLKS(kMCG_ClkOutSrcExternal)    /* CLKS = 2 */
                | MCG_C1_FRDIV(frdiv)                  /* FRDIV = frdiv */
                | MCG_C1_IREFS(kMCG_FllSrcExternal))); /* IREFS = 0 */

    /* Wait for Reference clock Status bit to clear */
    while (kMCG_FllSrcExternal != MCG_S_IREFST_VAL)
    {
    }

    /* Errata: ERR007993 */
    if (change_drs)
    {
        MCG->C4 = mcg_c4;
    }

    /* Set DRST_DRS and DMX32. */
    mcg_c4 = ((mcg_c4 & ~(MCG_C4_DMX32_MASK | MCG_C4_DRST_DRS_MASK)) | (MCG_C4_DMX32(dmx32) | MCG_C4_DRST_DRS(drs)));

    /* Wait for clock status bits to show clock source is ext ref clk */
    while (kMCG_ClkOutStatExt != MCG_S_CLKST_VAL)
    {
    }

    /* Wait for fll stable time. */
    if (fllStableDelay)
    {
        fllStableDelay();
    }

    return kStatus_Success;
}

status_t CLOCK_SetBlpiMode(void)
{
#if (defined(MCG_CONFIG_CHECK_PARAM) && MCG_CONFIG_CHECK_PARAM)
    if (MCG_S_CLKST_VAL != kMCG_ClkOutStatInt)
    {
        return kStatus_MCG_ModeUnreachable;
    }
#endif /* MCG_CONFIG_CHECK_PARAM */

    /* Set LP. */
    MCG->C2 |= MCG_C2_LP_MASK;

    return kStatus_Success;
}

status_t CLOCK_SetBlpeMode(void)
{
#if (defined(MCG_CONFIG_CHECK_PARAM) && MCG_CONFIG_CHECK_PARAM)
    if (MCG_S_CLKST_VAL != kMCG_ClkOutStatExt)
    {
        return kStatus_MCG_ModeUnreachable;
    }
#endif

    /* Set LP bit to enter BLPE mode. */
    MCG->C2 |= MCG_C2_LP_MASK;

    return kStatus_Success;
}

status_t CLOCK_ExternalModeToFbeModeQuick(void)
{
#if (defined(MCG_CONFIG_CHECK_PARAM) && MCG_CONFIG_CHECK_PARAM)
    if (MCG->S & MCG_S_IREFST_MASK)
    {
        return kStatus_MCG_ModeInvalid;
    }
#endif /* MCG_CONFIG_CHECK_PARAM */

    /* Disable low power */
    MCG->C2 &= ~MCG_C2_LP_MASK;

    MCG->C1 = ((MCG->C1 & ~MCG_C1_CLKS_MASK) | MCG_C1_CLKS(kMCG_ClkOutSrcExternal));
    while (MCG_S_CLKST_VAL != kMCG_ClkOutStatExt)
    {
    }

    return kStatus_Success;
}

status_t CLOCK_InternalModeToFbiModeQuick(void)
{
#if (defined(MCG_CONFIG_CHECK_PARAM) && MCG_CONFIG_CHECK_PARAM)
    if (!(MCG->S & MCG_S_IREFST_MASK))
    {
        return kStatus_MCG_ModeInvalid;
    }
#endif

    /* Disable low power */
    MCG->C2 &= ~MCG_C2_LP_MASK;

    MCG->C1 = ((MCG->C1 & ~MCG_C1_CLKS_MASK) | MCG_C1_CLKS(kMCG_ClkOutSrcInternal));
    while (MCG_S_CLKST_VAL != kMCG_ClkOutStatInt)
    {
    }

    return kStatus_Success;
}

status_t CLOCK_BootToFeiMode(mcg_dmx32_t dmx32, mcg_drs_t drs, void (*fllStableDelay)(void))
{
    return CLOCK_SetFeiMode(dmx32, drs, fllStableDelay);
}

status_t CLOCK_BootToFeeMode(
    mcg_oscsel_t oscsel, uint8_t frdiv, mcg_dmx32_t dmx32, mcg_drs_t drs, void (*fllStableDelay)(void))
{
    CLOCK_SetExternalRefClkConfig(oscsel);

    return CLOCK_SetFeeMode(frdiv, dmx32, drs, fllStableDelay);
}

status_t CLOCK_BootToBlpiMode(uint8_t fcrdiv, mcg_irc_mode_t ircs, uint8_t ircEnableMode)
{
    /* If reset mode is FEI mode, set MCGIRCLK and always success. */
    CLOCK_SetInternalRefClkConfig(ircEnableMode, ircs, fcrdiv);

    /* If reset mode is not BLPI, first enter FBI mode. */
    MCG->C1 = (MCG->C1 & ~MCG_C1_CLKS_MASK) | MCG_C1_CLKS(kMCG_ClkOutSrcInternal);
    while (MCG_S_CLKST_VAL != kMCG_ClkOutStatInt)
    {
    }

    /* Enter BLPI mode. */
    MCG->C2 |= MCG_C2_LP_MASK;

    return kStatus_Success;
}

status_t CLOCK_BootToBlpeMode(mcg_oscsel_t oscsel)
{
    CLOCK_SetExternalRefClkConfig(oscsel);

    /* Set to FBE mode. */
    MCG->C1 =
        ((MCG->C1 & ~(MCG_C1_CLKS_MASK | MCG_C1_IREFS_MASK)) | (MCG_C1_CLKS(kMCG_ClkOutSrcExternal)    /* CLKS = 2 */
                                                                | MCG_C1_IREFS(kMCG_FllSrcExternal))); /* IREFS = 0 */

    /* Wait for MCG_S[CLKST] and MCG_S[IREFST]. */
    while ((MCG->S & (MCG_S_IREFST_MASK | MCG_S_CLKST_MASK)) !=
           (MCG_S_IREFST(kMCG_FllSrcExternal) | MCG_S_CLKST(kMCG_ClkOutStatExt)))
    {
    }

    /* In FBE now, start to enter BLPE. */
    MCG->C2 |= MCG_C2_LP_MASK;

    return kStatus_Success;
}

/*
   The transaction matrix. It defines the path for mode switch, the row is for
   current mode and the column is target mode.
   For example, switch from FEI to PEE:
   1. Current mode FEI, next mode is mcgModeMatrix[FEI][PEE] = FBE, so swith to FBE.
   2. Current mode FBE, next mode is mcgModeMatrix[FBE][PEE] = PBE, so swith to PBE.
   3. Current mode PBE, next mode is mcgModeMatrix[PBE][PEE] = PEE, so swith to PEE.
   Thus the MCG mode has changed from FEI to PEE.
 */
static const mcg_mode_t mcgModeMatrix[6][6] = {
    {kMCG_ModeFEI, kMCG_ModeFBI, kMCG_ModeFBI, kMCG_ModeFEE, kMCG_ModeFBE, kMCG_ModeFBE},  /* FEI */
    {kMCG_ModeFEI, kMCG_ModeFBI, kMCG_ModeBLPI, kMCG_ModeFEE, kMCG_ModeFBE, kMCG_ModeFBE}, /* FBI */
    {kMCG_ModeFBI, kMCG_ModeFBI, kMCG_ModeBLPI, kMCG_ModeFBI, kMCG_ModeFBI, kMCG_ModeFBI}, /* BLPI */
    {kMCG_ModeFEI, kMCG_ModeFBI, kMCG_ModeFBI, kMCG_ModeFEE, kMCG_ModeFBE, kMCG_ModeFBE},  /* FEE */
    {kMCG_ModeFEI, kMCG_ModeFBI, kMCG_ModeFBI, kMCG_ModeFEE, kMCG_ModeFBE, kMCG_ModeBLPE}, /* FBE */
    {kMCG_ModeFBE, kMCG_ModeFBE, kMCG_ModeFBE, kMCG_ModeFBE, kMCG_ModeFBE, kMCG_ModeBLPE}, /* BLPE */
    /*      FEI           FBI           BLPI          FEE           FBE           BLPE      */
};

status_t CLOCK_SetMcgConfig(const mcg_config_t *config)
{
    mcg_mode_t next_mode;
    status_t status = kStatus_Success;

    /* If need to change external clock, MCG_C7[OSCSEL]. */
    if (MCG_C7_OSCSEL_VAL != config->oscsel)
    {
        /* If external clock is in use, change to FEI first. */
        if (!(MCG->S & MCG_S_IRCST_MASK))
        {
            CLOCK_ExternalModeToFbeModeQuick();
            CLOCK_SetFeiMode(config->dmx32, config->drs, (void (*)(void))0);
        }

        CLOCK_SetExternalRefClkConfig(config->oscsel);
    }

    /* Re-configure MCGIRCLK, if MCGIRCLK is used as system clock source, then change to FEI/PEI first. */
    if (MCG_S_CLKST_VAL == kMCG_ClkOutStatInt)
    {
        MCG->C2 &= ~MCG_C2_LP_MASK; /* Disable lowpower. */

        {
            CLOCK_SetFeiMode(config->dmx32, config->drs, CLOCK_FllStableDelay);
        }
    }

    /* Configure MCGIRCLK. */
    CLOCK_SetInternalRefClkConfig(config->irclkEnableMode, config->ircs, config->fcrdiv);

    next_mode = CLOCK_GetMode();

    do
    {
        next_mode = mcgModeMatrix[next_mode][config->mcgMode];

        switch (next_mode)
        {
            case kMCG_ModeFEI:
                status = CLOCK_SetFeiMode(config->dmx32, config->drs, CLOCK_FllStableDelay);
                break;
            case kMCG_ModeFEE:
                status = CLOCK_SetFeeMode(config->frdiv, config->dmx32, config->drs, CLOCK_FllStableDelay);
                break;
            case kMCG_ModeFBI:
                status = CLOCK_SetFbiMode(config->dmx32, config->drs, (void (*)(void))0);
                break;
            case kMCG_ModeFBE:
                status = CLOCK_SetFbeMode(config->frdiv, config->dmx32, config->drs, (void (*)(void))0);
                break;
            case kMCG_ModeBLPI:
                status = CLOCK_SetBlpiMode();
                break;
            case kMCG_ModeBLPE:
                status = CLOCK_SetBlpeMode();
                break;
            default:
                break;
        }
        if (kStatus_Success != status)
        {
            return status;
        }
    } while (next_mode != config->mcgMode);

    return kStatus_Success;
}
