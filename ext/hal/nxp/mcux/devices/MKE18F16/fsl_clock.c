/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright (c) 2016 - 2017 , NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_clock.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.clock"
#endif

#define SCG_SIRC_LOW_RANGE_FREQ 2000000U  /* Slow IRC low range clock frequency. */
#define SCG_SIRC_HIGH_RANGE_FREQ 8000000U /* Slow IRC high range clock frequency.   */

#define SCG_FIRC_FREQ0 48000000U /* Fast IRC trimed clock frequency(48MHz). */
#define SCG_FIRC_FREQ1 52000000U /* Fast IRC trimed clock frequency(52MHz). */
#define SCG_FIRC_FREQ2 56000000U /* Fast IRC trimed clock frequency(56MHz). */
#define SCG_FIRC_FREQ3 60000000U /* Fast IRC trimed clock frequency(60MHz). */

/*
 * System PLL base divider value, it is the PLL reference clock divider
 * value when SCG_SPLLCFG[PREDIV]=0.
 */
#define SCG_SPLL_PREDIV_BASE_VALUE 1U

/*
 * System PLL base multiplier value, it is the PLL multiplier value
 * when SCG_SPLLCFG[MULT]=0.
 */
#define SCG_SPLL_MULT_BASE_VALUE 16U

#define SCG_SPLL_PREDIV_MAX_VALUE 7U /* Max value of SCG_SPLLCFG[PREDIV]. */
#define SCG_SPLL_MULT_MAX_VALUE 31U  /* Max value of SCG_SPLLCFG[MULT].   */

/*
 * System PLL reference clock after SCG_SPLLCFG[PREDIV] should be in
 * the range of SCG_SPLL_REF_MIN to SCG_SPLL_REF_MAX.
 */
#define SCG_SPLL_REF_MIN 8000000U
#define SCG_SPLL_REF_MAX 32000000U

#define SCG_CSR_SCS_VAL ((SCG->CSR & SCG_CSR_SCS_MASK) >> SCG_CSR_SCS_SHIFT)
#define SCG_SOSCDIV_SOSCDIV1_VAL ((SCG->SOSCDIV & SCG_SOSCDIV_SOSCDIV1_MASK) >> SCG_SOSCDIV_SOSCDIV1_SHIFT)
#define SCG_SOSCDIV_SOSCDIV2_VAL ((SCG->SOSCDIV & SCG_SOSCDIV_SOSCDIV2_MASK) >> SCG_SOSCDIV_SOSCDIV2_SHIFT)
#define SCG_SIRCDIV_SIRCDIV1_VAL ((SCG->SIRCDIV & SCG_SIRCDIV_SIRCDIV1_MASK) >> SCG_SIRCDIV_SIRCDIV1_SHIFT)
#define SCG_SIRCDIV_SIRCDIV2_VAL ((SCG->SIRCDIV & SCG_SIRCDIV_SIRCDIV2_MASK) >> SCG_SIRCDIV_SIRCDIV2_SHIFT)
#define SCG_FIRCDIV_FIRCDIV1_VAL ((SCG->FIRCDIV & SCG_FIRCDIV_FIRCDIV1_MASK) >> SCG_FIRCDIV_FIRCDIV1_SHIFT)
#define SCG_FIRCDIV_FIRCDIV2_VAL ((SCG->FIRCDIV & SCG_FIRCDIV_FIRCDIV2_MASK) >> SCG_FIRCDIV_FIRCDIV2_SHIFT)

#define SCG_SPLLDIV_SPLLDIV1_VAL ((SCG->SPLLDIV & SCG_SPLLDIV_SPLLDIV1_MASK) >> SCG_SPLLDIV_SPLLDIV1_SHIFT)
#define SCG_SPLLDIV_SPLLDIV2_VAL ((SCG->SPLLDIV & SCG_SPLLDIV_SPLLDIV2_MASK) >> SCG_SPLLDIV_SPLLDIV2_SHIFT)

#define SCG_SIRCCFG_RANGE_VAL ((SCG->SIRCCFG & SCG_SIRCCFG_RANGE_MASK) >> SCG_SIRCCFG_RANGE_SHIFT)
#define SCG_FIRCCFG_RANGE_VAL ((SCG->FIRCCFG & SCG_FIRCCFG_RANGE_MASK) >> SCG_FIRCCFG_RANGE_SHIFT)

#define SCG_SPLLCFG_PREDIV_VAL ((SCG->SPLLCFG & SCG_SPLLCFG_PREDIV_MASK) >> SCG_SPLLCFG_PREDIV_SHIFT)
#define SCG_SPLLCFG_MULT_VAL ((SCG->SPLLCFG & SCG_SPLLCFG_MULT_MASK) >> SCG_SPLLCFG_MULT_SHIFT)

