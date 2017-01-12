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

#include "fsl_common.h"
#include "fsl_smc.h"
#include "clock_config.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief Clock configuration structure. */
typedef struct _clock_config
{
    mcg_config_t mcgConfig;       /*!< MCG configuration.      */
    sim_clock_config_t simConfig; /*!< SIM configuration.      */
    osc_config_t oscConfig;       /*!< OSC configuration.      */
    uint32_t coreClock;           /*!< core clock frequency.   */
} clock_config_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/
/* System clock frequency. */
extern uint32_t SystemCoreClock;

/* Configuration for enter VLPR mode. Core clock = 4MHz. */
const clock_config_t g_defaultClockConfigVlpr = {
    .mcgConfig =
        {
            .mcgMode = kMCG_ModeBLPI,            /* Work in BLPI mode. */
            .irclkEnableMode = kMCG_IrclkEnable, /* MCGIRCLK enable. */
            .ircs = kMCG_IrcFast,                /* Select IRC4M. */
            .fcrdiv = 0U,                        /* FCRDIV is 0. */

            .frdiv = 5U,
            .drs = kMCG_DrsLow,         /* Low frequency range */
            .dmx32 = kMCG_Dmx32Default, /* DCO has a default range of 25% */
            .oscsel = kMCG_OscselOsc,   /* Select OSC */
        },
    .simConfig =
        {
            .er32kSrc = 0U,         /* ERCLK32K selection, use OSC. */
            .clkdiv1 = 0x00040000U, /* SIM_CLKDIV1. */
        },
    .oscConfig =
        {
            .freq = BOARD_XTAL0_CLK_HZ, /* Feed by RF XTAL_32M */
            .workMode = kOSC_ModeExt,   /* Must work in external source mode. */
        },
    .coreClock = 4000000U, /* Core clock frequency */
};

/* Configuration for enter RUN mode. Core clock = 40MHz. */
const clock_config_t g_defaultClockConfigRun = {
    .mcgConfig =
        {
            .mcgMode = kMCG_ModeFEE,             /* Work in FEE mode. */
            .irclkEnableMode = kMCG_IrclkEnable, /* MCGIRCLK enable. */
            .ircs = kMCG_IrcSlow,                /* Select IRC32k. */
            .fcrdiv = 0U,                        /* FCRDIV is 0. */

            .frdiv = 5U,
            .drs = kMCG_DrsMid,         /* Middle frequency range */
            .dmx32 = kMCG_Dmx32Default, /* DCO has a default range of 25% */
            .oscsel = kMCG_OscselOsc,   /* Select OSC */
        },
    .simConfig =
        {
            .er32kSrc = 0U,         /* ERCLK32K selection, use OSC. */
            .clkdiv1 = 0x00010000U, /* SIM_CLKDIV1. */
        },
    .oscConfig =
        {
            .freq = BOARD_XTAL0_CLK_HZ, /* Feed by RF XTAL_32M */
            .workMode = kOSC_ModeExt,   /* Must work in external source mode. */
        },
    .coreClock = 40000000U, /* Core clock frequency */
};

/*******************************************************************************
 * Code
 ******************************************************************************/
/*
 * How to setup clock using clock driver functions:
 *
 * 1. CLOCK_SetSimSafeDivs, to make sure core clock, bus clock, flexbus clock
 *    and flash clock are in allowed range during clock mode switch.
 *
 * 2. Call CLOCK_Osc0Init to setup OSC clock, if it is used in target mode.
 *
 * 3. Set MCG configuration, MCG includes three parts: FLL clock, PLL clock and
 *    internal reference clock(MCGIRCLK). Follow the steps to setup:
 *
 *    1). Call CLOCK_BootToXxxMode to set MCG to target mode.
 *
 *    2). If target mode is FBI/BLPI/PBI mode, the MCGIRCLK has been configured
 *        correctly. For other modes, need to call CLOCK_SetInternalRefClkConfig
 *        explicitly to setup MCGIRCLK.
 *
 *    3). Don't need to configure FLL explicitly, because if target mode is FLL
 *        mode, then FLL has been configured by the function CLOCK_BootToXxxMode,
 *        if the target mode is not FLL mode, the FLL is disabled.
 *
 *    4). If target mode is PEE/PBE/PEI/PBI mode, then the related PLL has been
 *        setup by CLOCK_BootToXxxMode. In FBE/FBI/FEE/FBE mode, the PLL could
 *        be enabled independently, call CLOCK_EnablePll0 explicitly in this case.
 *
 * 4. Call CLOCK_SetSimConfig to set the clock configuration in SIM.
 */

