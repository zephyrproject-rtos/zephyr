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

#define SCG_SIRC_LOW_RANGE_FREQ 2000000U  /* Slow IRC low range clock frequency. */
#define SCG_SIRC_HIGH_RANGE_FREQ 8000000U /* Slow IRC high range clock frequency.   */

#define SCG_FIRC_FREQ0 48000000U /* Fast IRC trimed clock frequency(48MHz). */
#define SCG_FIRC_FREQ1 52000000U /* Fast IRC trimed clock frequency(52MHz). */
#define SCG_FIRC_FREQ2 56000000U /* Fast IRC trimed clock frequency(56MHz). */
#define SCG_FIRC_FREQ3 60000000U /* Fast IRC trimed clock frequency(60MHz). */

#define SCG_LPFLL_FREQ0 48000000U  /* LPFLL trimed clock frequency(48MHz). */
#define SCG_LPFLL_FREQ1 72000000U  /* LPFLL trimed clock frequency(72MHz). */
#define SCG_LPFLL_FREQ2 96000000U  /* LPFLL trimed clock frequency(96MHz). */
#define SCG_LPFLL_FREQ3 120000000U /* LPFLL trimed clock frequency(120MHz). */

#define SCG_CSR_SCS_VAL ((SCG->CSR & SCG_CSR_SCS_MASK) >> SCG_CSR_SCS_SHIFT)
#define SCG_SOSCDIV_SOSCDIV1_VAL ((SCG->SOSCDIV & SCG_SOSCDIV_SOSCDIV1_MASK) >> SCG_SOSCDIV_SOSCDIV1_SHIFT)
#define SCG_SOSCDIV_SOSCDIV2_VAL ((SCG->SOSCDIV & SCG_SOSCDIV_SOSCDIV2_MASK) >> SCG_SOSCDIV_SOSCDIV2_SHIFT)
#define SCG_SOSCDIV_SOSCDIV3_VAL ((SCG->SOSCDIV & SCG_SOSCDIV_SOSCDIV3_MASK) >> SCG_SOSCDIV_SOSCDIV3_SHIFT)
#define SCG_SIRCDIV_SIRCDIV1_VAL ((SCG->SIRCDIV & SCG_SIRCDIV_SIRCDIV1_MASK) >> SCG_SIRCDIV_SIRCDIV1_SHIFT)
#define SCG_SIRCDIV_SIRCDIV2_VAL ((SCG->SIRCDIV & SCG_SIRCDIV_SIRCDIV2_MASK) >> SCG_SIRCDIV_SIRCDIV2_SHIFT)
#define SCG_SIRCDIV_SIRCDIV3_VAL ((SCG->SIRCDIV & SCG_SIRCDIV_SIRCDIV3_MASK) >> SCG_SIRCDIV_SIRCDIV3_SHIFT)
#define SCG_FIRCDIV_FIRCDIV1_VAL ((SCG->FIRCDIV & SCG_FIRCDIV_FIRCDIV1_MASK) >> SCG_FIRCDIV_FIRCDIV1_SHIFT)
#define SCG_FIRCDIV_FIRCDIV2_VAL ((SCG->FIRCDIV & SCG_FIRCDIV_FIRCDIV2_MASK) >> SCG_FIRCDIV_FIRCDIV2_SHIFT)
#define SCG_FIRCDIV_FIRCDIV3_VAL ((SCG->FIRCDIV & SCG_FIRCDIV_FIRCDIV3_MASK) >> SCG_FIRCDIV_FIRCDIV3_SHIFT)

#define SCG_LPFLLDIV_LPFLLDIV1_VAL ((SCG->LPFLLDIV & SCG_LPFLLDIV_LPFLLDIV1_MASK) >> SCG_LPFLLDIV_LPFLLDIV1_SHIFT)
#define SCG_LPFLLDIV_LPFLLDIV2_VAL ((SCG->LPFLLDIV & SCG_LPFLLDIV_LPFLLDIV2_MASK) >> SCG_LPFLLDIV_LPFLLDIV2_SHIFT)
#define SCG_LPFLLDIV_LPFLLDIV3_VAL ((SCG->LPFLLDIV & SCG_LPFLLDIV_LPFLLDIV3_MASK) >> SCG_LPFLLDIV_LPFLLDIV3_SHIFT)