#define PCC_PCS_VAL(reg) (((reg) & PCC_CLKCFG_PCS_MASK) >> PCC_CLKCFG_PCS_SHIFT)

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* External XTAL0 (OSC0) clock frequency. */
volatile uint32_t g_xtal0Freq;
/* External XTAL32K clock frequency. */
volatile uint32_t g_xtal32Freq;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief Get the common System PLL frequency for both RAW SPLL output and SPLL PFD output.
 *
 * The "raw" SPLL output is the clkout divided by postdiv1-2 of SAPLL.
 * The "common" System PLL frequency is the common part for both RAW SPLL and SPLL PFD output.
 * That is the frequency calculated based on the clock source which passes through POSTDIV & MULT.
 * "Common" SPLL Frequency = Divided Reference Frequency * MULT
 *
 * @return  Clock frequency; If the clock is invalid, returns 0.
 */
static uint32_t CLOCK_GetSysPllCommonFreq(void);

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * brief Get the external reference clock frequency (ERCLK).
 *
 * return Clock frequency in Hz.
 */
uint32_t CLOCK_GetErClkFreq(void)
{
    if (SCG->SOSCCSR & SCG_SOSCCSR_SOSCEN_MASK)
    {
        /* Please call CLOCK_SetXtal0Freq base on board setting before using OSC0 clock. */
        assert(g_xtal0Freq);
        return g_xtal0Freq;
    }
    else
    {
        return 0U;
    }
}

/*!
 * brief Get the OSC 32K clock frequency (OSC32KCLK).
 *
 * return Clock frequency in Hz.
 */
uint32_t CLOCK_GetOsc32kClkFreq(void)
{
    assert(g_xtal32Freq);
    return g_xtal32Freq;
}

/*!
 * brief Get the flash clock frequency.
 *
 * return Clock frequency in Hz.
 */
uint32_t CLOCK_GetFlashClkFreq(void)
{
    return CLOCK_GetSysClkFreq(kSCG_SysClkSlow);
}

/*!
 * brief Get the bus clock frequency.
 *
 * return Clock frequency in Hz.
 */
uint32_t CLOCK_GetBusClkFreq(void)
{
    return CLOCK_GetSysClkFreq(kSCG_SysClkBus);
}

/*!
 * brief Get the core clock or system clock frequency.
 *
 * return Clock frequency in Hz.
 */
uint32_t CLOCK_GetCoreSysClkFreq(void)
{
    return CLOCK_GetSysClkFreq(kSCG_SysClkCore);
}

/*!
 * brief Gets the clock frequency for a specific clock name.
 *
 * This function checks the current clock configurations and then calculates
 * the clock frequency for a specific clock name defined in clock_name_t.
 *
 * param clockName Clock names defined in clock_name_t
 * return Clock frequency value in hertz
 */
uint32_t CLOCK_GetFreq(clock_name_t clockName)
{
    uint32_t freq;

    switch (clockName)
    {
        case kCLOCK_CoreSysClk:
            freq = CLOCK_GetSysClkFreq(kSCG_SysClkCore);
            break;
        case kCLOCK_BusClk:
            freq = CLOCK_GetSysClkFreq(kSCG_SysClkBus);
            break;
        case kCLOCK_FlashClk:
            freq = CLOCK_GetSysClkFreq(kSCG_SysClkSlow);
            break;

        case kCLOCK_ScgSysOscClk:
            freq = CLOCK_GetSysOscFreq();
            break;
        case kCLOCK_ScgSircClk:
            freq = CLOCK_GetSircFreq();
            break;
        case kCLOCK_ScgFircClk:
            freq = CLOCK_GetFircFreq();
            break;
        case kCLOCK_ScgSysPllClk:
            freq = CLOCK_GetSysPllFreq();
            break;

        case kCLOCK_ScgSysOscAsyncDiv1Clk:
            freq = CLOCK_GetSysOscAsyncFreq(kSCG_AsyncDiv1Clk);
            break;
        case kCLOCK_ScgSysOscAsyncDiv2Clk:
            freq = CLOCK_GetSysOscAsyncFreq(kSCG_AsyncDiv2Clk);
            break;

        case kCLOCK_ScgSircAsyncDiv1Clk:
            freq = CLOCK_GetSircAsyncFreq(kSCG_AsyncDiv1Clk);
            break;
        case kCLOCK_ScgSircAsyncDiv2Clk:
            freq = CLOCK_GetSircAsyncFreq(kSCG_AsyncDiv2Clk);
            break;

        case kCLOCK_ScgFircAsyncDiv1Clk:
            freq = CLOCK_GetFircAsyncFreq(kSCG_AsyncDiv1Clk);
            break;
        case kCLOCK_ScgFircAsyncDiv2Clk:
            freq = CLOCK_GetFircAsyncFreq(kSCG_AsyncDiv2Clk);
            break;

        case kCLOCK_ScgSysPllAsyncDiv1Clk:
            freq = CLOCK_GetSysPllAsyncFreq(kSCG_AsyncDiv1Clk);
            break;
        case kCLOCK_ScgSysPllAsyncDiv2Clk:
            freq = CLOCK_GetSysPllAsyncFreq(kSCG_AsyncDiv2Clk);
            break;

        case kCLOCK_LpoClk:
            freq = LPO_CLK_FREQ;
            break;
        case kCLOCK_Osc32kClk:
            freq = CLOCK_GetOsc32kClkFreq();
            break;
        case kCLOCK_ErClk:
            freq = CLOCK_GetErClkFreq();
            break;
        default:
            freq = 0U;
            break;
    }
    return freq;
}