static void CLOCK_SYS_FllStableDelay(void)
{
    uint32_t i = 30000U;
    while (i--)
    {
        __NOP();
    }
}

void BOARD_BootClockVLPR(void)
{
    /* ERR010224 */
    RSIM->RF_OSC_CTRL |= RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_OVRD_EN_MASK;   /* Prevent XTAL_OUT_EN from generating XTAL_OUT request */

    CLOCK_SetSimSafeDivs();

    CLOCK_BootToBlpiMode(g_defaultClockConfigVlpr.mcgConfig.fcrdiv, g_defaultClockConfigVlpr.mcgConfig.ircs,
                         g_defaultClockConfigVlpr.mcgConfig.irclkEnableMode);

    CLOCK_SetSimConfig(&g_defaultClockConfigVlpr.simConfig);

    SystemCoreClock = g_defaultClockConfigVlpr.coreClock;

    SMC_SetPowerModeProtection(SMC, kSMC_AllowPowerModeAll);
    SMC_SetPowerModeVlpr(SMC);
    while (SMC_GetPowerModeState(SMC) != kSMC_PowerStateVlpr)
    {
    }
}

void BOARD_BootClockRUN(void)
{
    BOARD_RfOscInit();

    CLOCK_SetSimSafeDivs();

    CLOCK_InitOsc0(&g_defaultClockConfigRun.oscConfig);
    CLOCK_SetXtal0Freq(BOARD_XTAL0_CLK_HZ);
    CLOCK_BootToFeeMode(kMCG_OscselOsc, g_defaultClockConfigRun.mcgConfig.frdiv,
                        g_defaultClockConfigRun.mcgConfig.dmx32, g_defaultClockConfigRun.mcgConfig.drs,
                        CLOCK_SYS_FllStableDelay);

    CLOCK_SetInternalRefClkConfig(g_defaultClockConfigRun.mcgConfig.irclkEnableMode,
                                  g_defaultClockConfigRun.mcgConfig.ircs, g_defaultClockConfigRun.mcgConfig.fcrdiv);

    CLOCK_SetSimConfig(&g_defaultClockConfigRun.simConfig);

    SystemCoreClock = g_defaultClockConfigRun.coreClock;
}

void BOARD_RfOscInit(void)
{
    uint32_t temp, tempTrim;
    uint8_t revId;

    /* Obtain REV ID from SIM */
    temp = SIM->SDID;
    revId = (uint8_t)((temp & SIM_SDID_REVID_MASK) >> SIM_SDID_REVID_SHIFT);

    if(0 == revId)
    {
        tempTrim = RSIM->ANA_TRIM;
        RSIM->ANA_TRIM |= RSIM_ANA_TRIM_BB_LDO_XO_TRIM_MASK;            /* Set max trim for BB LDO for XO */
    }/* Workaround for Rev 1.0 XTAL startup and ADC analog diagnostics circuitry */

    /* Turn on clocks for the XCVR */
    /* Enable RF OSC in RSIM and wait for ready */
    temp = RSIM->CONTROL;
    temp &= ~RSIM_CONTROL_RF_OSC_EN_MASK;
    RSIM->CONTROL = temp | RSIM_CONTROL_RF_OSC_EN(1);

    /* ERR010224 */
    RSIM->RF_OSC_CTRL |= RSIM_RF_OSC_CTRL_RADIO_EXT_OSC_OVRD_EN_MASK;   /* Prevent XTAL_OUT_EN from generating XTAL_OUT request */

    while((RSIM->CONTROL & RSIM_CONTROL_RF_OSC_READY_MASK) == 0);       /* Wait for RF_OSC_READY */

    if(0 == revId)
    {
        SIM->SCGC5 |= SIM_SCGC5_PHYDIG_MASK;
        XCVR_TSM->OVRD0 |= XCVR_TSM_OVRD0_BB_LDO_ADCDAC_EN_OVRD_EN_MASK | XCVR_TSM_OVRD0_BB_LDO_ADCDAC_EN_OVRD_MASK; /* Force ADC DAC LDO on to prevent BGAP failure */

        RSIM->ANA_TRIM = tempTrim;                                      /* Reset LDO trim settings */
    }/* Workaround for Rev 1.0 XTAL startup and ADC analog diagnostics circuitry */
}