#define SCG_SIRCCFG_RANGE_VAL ((SCG->SIRCCFG & SCG_SIRCCFG_RANGE_MASK) >> SCG_SIRCCFG_RANGE_SHIFT)
#define SCG_FIRCCFG_RANGE_VAL ((SCG->FIRCCFG & SCG_FIRCCFG_RANGE_MASK) >> SCG_FIRCCFG_RANGE_SHIFT)

#define SCG_LPFLLCFG_FSEL_VAL ((SCG->LPFLLCFG & SCG_LPFLLCFG_FSEL_MASK) >> SCG_LPFLLCFG_FSEL_SHIFT)

/* Get the value of each field in PCC register. */
#define PCC_PCS_VAL(reg) ((reg & PCC_CLKCFG_PCS_MASK) >> PCC_CLKCFG_PCS_SHIFT)
#define PCC_FRAC_VAL(reg) ((reg & PCC_CLKCFG_FRAC_MASK) >> PCC_CLKCFG_FRAC_SHIFT)
#define PCC_PCD_VAL(reg) ((reg & PCC_CLKCFG_PCD_MASK) >> PCC_CLKCFG_PCD_SHIFT)

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* External XTAL0 (OSC0) clock frequency. */
uint32_t g_xtal0Freq;
/* External XTAL32K clock frequency. */
uint32_t g_xtal32Freq;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

uint32_t CLOCK_GetOsc32kClkFreq(void)
{
    assert(g_xtal32Freq);
    return g_xtal32Freq;
}

uint32_t CLOCK_GetFlashClkFreq(void)
{
    return CLOCK_GetSysClkFreq(kSCG_SysClkSlow);
}

uint32_t CLOCK_GetBusClkFreq(void)
{
    return CLOCK_GetSysClkFreq(kSCG_SysClkSlow);
}

uint32_t CLOCK_GetPlatClkFreq(void)
{
    return CLOCK_GetSysClkFreq(kSCG_SysClkCore);
}

uint32_t CLOCK_GetCoreSysClkFreq(void)
{
    return CLOCK_GetSysClkFreq(kSCG_SysClkCore);
}

uint32_t CLOCK_GetExtClkFreq(void)
{
    return CLOCK_GetSysClkFreq(kSCG_SysClkExt);
}

uint32_t CLOCK_GetFreq(clock_name_t clockName)
{
    uint32_t freq;

    switch (clockName)
    {
        /* System layer clock. */
        case kCLOCK_CoreSysClk:
        case kCLOCK_PlatClk:
            freq = CLOCK_GetSysClkFreq(kSCG_SysClkCore);
            break;
        case kCLOCK_BusClk:
            freq = CLOCK_GetSysClkFreq(kSCG_SysClkBus);
            break;
        case kCLOCK_FlashClk:
            freq = CLOCK_GetSysClkFreq(kSCG_SysClkSlow);
            break;
        case kCLOCK_ExtClk:
            freq = CLOCK_GetSysClkFreq(kSCG_SysClkExt);
            break;
        /* Original clock source. */
        case kCLOCK_ScgSysOscClk:
            freq = CLOCK_GetSysOscFreq();
            break;
        case kCLOCK_ScgSircClk:
            freq = CLOCK_GetSircFreq();
            break;
        case kCLOCK_ScgFircClk:
            freq = CLOCK_GetFircFreq();
            break;
        case kCLOCK_ScgLpFllClk:
            freq = CLOCK_GetLpFllFreq();
            break;

        /* SOSC div clock. */
        case kCLOCK_ScgSysOscAsyncDiv1Clk:
            freq = CLOCK_GetSysOscAsyncFreq(kSCG_AsyncDiv1Clk);
            break;
        case kCLOCK_ScgSysOscAsyncDiv2Clk:
            freq = CLOCK_GetSysOscAsyncFreq(kSCG_AsyncDiv2Clk);
            break;
        case kCLOCK_ScgSysOscAsyncDiv3Clk:
            freq = CLOCK_GetSysOscAsyncFreq(kSCG_AsyncDiv3Clk);
            break;

        /* SIRC div clock. */
        case kCLOCK_ScgSircAsyncDiv1Clk:
            freq = CLOCK_GetSircAsyncFreq(kSCG_AsyncDiv1Clk);
            break;
        case kCLOCK_ScgSircAsyncDiv2Clk:
            freq = CLOCK_GetSircAsyncFreq(kSCG_AsyncDiv2Clk);
            break;
        case kCLOCK_ScgSircAsyncDiv3Clk:
            freq = CLOCK_GetSircAsyncFreq(kSCG_AsyncDiv3Clk);
            break;

        /* FIRC div clock. */
        case kCLOCK_ScgFircAsyncDiv1Clk:
            freq = CLOCK_GetFircAsyncFreq(kSCG_AsyncDiv1Clk);
            break;
        case kCLOCK_ScgFircAsyncDiv2Clk:
            freq = CLOCK_GetFircAsyncFreq(kSCG_AsyncDiv2Clk);
            break;
        case kCLOCK_ScgFircAsyncDiv3Clk:
            freq = CLOCK_GetFircAsyncFreq(kSCG_AsyncDiv3Clk);
            break;

        /* LPFLL div clock. */
        case kCLOCK_ScgSysLpFllAsyncDiv1Clk:
            freq = CLOCK_GetLpFllAsyncFreq(kSCG_AsyncDiv1Clk);
            break;
        case kCLOCK_ScgSysLpFllAsyncDiv2Clk:
            freq = CLOCK_GetLpFllAsyncFreq(kSCG_AsyncDiv2Clk);
            break;
        case kCLOCK_ScgSysLpFllAsyncDiv3Clk:
            freq = CLOCK_GetLpFllAsyncFreq(kSCG_AsyncDiv3Clk);
            break;

        /* Other clocks. */
        case kCLOCK_LpoClk:
            freq = CLOCK_GetLpoClkFreq();
            break;
        case kCLOCK_Osc32kClk:
            freq = CLOCK_GetOsc32kClkFreq();
            break;
        default:
            freq = 0U;
            break;
    }
    return freq;
}