/*!
 * brief Gets the clock frequency for a specific IP module.
 *
 * This function gets the IP module clock frequency based on PCC registers. It is
 * only used for the IP modules which could select clock source by PCC[PCS].
 *
 * param name Which peripheral to get, see \ref clock_ip_name_t.
 * return Clock frequency value in hertz
 */
uint32_t CLOCK_GetIpFreq(clock_ip_name_t name)
{
    uint32_t reg = (*(volatile uint32_t *)name);

    scg_async_clk_t asycClk;
    uint32_t freq;

    assert(reg & PCC_CLKCFG_PR_MASK);

    /* FTM uses SCG DIV1 clock, others use SCG DIV2 clock. */
    if ((kCLOCK_Ftm0 == name) || (kCLOCK_Ftm1 == name) || (kCLOCK_Ftm2 == name) || (kCLOCK_Ftm3 == name))
    {
        asycClk = kSCG_AsyncDiv1Clk;
    }
    else
    {
        asycClk = kSCG_AsyncDiv2Clk;
    }

    switch (PCC_PCS_VAL(reg))
    {
        case kCLOCK_IpSrcSysOscAsync:
            freq = CLOCK_GetSysOscAsyncFreq(asycClk);
            break;
        case kCLOCK_IpSrcSircAsync:
            freq = CLOCK_GetSircAsyncFreq(asycClk);
            break;
        case kCLOCK_IpSrcFircAsync:
            freq = CLOCK_GetFircAsyncFreq(asycClk);
            break;
        case kCLOCK_IpSrcSysPllAsync:
            freq = CLOCK_GetSysPllAsyncFreq(asycClk);
            break;
        default:
            freq = 0U;
            break;
    }

    return freq;
}

/*!
 * brief Initializes OSC32.
 *
 * param base OSC32 peripheral base address.
 * param mode OSC32 work mode, see ref osc32_mode_t
 */
void OSC32_Init(OSC32_Type *base, osc32_mode_t mode)
{
    /* Only support one instance now. */
    assert(OSC32 == base);

    CLOCK_EnableClock(kCLOCK_RtcOsc0);

    /* Set work mode. */
    base->CR = (uint8_t)mode;

    if (mode & OSC32_CR_ROSCEREFS_MASK)
    {
        /* If use crystal mode, wait for stable. */
        while (!(base->CR & OSC32_CR_ROSCSTB_MASK))
        {
        }
    }
}

/*!
 * brief Deinitializes OSC32.
 *
 * param base OSC32 peripheral base address.
 */
void OSC32_Deinit(OSC32_Type *base)
{
    /* Only support one instance now. */
    assert(OSC32 == base);

    base->CR = 0U;
    CLOCK_DisableClock(kCLOCK_RtcOsc0);
}

/*!
 * brief Gets the SCG system clock frequency.
 *
 * This function gets the SCG system clock frequency. These clocks are used for
 * core, platform, external, and bus clock domains.
 *
 * param type     Which type of clock to get, core clock or slow clock.
 * return  Clock frequency.
 */
uint32_t CLOCK_GetSysClkFreq(scg_sys_clk_t type)
{
    uint32_t freq;

    scg_sys_clk_config_t sysClkConfig;

    CLOCK_GetCurSysClkConfig(&sysClkConfig);

    switch (sysClkConfig.src)
    {
        case kSCG_SysClkSrcSysOsc:
            freq = CLOCK_GetSysOscFreq();
            break;
        case kSCG_SysClkSrcSirc:
            freq = CLOCK_GetSircFreq();
            break;
        case kSCG_SysClkSrcFirc:
            freq = CLOCK_GetFircFreq();
            break;
        case kSCG_SysClkSrcSysPll:
            freq = CLOCK_GetSysPllFreq();
            break;
        default:
            freq = 0U;
            break;
    }

    freq /= (sysClkConfig.divCore + 1U);

    if (kSCG_SysClkSlow == type)
    {
        freq /= (sysClkConfig.divSlow + 1U);
    }
    else if (kSCG_SysClkBus == type)
    {
        freq /= (sysClkConfig.divBus + 1U);
    }
    else
    {
    }

    return freq;
}

/*!
 * brief Initializes the SCG system OSC.
 *
 * This function enables the SCG system OSC clock according to the
 * configuration.
 *
 * param config   Pointer to the configuration structure.
 * retval kStatus_Success System OSC is initialized.
 * retval kStatus_SCG_Busy System OSC has been enabled and is used by the system clock.
 * retval kStatus_ReadOnly System OSC control register is locked.
 *
 * note This function can't detect whether the system OSC has been enabled and
 * used by an IP.
 */
status_t CLOCK_InitSysOsc(const scg_sosc_config_t *config)
{
    assert(config);
    uint8_t range = 0U; /* SCG_SOSCCFG[RANGE] */
    status_t status;
    uint8_t tmp8;

    /* If crystal oscillator used, need to get RANGE value base on frequency. */
    if (kSCG_SysOscModeExt != config->workMode)
    {
        if ((config->freq >= 32768U) && (config->freq <= 40000U))
        {
            range = 1U;
        }
        else if ((config->freq >= 1000000U) && (config->freq <= 8000000U))
        {
            range = 2U;
        }
        else if ((config->freq >= 8000000U) && (config->freq <= 32000000U))
        {
            range = 3U;
        }
        else
        {
            return kStatus_InvalidArgument;
        }
    }

    /* De-init the SOSC first. */
    status = CLOCK_DeinitSysOsc();

    if (kStatus_Success != status)
    {
        return status;
    }

    /* Now start to set up OSC clock. */
    /* Step 1. Setup dividers. */
    SCG->SOSCDIV = SCG_SOSCDIV_SOSCDIV1(config->div1) | SCG_SOSCDIV_SOSCDIV2(config->div2);

    /* Step 2. Set OSC configuration. */
    SCG->SOSCCFG = config->workMode | SCG_SOSCCFG_RANGE(range);

    /* Step 3. Enable clock. */
    /* SCG->SOSCCSR = SCG_SOSCCSR_SOSCEN_MASK | (config->enableMode); */
    tmp8 = config->enableMode;
    tmp8 |= SCG_SOSCCSR_SOSCEN_MASK;
    SCG->SOSCCSR = tmp8;

    /* Step 4. Wait for OSC clock to be valid. */
    while (!(SCG->SOSCCSR & SCG_SOSCCSR_SOSCVLD_MASK))
    {
    }

    /* Step 5. Enabe monitor. */
    SCG->SOSCCSR |= (uint32_t)config->monitorMode;

    return kStatus_Success;
}

/*!
 * brief De-initializes the SCG system OSC.
 *
 * This function disables the SCG system OSC clock.
 *
 * retval kStatus_Success System OSC is deinitialized.
 * retval kStatus_SCG_Busy System OSC is used by the system clock.
 * retval kStatus_ReadOnly System OSC control register is locked.
 *
 * note This function can't detect whether the system OSC is used by an IP.
 */
status_t CLOCK_DeinitSysOsc(void)
{
    uint32_t reg = SCG->SOSCCSR;

    /* If clock is used by system, return error. */
    if (reg & SCG_SOSCCSR_SOSCSEL_MASK)
    {
        return kStatus_SCG_Busy;
    }

    /* If configure register is locked, return error. */
    if (reg & SCG_SOSCCSR_LK_MASK)
    {
        return kStatus_ReadOnly;
    }

    SCG->SOSCCSR = SCG_SOSCCSR_SOSCERR_MASK;

    return kStatus_Success;
}

/*!
 * brief Gets the SCG system OSC clock frequency (SYSOSC).
 *
 * return  Clock frequency; If the clock is invalid, returns 0.
 */
uint32_t CLOCK_GetSysOscFreq(void)
{
    if (SCG->SOSCCSR & SCG_SOSCCSR_SOSCVLD_MASK) /* System OSC clock is valid. */
    {
        /* Please call CLOCK_SetXtal0Freq base on board setting before using OSC0 clock. */
        assert(g_xtal0Freq);
        return g_xtal0Freq;
    }
    else
    {
        return 0U;
    }
}

/*!
 * brief Gets the SCG asynchronous clock frequency from the system OSC.
 *
 * param type     The asynchronous clock type.
 * return  Clock frequency; If the clock is invalid, returns 0.
 */
uint32_t CLOCK_GetSysOscAsyncFreq(scg_async_clk_t type)
{
    uint32_t oscFreq = CLOCK_GetSysOscFreq();
    uint32_t divider = 0U;

    /* Get divider. */
    if (oscFreq)
    {
        switch (type)
        {
            case kSCG_AsyncDiv2Clk: /* SOSCDIV2_CLK. */
                divider = SCG_SOSCDIV_SOSCDIV2_VAL;
                break;
            case kSCG_AsyncDiv1Clk: /* SOSCDIV1_CLK. */
                divider = SCG_SOSCDIV_SOSCDIV1_VAL;
                break;
            default:
                break;
        }
    }
    if (divider)
    {
        return oscFreq >> (divider - 1U);
    }
    else /* Output disabled. */
    {
        return 0U;
    }
}

/*!
 * brief Initializes the SCG slow IRC clock.
 *
 * This function enables the SCG slow IRC clock according to the
 * configuration.
 *
 * param config   Pointer to the configuration structure.
 * retval kStatus_Success SIRC is initialized.
 * retval kStatus_SCG_Busy SIRC has been enabled and is used by system clock.
 * retval kStatus_ReadOnly SIRC control register is locked.
 *
 * note This function can't detect whether the system OSC has been enabled and
 * used by an IP.
 */