uint32_t CLOCK_GetIpFreq(clock_ip_name_t name)
{
    uint32_t reg = (*(volatile uint32_t *)name);

    scg_async_clk_t asycClk;
    uint32_t freq;

    assert(reg & PCC_CLKCFG_PR_MASK);

    switch (name)
    {
        case kCLOCK_Lpit0:
        case kCLOCK_Lpit1:
            asycClk = kSCG_AsyncDiv3Clk;
            break;
        case kCLOCK_Sdhc0:
        case kCLOCK_Usb0:
            asycClk = kSCG_AsyncDiv1Clk;
            break;
        default:
            asycClk = kSCG_AsyncDiv2Clk;
            break;
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
        case kCLOCK_IpSrcLpFllAsync:
            freq = CLOCK_GetLpFllAsyncFreq(asycClk);
            break;
        default: /* kCLOCK_IpSrcNoneOrExt. */
            freq = 0U;
            break;
    }

    if (0U != (reg & (PCC_CLKCFG_PCD_MASK | PCC_CLKCFG_FRAC_MASK)))
    {
        return freq * (PCC_FRAC_VAL(reg) + 1U) / (PCC_PCD_VAL(reg) + 1U);
    }
    else
    {
        return freq;
    }
}

bool CLOCK_EnableUsbfs0Clock(clock_usb_src_t src, uint32_t freq)
{
    bool ret = true;

    CLOCK_SetIpSrc(kCLOCK_Usb0, kCLOCK_IpSrcFircAsync);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable clock gate. */
    CLOCK_EnableClock(kCLOCK_Usb0);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    USBVREG->CTRL |= USBVREG_CTRL_EN_MASK;
    USB0->CONTROL &= ~USB_CONTROL_DPPULLUPNONOTG_MASK;

    if (kCLOCK_UsbSrcIrc48M == src)
    {
        USB0->CLK_RECOVER_IRC_EN = 0x03U;
        USB0->CLK_RECOVER_CTRL |= USB_CLK_RECOVER_CTRL_CLOCK_RECOVER_EN_MASK;
        USB0->CLK_RECOVER_INT_EN = 0x00U;
    }
    return ret;
}