status_t CLOCK_InitSirc(const scg_sirc_config_t *config)
{
    assert(config);

    status_t status;

    /* De-init the SIRC first. */
    status = CLOCK_DeinitSirc();

    if (kStatus_Success != status)
    {
        return status;
    }

    /* Now start to set up SIRC clock. */
    /* Step 1. Setup dividers. */
    SCG->SIRCDIV = SCG_SIRCDIV_SIRCDIV1(config->div1) | SCG_SIRCDIV_SIRCDIV2(config->div2);

    /* Step 2. Set SIRC configuration. */
    SCG->SIRCCFG = SCG_SIRCCFG_RANGE(config->range);

    /* Step 3. Enable clock. */
    SCG->SIRCCSR = SCG_SIRCCSR_SIRCEN_MASK | config->enableMode;

    /* Step 4. Wait for SIRC clock to be valid. */
    while (!(SCG->SIRCCSR & SCG_SIRCCSR_SIRCVLD_MASK))
    {
    }

    return kStatus_Success;
}

/*!
 * brief De-initializes the SCG slow IRC.
 *
 * This function disables the SCG slow IRC.
 *
 * retval kStatus_Success SIRC is deinitialized.
 * retval kStatus_SCG_Busy SIRC is used by system clock.
 * retval kStatus_ReadOnly SIRC control register is locked.
 *
 * note This function can't detect whether the SIRC is used by an IP.
 */
status_t CLOCK_DeinitSirc(void)
{
    uint32_t reg = SCG->SIRCCSR;

    /* If clock is used by system, return error. */
    if (reg & SCG_SIRCCSR_SIRCSEL_MASK)
    {
        return kStatus_SCG_Busy;
    }

    /* If configure register is locked, return error. */
    if (reg & SCG_SIRCCSR_LK_MASK)
    {
        return kStatus_ReadOnly;
    }

    SCG->SIRCCSR = 0U;

    return kStatus_Success;
}

/*!
 * brief Gets the SCG SIRC clock frequency.
 *
 * return  Clock frequency; If the clock is invalid, returns 0.
 */
uint32_t CLOCK_GetSircFreq(void)
{
    static const uint32_t sircFreq[] = {SCG_SIRC_LOW_RANGE_FREQ, SCG_SIRC_HIGH_RANGE_FREQ};

    if (SCG->SIRCCSR & SCG_SIRCCSR_SIRCVLD_MASK) /* SIRC is valid. */
    {
        return sircFreq[SCG_SIRCCFG_RANGE_VAL];
    }
    else
    {
        return 0U;
    }
}

/*!
 * brief Gets the SCG asynchronous clock frequency from the SIRC.
 *
 * param type     The asynchronous clock type.
 * return  Clock frequency; If the clock is invalid, returns 0.
 */
uint32_t CLOCK_GetSircAsyncFreq(scg_async_clk_t type)
{
    uint32_t sircFreq = CLOCK_GetSircFreq();
    uint32_t divider = 0U;

    /* Get divider. */
    if (sircFreq)
    {
        switch (type)
        {
            case kSCG_AsyncDiv2Clk: /* SIRCDIV2_CLK. */
                divider = SCG_SIRCDIV_SIRCDIV2_VAL;
                break;
            case kSCG_AsyncDiv1Clk: /* SIRCDIV2_CLK. */
                divider = SCG_SIRCDIV_SIRCDIV1_VAL;
                break;
            default:
                break;
        }
    }
    if (divider)
    {
        return sircFreq >> (divider - 1U);
    }
    else /* Output disabled. */
    {
        return 0U;
    }
}

/*!
 * brief Initializes the SCG fast IRC clock.
 *
 * This function enables the SCG fast IRC clock according to the configuration.
 *
 * param config   Pointer to the configuration structure.
 * retval kStatus_Success FIRC is initialized.
 * retval kStatus_SCG_Busy FIRC has been enabled and is used by the system clock.
 * retval kStatus_ReadOnly FIRC control register is locked.
 *
 * note This function can't detect whether the FIRC has been enabled and
 * used by an IP.
 */
status_t CLOCK_InitFirc(const scg_firc_config_t *config)
{
    assert(config);

    status_t status;

    /* De-init the FIRC first. */
    status = CLOCK_DeinitFirc();

    if (kStatus_Success != status)
    {
        return status;
    }

    /* Now start to set up FIRC clock. */
    /* Step 1. Setup dividers. */
    SCG->FIRCDIV = SCG_FIRCDIV_FIRCDIV1(config->div1) | SCG_FIRCDIV_FIRCDIV2(config->div2);

    /* Step 2. Set FIRC configuration. */
    SCG->FIRCCFG = SCG_FIRCCFG_RANGE(config->range);

    /* Step 3. Set trimming configuration. */
    if (config->trimConfig)
    {
        SCG->FIRCTCFG =
            SCG_FIRCTCFG_TRIMDIV(config->trimConfig->trimDiv) | SCG_FIRCTCFG_TRIMSRC(config->trimConfig->trimSrc);

        /* TODO: Write FIRCSTAT cause bus error: TKT266932. */
        if (kSCG_FircTrimNonUpdate == config->trimConfig->trimMode)
        {
            SCG->FIRCSTAT = SCG_FIRCSTAT_TRIMCOAR(config->trimConfig->trimCoar) |
                            SCG_FIRCSTAT_TRIMFINE(config->trimConfig->trimFine);
        }

        /* trim mode. */
        SCG->FIRCCSR = config->trimConfig->trimMode;

        if (SCG->FIRCCSR & SCG_FIRCCSR_FIRCERR_MASK)
        {
            return kStatus_Fail;
        }
    }

    /* Step 4. Enable clock. */
    SCG->FIRCCSR |= (SCG_FIRCCSR_FIRCEN_MASK | config->enableMode);

    /* Step 5. Wait for FIRC clock to be valid. */
    while (!(SCG->FIRCCSR & SCG_FIRCCSR_FIRCVLD_MASK))
    {
    }

    return kStatus_Success;
}

/*!
 * brief De-initializes the SCG fast IRC.
 *
 * This function disables the SCG fast IRC.
 *
 * retval kStatus_Success FIRC is deinitialized.
 * retval kStatus_SCG_Busy FIRC is used by the system clock.
 * retval kStatus_ReadOnly FIRC control register is locked.
 *
 * note This function can't detect whether the FIRC is used by an IP.
 */
status_t CLOCK_DeinitFirc(void)
{
    uint32_t reg = SCG->FIRCCSR;

    /* If clock is used by system, return error. */
    if (reg & SCG_FIRCCSR_FIRCSEL_MASK)
    {
        return kStatus_SCG_Busy;
    }

    /* If configure register is locked, return error. */
    if (reg & SCG_FIRCCSR_LK_MASK)
    {
        return kStatus_ReadOnly;
    }

    SCG->FIRCCSR = SCG_FIRCCSR_FIRCERR_MASK;

    return kStatus_Success;
}

/*!
 * brief Gets the SCG FIRC clock frequency.
 *
 * return  Clock frequency; If the clock is invalid, returns 0.
 */
uint32_t CLOCK_GetFircFreq(void)
{
    static const uint32_t fircFreq[] = {
        SCG_FIRC_FREQ0, SCG_FIRC_FREQ1, SCG_FIRC_FREQ2, SCG_FIRC_FREQ3,
    };

    if (SCG->FIRCCSR & SCG_FIRCCSR_FIRCVLD_MASK) /* FIRC is valid. */
    {
        return fircFreq[SCG_FIRCCFG_RANGE_VAL];
    }
    else
    {
        return 0U;
    }
}

/*!
 * brief Gets the SCG asynchronous clock frequency from the FIRC.
 *
 * param type     The asynchronous clock type.
 * return  Clock frequency; If the clock is invalid, returns 0.
 */
uint32_t CLOCK_GetFircAsyncFreq(scg_async_clk_t type)
{
    uint32_t fircFreq = CLOCK_GetFircFreq();
    uint32_t divider = 0U;

    /* Get divider. */
    if (fircFreq)
    {
        switch (type)
        {
            case kSCG_AsyncDiv2Clk: /* FIRCDIV2_CLK. */
                divider = SCG_FIRCDIV_FIRCDIV2_VAL;
                break;
            case kSCG_AsyncDiv1Clk: /* FIRCDIV1_CLK. */
                divider = SCG_FIRCDIV_FIRCDIV1_VAL;
                break;
            default:
                break;
        }
    }
    if (divider)
    {
        return fircFreq >> (divider - 1U);
    }
    else /* Output disabled. */
    {
        return 0U;
    }
}

/*!
 * brief Calculates the MULT and PREDIV for the PLL.
 *
 * This function calculates the proper MULT and PREDIV to generate the desired PLL
 * output frequency with the input reference clock frequency. It returns the closest
 * frequency match that the PLL can generate. The corresponding MULT/PREDIV are returned with
 * parameters. If the desired frequency is not valid, this function returns 0.
 *
 * param refFreq     The input reference clock frequency.
 * param desireFreq  The desired output clock frequency.
 * param mult        The value of MULT.
 * param prediv      The value of PREDIV.
 * return The PLL output frequency with the MULT and PREDIV; If
 * the desired frequency can't be generated, this function returns 0U.
 */