uint32_t CLOCK_GetSysClkFreq(scg_sys_clk_t type)
{
    uint32_t freq;

    scg_sys_clk_config_t sysClkConfig;

    CLOCK_GetCurSysClkConfig(&sysClkConfig); /* Get the main clock for SoC platform. */

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
        case kSCG_SysClkSrcRosc:
            freq = CLOCK_GetRtcOscFreq();
            break;
        case kSCG_SysClkSrcLpFll:
            freq = CLOCK_GetLpFllFreq();
            break;
        default:
            freq = 0U;
            break;
    }

    freq /= (sysClkConfig.divCore + 1U); /* divided by the DIVCORE firstly. */

    if (kSCG_SysClkSlow == type)
    {
        freq /= (sysClkConfig.divSlow + 1U);
    }
    else if (kSCG_SysClkBus == type)
    {
        freq /= (sysClkConfig.divBus + 1U);
    }
    else if (kSCG_SysClkExt == type)
    {
        freq /= (sysClkConfig.divExt + 1U);
    }
    else
    {
    }

    return freq;
}

status_t CLOCK_InitSysOsc(const scg_sosc_config_t *config)
{
    assert(config);
    status_t status;
    uint8_t tmp8;

    /* De-init the SOSC first. */
    status = CLOCK_DeinitSysOsc();

    if (kStatus_Success != status)
    {
        return status;
    }

    /* Now start to set up OSC clock. */
    /* Step 1. Setup dividers. */
    SCG->SOSCDIV =
        SCG_SOSCDIV_SOSCDIV1(config->div1) | SCG_SOSCDIV_SOSCDIV2(config->div2) | SCG_SOSCDIV_SOSCDIV3(config->div3);

    /* Step 2. Set OSC configuration. */

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

uint32_t CLOCK_GetSysOscAsyncFreq(scg_async_clk_t type)
{
    uint32_t oscFreq = CLOCK_GetSysOscFreq();
    uint32_t divider = 0U;

    /* Get divider. */
    if (oscFreq)
    {
        switch (type)
        {
            case kSCG_AsyncDiv3Clk: /* SOSCDIV3_CLK. */
                divider = SCG_SOSCDIV_SOSCDIV3_VAL;
                break;
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
    SCG->SIRCDIV =
        SCG_SIRCDIV_SIRCDIV1(config->div1) | SCG_SIRCDIV_SIRCDIV2(config->div2) | SCG_SIRCDIV_SIRCDIV3(config->div3);

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

uint32_t CLOCK_GetSircAsyncFreq(scg_async_clk_t type)
{
    uint32_t sircFreq = CLOCK_GetSircFreq();
    uint32_t divider = 0U;

    /* Get divider. */
    if (sircFreq)
    {
        switch (type)
        {
            case kSCG_AsyncDiv3Clk: /* SIRCDIV3_CLK. */
                divider = SCG_SIRCDIV_SIRCDIV3_VAL;
                break;
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
    SCG->FIRCDIV =
        SCG_FIRCDIV_FIRCDIV1(config->div1) | SCG_FIRCDIV_FIRCDIV2(config->div2) | SCG_FIRCDIV_FIRCDIV3(config->div3);

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
    SCG->FIRCCSR |= (SCG_FIRCCSR_FIRCEN_MASK | SCG_FIRCCSR_FIRCTREN_MASK | config->enableMode);

    /* Step 5. Wait for FIRC clock to be valid. */
    while (!(SCG->FIRCCSR & SCG_FIRCCSR_FIRCVLD_MASK))
    {
    }

    return kStatus_Success;
}

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

uint32_t CLOCK_GetFircAsyncFreq(scg_async_clk_t type)
{
    uint32_t fircFreq = CLOCK_GetFircFreq();
    uint32_t divider = 0U;

    /* Get divider. */
    if (fircFreq)
    {
        switch (type)
        {
            case kSCG_AsyncDiv3Clk: /* FIRCDIV3_CLK. */
                divider = SCG_FIRCDIV_FIRCDIV3_VAL;
                break;
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

uint32_t CLOCK_GetRtcOscFreq(void)
{
    if (SCG->ROSCCSR & SCG_ROSCCSR_ROSCVLD_MASK) /* RTC OSC clock is valid. */
    {
        /* Please call CLOCK_SetXtal32Freq base on board setting before using RTC OSC clock. */
        assert(g_xtal32Freq);
        return g_xtal32Freq;
    }
    else
    {
        return 0U;
    }
}

status_t CLOCK_InitLpFll(const scg_lpfll_config_t *config)
{
    assert(config);

    status_t status;

    /* De-init the LPFLL first. */
    status = CLOCK_DeinitLpFll();

    if (kStatus_Success != status)
    {
        return status;
    }

    /* Now start to set up LPFLL clock. */
    /* Step 1. Setup dividers. */
    SCG->LPFLLDIV = SCG_LPFLLDIV_LPFLLDIV1(config->div1) | SCG_LPFLLDIV_LPFLLDIV2(config->div2) |
                    SCG_LPFLLDIV_LPFLLDIV3(config->div3);

    /* Step 2. Set LPFLL configuration. */
    SCG->LPFLLCFG = SCG_LPFLLCFG_FSEL(config->range);

    /* Step 3. Set trimming configuration. */
    if (config->trimConfig)
    {
        SCG->LPFLLTCFG = SCG_LPFLLTCFG_TRIMDIV(config->trimConfig->trimDiv) |
                         SCG_LPFLLTCFG_TRIMSRC(config->trimConfig->trimSrc) |
                         SCG_LPFLLTCFG_LOCKW2LSB(config->trimConfig->lockMode);

        if (kSCG_LpFllTrimNonUpdate == config->trimConfig->trimMode)
        {
            SCG->LPFLLSTAT = config->trimConfig->trimValue;
        }

        /* Trim mode. */
        SCG->LPFLLCSR = config->trimConfig->trimMode;

        if (SCG->LPFLLCSR & SCG_LPFLLCSR_LPFLLERR_MASK)
        {
            return kStatus_Fail;
        }
    }

    /* Step 4. Enable clock. */
    SCG->LPFLLCSR |= (SCG_LPFLLCSR_LPFLLEN_MASK | config->enableMode);

    /* Step 5. Wait for LPFLL clock to be valid. */
    while (!(SCG->LPFLLCSR & SCG_LPFLLCSR_LPFLLVLD_MASK))
    {
    }

    /* Step 6. Wait for LPFLL trim lock. */
    if ((config->trimConfig) && (kSCG_LpFllTrimUpdate == config->trimConfig->trimMode))
    {
        while (!(SCG->LPFLLCSR & SCG_LPFLLCSR_LPFLLTRMLOCK_MASK))
        {
        }
    }

    return kStatus_Success;
}

status_t CLOCK_DeinitLpFll(void)
{
    uint32_t reg = SCG->LPFLLCSR;

    /* If clock is used by system, return error. */
    if (reg & SCG_LPFLLCSR_LPFLLSEL_MASK)
    {
        return kStatus_SCG_Busy;
    }

    /* If configure register is locked, return error. */
    if (reg & SCG_LPFLLCSR_LK_MASK)
    {
        return kStatus_ReadOnly;
    }

    SCG->LPFLLCSR = SCG_LPFLLCSR_LPFLLERR_MASK;

    return kStatus_Success;
}

uint32_t CLOCK_GetLpFllFreq(void)
{
    static const uint32_t lpfllFreq[] = {
        SCG_LPFLL_FREQ0, SCG_LPFLL_FREQ1, SCG_LPFLL_FREQ2, SCG_LPFLL_FREQ3,
    };

    if (SCG->LPFLLCSR & SCG_LPFLLCSR_LPFLLVLD_MASK) /* LPFLL is valid. */
    {
        return lpfllFreq[SCG_LPFLLCFG_FSEL_VAL];
    }
    else
    {
        return 0U;
    }
}

uint32_t CLOCK_GetLpFllAsyncFreq(scg_async_clk_t type)
{
    uint32_t lpfllFreq = CLOCK_GetLpFllFreq();
    uint32_t divider = 0U;

    /* Get divider. */
    if (lpfllFreq)
    {
        switch (type)
        {
            case kSCG_AsyncDiv2Clk: /* LPFLLDIV2_CLK. */
                divider = SCG_LPFLLDIV_LPFLLDIV2_VAL;
                break;
            case kSCG_AsyncDiv1Clk: /* LPFLLDIV1_CLK. */
                divider = SCG_LPFLLDIV_LPFLLDIV1_VAL;
                break;
            default:
                break;
        }
    }
    if (divider)
    {
        return lpfllFreq >> (divider - 1U);
    }
    else /* Output disabled. */
    {
        return 0U;
    }
}