uint32_t CLOCK_GetSysPllMultDiv(uint32_t refFreq, uint32_t desireFreq, uint8_t *mult, uint8_t *prediv)
{
    uint8_t ret_prediv;          /* PREDIV to return */
    uint8_t ret_mult;            /* MULT to return */
    uint8_t prediv_min;          /* Minimal PREDIV value to make reference clock in allowed range. */
    uint8_t prediv_max;          /* Max PREDIV value to make reference clock in allowed range. */
    uint8_t prediv_cur;          /* PREDIV value for iteration. */
    uint8_t mult_cur;            /* MULT value for iteration. */
    uint32_t ret_freq = 0U;      /* Output frequency to return .*/
    uint32_t diff = 0xFFFFFFFFU; /* Difference between desireFreq and return frequency. */
    uint32_t ref_div;            /* Reference frequency after PREDIV. */

    /*
     * Steps:
     * 1. Get allowed prediv with such rules:
     *    1). refFreq / prediv >= SCG_PLL_REF_MIN.
     *    2). refFreq / prediv <= SCG_PLL_REF_MAX.
     * 2. For each allowed prediv, there are two candidate mult values:
     *    1). (desireFreq / (refFreq / prediv)).
     *    2). (desireFreq / (refFreq / prediv)) + 1.
     *    If could get the precise desired frequency, return current prediv and
     *    mult directly. Otherwise choose the one which is closer to desired
     *    frequency.
     */

    /* Reference frequency is out of range. */
    if ((refFreq < SCG_SPLL_REF_MIN) ||
        (refFreq > (SCG_SPLL_REF_MAX * (SCG_SPLL_PREDIV_MAX_VALUE + SCG_SPLL_PREDIV_BASE_VALUE))))
    {
        return 0U;
    }

    /* refFreq/PREDIV must in a range. First get the allowed PREDIV range. */
    prediv_max = refFreq / SCG_SPLL_REF_MIN;
    prediv_min = (refFreq + SCG_SPLL_REF_MAX - 1U) / SCG_SPLL_REF_MAX;

    desireFreq *= 2U;

    /* PREDIV traversal. */
    for (prediv_cur = prediv_max; prediv_cur >= prediv_min; prediv_cur--)
    {
        /*
         * For each PREDIV, the available MULT is (desireFreq*PREDIV/refFreq)
         * or (desireFreq*PREDIV/refFreq + 1U). This function chooses the closer
         * one.
         */
        /* Reference frequency after PREDIV. */
        ref_div = refFreq / prediv_cur;

        mult_cur = desireFreq / ref_div;

        if ((mult_cur < SCG_SPLL_MULT_BASE_VALUE - 1U) ||
            (mult_cur > SCG_SPLL_MULT_BASE_VALUE + SCG_SPLL_MULT_MAX_VALUE))
        {
            /* No MULT is available with this PREDIV. */
            continue;
        }

        ret_freq = mult_cur * ref_div;

        if (mult_cur >= SCG_SPLL_MULT_BASE_VALUE)
        {
            if (ret_freq == desireFreq) /* If desire frequency is got. */
            {
                *prediv = prediv_cur - SCG_SPLL_PREDIV_BASE_VALUE;
                *mult = mult_cur - SCG_SPLL_MULT_BASE_VALUE;
                return ret_freq / 2U;
            }
            if (diff > desireFreq - ret_freq) /* New PRDIV/VDIV is closer. */
            {
                diff = desireFreq - ret_freq;
                ret_prediv = prediv_cur;
                ret_mult = mult_cur;
            }
        }
        mult_cur++;
        if (mult_cur <= (SCG_SPLL_MULT_BASE_VALUE + SCG_SPLL_MULT_MAX_VALUE))
        {
            ret_freq += ref_div;
            if (diff > ret_freq - desireFreq) /* New PRDIV/VDIV is closer. */
            {
                diff = ret_freq - desireFreq;
                ret_prediv = prediv_cur;
                ret_mult = mult_cur;
            }
        }
    }

    if (0xFFFFFFFFU != diff)
    {
        /* PREDIV/MULT found. */
        *prediv = ret_prediv - SCG_SPLL_PREDIV_BASE_VALUE;
        *mult = ret_mult - SCG_SPLL_MULT_BASE_VALUE;
        return ((refFreq / ret_prediv) * ret_mult) / 2;
    }
    else
    {
        return 0U; /* No proper PREDIV/MULT found. */
    }
}

/*!
 * brief Initializes the SCG system PLL.
 *
 * This function enables the SCG system PLL clock according to the
 * configuration. The system PLL can use the system OSC or FIRC as
 * the clock source. Ensure that the source clock is valid before
 * calling this function.
 *
 * Example code for initializing SPLL clock output:
 * code
 * const scg_spll_config_t g_scgSysPllConfig = {.enableMode = kSCG_SysPllEnable,
 *                                            .monitorMode = kSCG_SysPllMonitorDisable,
 *                                            .div1 = kSCG_AsyncClkDivBy1,
 *                                            .div2 = kSCG_AsyncClkDisable,
 *                                            .div3 = kSCG_AsyncClkDivBy2,
 *                                            .src = kSCG_SysPllSrcFirc,
 *                                            .isBypassSelected = false,
 *                                            .isPfdSelected = false, // Configure SPLL PFD as diabled
 *                                            .prediv = 5U,
 *                                            .pfdClkout = kSCG_AuxPllPfd0Clk, // No need to configure pfdClkout; only
 * needed for initialization
 *                                            .mult = 20U,
 *                                            .pllPostdiv1 = kSCG_SysClkDivBy3,
 *                                            .pllPostdiv2 = kSCG_SysClkDivBy4};
 * CLOCK_InitSysPll(&g_scgSysPllConfig);
 * endcode
 *
 * param config   Pointer to the configuration structure.
 * retval kStatus_Success System PLL is initialized.
 * retval kStatus_SCG_Busy System PLL has been enabled and is used by the system clock.
 * retval kStatus_ReadOnly System PLL control register is locked.
 *
 * note This function can't detect whether the system PLL has been enabled and
 * used by an IP.
 */
status_t CLOCK_InitSysPll(const scg_spll_config_t *config)
{
    assert(config);

    status_t status;

    /* De-init the SPLL first. */
    status = CLOCK_DeinitSysPll();

    if (kStatus_Success != status)
    {
        return status;
    }

    /* Now start to set up PLL clock. */
    /* Step 1. Setup dividers. */
    SCG->SPLLDIV = SCG_SPLLDIV_SPLLDIV1(config->div1) | SCG_SPLLDIV_SPLLDIV2(config->div2);

    /* Step 2. Set PLL configuration. */
    SCG->SPLLCFG =
        SCG_SPLLCFG_SOURCE(config->src) | SCG_SPLLCFG_PREDIV(config->prediv) | SCG_SPLLCFG_MULT(config->mult);

    /* Step 3. Enable clock. */
    SCG->SPLLCSR = SCG_SPLLCSR_SPLLEN_MASK | config->enableMode;

    /* Step 4. Wait for PLL clock to be valid. */
    while (!(SCG->SPLLCSR & SCG_SPLLCSR_SPLLVLD_MASK))
    {
    }

    /* Step 5. Enabe monitor. */
    SCG->SPLLCSR |= (uint32_t)config->monitorMode;

    return kStatus_Success;
}

/*!
 * brief De-initializes the SCG system PLL.
 *
 * This function disables the SCG system PLL.
 *
 * retval kStatus_Success system PLL is deinitialized.
 * retval kStatus_SCG_Busy system PLL is used by the system clock.
 * retval kStatus_ReadOnly System PLL control register is locked.
 *
 * note This function can't detect whether the system PLL is used by an IP.
 */
status_t CLOCK_DeinitSysPll(void)
{
    uint32_t reg = SCG->SPLLCSR;

    /* If clock is used by system, return error. */
    if (reg & SCG_SPLLCSR_SPLLSEL_MASK)
    {
        return kStatus_SCG_Busy;
    }

    /* If configure register is locked, return error. */
    if (reg & SCG_SPLLCSR_LK_MASK)
    {
        return kStatus_ReadOnly;
    }

    /* Deinit and clear the error. */
    SCG->SPLLCSR = SCG_SPLLCSR_SPLLERR_MASK;

    return kStatus_Success;
}

static uint32_t CLOCK_GetSysPllCommonFreq(void)
{
    uint32_t freq = 0U;

    if (SCG->SPLLCFG & SCG_SPLLCFG_SOURCE_MASK) /* If use FIRC */
    {
        freq = CLOCK_GetFircFreq();
    }
    else /* Use System OSC. */
    {
        freq = CLOCK_GetSysOscFreq();
    }

    if (freq) /* If source is valid. */
    {
        freq /= (SCG_SPLLCFG_PREDIV_VAL + SCG_SPLL_PREDIV_BASE_VALUE); /* Pre-divider. */
        freq *= (SCG_SPLLCFG_MULT_VAL + SCG_SPLL_MULT_BASE_VALUE);     /* Multiplier. */
    }

    return freq;
}

/*!
 * brief Gets the SCG system PLL clock frequency.
 *
 * return  Clock frequency; If the clock is invalid, returns 0.
 */
uint32_t CLOCK_GetSysPllFreq(void)
{
    uint32_t freq;

    if (SCG->SPLLCSR & SCG_SPLLCSR_SPLLVLD_MASK) /* System PLL is valid. */
    {
        freq = CLOCK_GetSysPllCommonFreq();

        return freq >> 1U;
    }
    else
    {
        return 0U;
    }
}

/*!
 * brief Gets the SCG asynchronous clock frequency from the system PLL.
 *
 * param type     The asynchronous clock type.
 * return  Clock frequency; If the clock is invalid, returns 0.
 */
uint32_t CLOCK_GetSysPllAsyncFreq(scg_async_clk_t type)
{
    uint32_t pllFreq = CLOCK_GetSysPllFreq();
    uint32_t divider = 0U;

    /* Get divider. */
    if (pllFreq)
    {
        switch (type)
        {
            case kSCG_AsyncDiv2Clk: /* SPLLDIV2_CLK. */
                divider = SCG_SPLLDIV_SPLLDIV2_VAL;
                break;
            case kSCG_AsyncDiv1Clk: /* SPLLDIV1_CLK. */
                divider = SCG_SPLLDIV_SPLLDIV1_VAL;
                break;
            default:
                break;
        }
    }
    if (divider)
    {
        return pllFreq >> (divider - 1U);
    }
    else /* Output disabled. */
    {
        return 0U;
    }
}
