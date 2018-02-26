/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
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
 * o Neither the name of the copyright holder nor the names of its
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

#include "fsl_dcdc.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get instance number for DCDC module.
 *
 * @param base DCDC peripheral base address
 */
static uint32_t DCDC_GetInstance(DCDC_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to DCDC bases for each instance. */
static DCDC_Type *const s_dcdcBases[] = DCDC_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to DCDC clocks for each instance. */
static const clock_ip_name_t s_dcdcClocks[] = DCDC_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t DCDC_GetInstance(DCDC_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_dcdcBases); instance++)
    {
        if (s_dcdcBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_dcdcBases));

    return instance;
}

void DCDC_Init(DCDC_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the clock. */
    CLOCK_EnableClock(s_dcdcClocks[DCDC_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void DCDC_Deinit(DCDC_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable the clock. */
    CLOCK_DisableClock(s_dcdcClocks[DCDC_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

uint32_t DCDC_GetStatusFlags(DCDC_Type *base)
{
    uint32_t tmp32 = 0U;

    /* kDCDC_LockedOKStatus. */
    if (0U != (DCDC_REG0_DCDC_STS_DC_OK_MASK & base->REG0))
    {
        tmp32 |= kDCDC_LockedOKStatus;
    }
    /* kDCDC_PSwitchStatus. */
    if (0U != (DCDC_REG0_PSWITCH_STATUS_MASK & base->REG0))
    {
        tmp32 |= kDCDC_PSwitchStatus;
    }
    /* kDCDC_PSwitchInterruptStatus. */
    if (0U != (DCDC_REG6_PSWITCH_INT_STS_MASK & base->REG6))
    {
        tmp32 |= kDCDC_PSwitchInterruptStatus;
    }

    return tmp32;
}

void DCDC_ClearStatusFlags(DCDC_Type *base, uint32_t mask) /* Clear flags indicated by mask. */
{
    if (0U != (kDCDC_PSwitchInterruptStatus & mask))
    {
        /* Write 1 to clear interrupt. Set to 0 after clear. */
        base->REG6 |= DCDC_REG6_PSWITCH_INT_CLEAR_MASK;
        base->REG6 &= ~DCDC_REG6_PSWITCH_INT_CLEAR_MASK;
    }
}

void DCDC_SetPSwitchInterruptConfig(DCDC_Type *base, uint32_t mask)
{
    assert(0U == (mask & ~(DCDC_REG6_PSWITCH_INT_RISE_EN_MASK | DCDC_REG6_PSWITCH_INT_FALL_EN_MASK)));

    uint32_t tmp32 = base->REG6 & ~(DCDC_REG6_PSWITCH_INT_RISE_EN_MASK | DCDC_REG6_PSWITCH_INT_FALL_EN_MASK);

    tmp32 |= mask;
    base->REG6 = tmp32;
}

void DCDC_GetDefaultLowPowerConfig(dcdc_low_power_config_t *config)
{
    assert(NULL != config);

    config->workModeInVLPRW = kDCDC_WorkInPulsedMode;
    config->workModeInVLPS = kDCDC_WorkInPulsedMode;
    config->enableHysteresisVoltageSense = true;
    config->enableAdjustHystereticValueSense = false;
    config->enableHystersisComparator = true;
    config->enableAdjustHystereticValueComparator = false;
    config->hystereticUpperThresholdValue = kDCDC_HystereticThresholdOffset75mV;
    config->hystereticLowerThresholdValue = kDCDC_HystereticThresholdOffset0mV;
    config->enableDiffComparators = false;
}

void DCDC_SetLowPowerConfig(DCDC_Type *base, const dcdc_low_power_config_t *config)
{
    uint32_t tmp32;

    tmp32 =
        base->REG0 &
        ~(DCDC_REG0_VLPR_VLPW_CONFIG_DCDC_HP_MASK | DCDC_REG0_VLPS_CONFIG_DCDC_HP_MASK |
          DCDC_REG0_OFFSET_RSNS_LP_DISABLE_MASK | DCDC_REG0_OFFSET_RSNS_LP_ADJ_MASK |
          DCDC_REG0_HYST_LP_CMP_DISABLE_MASK | DCDC_REG0_HYST_LP_COMP_ADJ_MASK | DCDC_REG0_DCDC_LP_STATE_HYS_H_MASK |
          DCDC_REG0_DCDC_LP_STATE_HYS_L_MASK | DCDC_REG0_DCDC_LP_DF_CMP_ENABLE_MASK);
    if (kDCDC_WorkInContinuousMode == config->workModeInVLPRW)
    {
        tmp32 |= DCDC_REG0_VLPR_VLPW_CONFIG_DCDC_HP_MASK;
    }
    if (kDCDC_WorkInContinuousMode == config->workModeInVLPS)
    {
        tmp32 |= DCDC_REG0_VLPS_CONFIG_DCDC_HP_MASK;
    }
    if (!config->enableHysteresisVoltageSense)
    {
        tmp32 |= DCDC_REG0_OFFSET_RSNS_LP_DISABLE_MASK;
    }
    if (config->enableAdjustHystereticValueSense)
    {
        tmp32 |= DCDC_REG0_OFFSET_RSNS_LP_ADJ_MASK;
    }
    if (!config->enableHystersisComparator)
    {
        tmp32 |= DCDC_REG0_HYST_LP_CMP_DISABLE_MASK;
    }
    if (config->enableAdjustHystereticValueComparator)
    {
        tmp32 |= DCDC_REG0_HYST_LP_COMP_ADJ_MASK;
    }
    tmp32 |= DCDC_REG0_DCDC_LP_STATE_HYS_H(config->hystereticUpperThresholdValue) |
             DCDC_REG0_DCDC_LP_STATE_HYS_L(config->hystereticLowerThresholdValue);
    /* true  - DCDC compare the lower supply(relative to target value) with DCDC_LP_STATE_HYS_L. When it is lower than
     *         DCDC_LP_STATE_HYS_L, re-charge output.
     * false - DCDC compare the common mode sense of supply(relative to target value) with DCDC_LP_STATE_HYS_L. When it
     *         is lower than DCDC_LP_STATE_HYS_L, re-charge output.
     */
    if (config->enableDiffComparators)
    {
        tmp32 |= DCDC_REG0_DCDC_LP_DF_CMP_ENABLE_MASK;
    }

    base->REG0 = tmp32;
}

void DCDC_GetDefaultLoopControlConfig(dcdc_loop_control_config_t *config)
{
    assert(NULL != config);

    config->enableDiffHysteresis = false;
    config->enableCommonHysteresis = false;
    config->enableDiffHysteresisThresh = false;
    config->enableCommonHysteresisThresh = false;
    config->enableInvertHysteresisSign = false;
}

void DCDC_SetLoopControlConfig(DCDC_Type *base, const dcdc_loop_control_config_t *config)
{
    uint32_t tmp32;

    /* DCDC_REG1. */
    tmp32 = base->REG1 &
            ~(DCDC_REG1_DCDC_LOOPCTRL_EN_DF_HYST_MASK | DCDC_REG1_DCDC_LOOPCTRL_EN_CM_HYST_MASK |
              DCDC_REG1_DCDC_LOOPCTRL_DF_HST_THRESH_MASK | DCDC_REG1_DCDC_LOOPCTRL_CM_HST_THRESH_MASK);
    if (config->enableDiffHysteresis)
    {
        tmp32 |= DCDC_REG1_DCDC_LOOPCTRL_EN_DF_HYST_MASK;
    }
    if (config->enableCommonHysteresis)
    {
        tmp32 |= DCDC_REG1_DCDC_LOOPCTRL_EN_CM_HYST_MASK;
    }
    if (config->enableDiffHysteresisThresh)
    {
        tmp32 |= DCDC_REG1_DCDC_LOOPCTRL_DF_HST_THRESH_MASK;
    }
    if (config->enableCommonHysteresisThresh)
    {
        tmp32 |= DCDC_REG1_DCDC_LOOPCTRL_CM_HST_THRESH_MASK;
    }
    base->REG1 = tmp32;

    /* DCDC_REG2. */
    if (config->enableInvertHysteresisSign)
    {
        base->REG2 |= DCDC_REG2_DCDC_LOOPCTRL_HYST_SIGN_MASK;
    }
    else
    {
        base->REG2 &= ~DCDC_REG2_DCDC_LOOPCTRL_HYST_SIGN_MASK;
    }
}

void DCDC_SetClockSource(DCDC_Type *base, dcdc_clock_source_t clockSource)
{
    uint32_t tmp32;

    tmp32 =
        base->REG0 &
        ~(DCDC_REG0_DCDC_PWD_OSC_INT_MASK | DCDC_REG0_DCDC_SEL_CLK_MASK | DCDC_REG0_DCDC_DISABLE_AUTO_CLK_SWITCH_MASK);
    switch (clockSource)
    {
        case kDCDC_ClockInternalOsc:
            tmp32 |= DCDC_REG0_DCDC_DISABLE_AUTO_CLK_SWITCH_MASK;
            break;
        case kDCDC_ClockExternalOsc:
            /* Choose the external clock and disable the internal clock. */
            tmp32 |= DCDC_REG0_DCDC_DISABLE_AUTO_CLK_SWITCH_MASK | DCDC_REG0_DCDC_SEL_CLK_MASK |
                     DCDC_REG0_DCDC_PWD_OSC_INT_MASK;
            break;
        default:
            break;
    }
    base->REG0 = tmp32;
}

void DCDC_AdjustTargetVoltage(DCDC_Type *base, uint32_t vdd1p5xBoost, uint32_t vdd1p5xBuck, uint32_t vdd1p8)
{
    uint32_t tmp32;

    /* Unlock the limitation of setting target voltage. */
    base->REG3 &= ~(DCDC_REG3_DCDC_VDD1P8CTRL_DISABLE_STEP_MASK | DCDC_REG3_DCDC_VDD1P5XCTRL_DISABLE_STEP_MASK);
    /* Change the target voltage value. */
    tmp32 = base->REG3 &
            ~(DCDC_REG3_DCDC_VDD1P5XCTRL_TRG_BOOST_MASK | DCDC_REG3_DCDC_VDD1P5XCTRL_TRG_BUCK_MASK |
              DCDC_REG3_DCDC_VDD1P8CTRL_TRG_MASK);
    tmp32 |= DCDC_REG3_DCDC_VDD1P5XCTRL_TRG_BOOST(vdd1p5xBoost) | DCDC_REG3_DCDC_VDD1P5XCTRL_TRG_BUCK(vdd1p5xBuck) |
             DCDC_REG3_DCDC_VDD1P8CTRL_TRG(vdd1p8);
    base->REG3 = tmp32;

    /* DCDC_STS_DC_OK bit will be de-asserted after target register changes. After output voltage settling to new
     * target value, DCDC_STS_DC_OK will be asserted. */
    while (0U != (DCDC_REG0_DCDC_STS_DC_OK_MASK & base->REG0))
    {
    }
}

void DCDC_SetBatteryMonitorValue(DCDC_Type *base, uint32_t battValue)
{
    uint32_t tmp32;

    /* Disable the monitor before setting the new value */
    base->REG2 &= ~DCDC_REG2_DCDC_BATTMONITOR_EN_BATADJ_MASK;
    if (0U != battValue)
    {
        tmp32 = base->REG2 & ~DCDC_REG2_DCDC_BATTMONITOR_BATT_VAL_MASK;
        /* Enable the monitor with setting value. */
        tmp32 |= (DCDC_REG2_DCDC_BATTMONITOR_EN_BATADJ_MASK | DCDC_REG2_DCDC_BATTMONITOR_BATT_VAL(battValue));
        base->REG2 = tmp32;
    }
}

void DCDC_SetMinPowerConfig(DCDC_Type *base, const dcdc_min_power_config_t *config)
{
    uint32_t tmp32 = base->REG3 &
                     ~(DCDC_REG3_DCDC_MINPWR_HALF_FETS_MASK | DCDC_REG3_DCDC_MINPWR_DOUBLE_FETS_MASK |
                       DCDC_REG3_DCDC_MINPWR_DC_HALFCLK_MASK | DCDC_REG3_DCDC_MINPWR_HALF_FETS_PULSED_MASK |
                       DCDC_REG3_DCDC_MINPWR_DOUBLE_FETS_PULSED_MASK | DCDC_REG3_DCDC_MINPWR_DC_HALFCLK_PULSED_MASK);

    /* For Continuous mode. */
    if (config->enableUseHalfFetForContinuous)
    {
        tmp32 |= DCDC_REG3_DCDC_MINPWR_HALF_FETS_MASK;
    }
    if (config->enableUseDoubleFetForContinuous)
    {
        tmp32 |= DCDC_REG3_DCDC_MINPWR_DOUBLE_FETS_MASK;
    }
    if (config->enableUseHalfFreqForContinuous)
    {
        tmp32 |= DCDC_REG3_DCDC_MINPWR_DC_HALFCLK_MASK;
    }
    /* For Pulsed mode. */
    if (config->enableUseHalfFetForPulsed)
    {
        tmp32 |= DCDC_REG3_DCDC_MINPWR_HALF_FETS_PULSED_MASK;
    }
    if (config->enableUseDoubleFetForPulsed)
    {
        tmp32 |= DCDC_REG3_DCDC_MINPWR_DOUBLE_FETS_PULSED_MASK;
    }
    if (config->enableUseHalfFreqForPulsed)
    {
        tmp32 |= DCDC_REG3_DCDC_MINPWR_DC_HALFCLK_PULSED_MASK;
    }
    base->REG3 = tmp32;
}

void DCDC_GetDefaultMinPowerDefault(dcdc_min_power_config_t *config)
{
    assert(NULL != config);

    /* For Continuous mode. */
    config->enableUseHalfFetForContinuous = false;
    config->enableUseDoubleFetForContinuous = false;
    config->enableUseHalfFreqForContinuous = false;
    /* For Pulsed mode. */
    config->enableUseHalfFetForPulsed = false;
    config->enableUseDoubleFetForPulsed = false;
    config->enableUseHalfFreqForPulsed = false;
}

void DCDC_SetPulsedIntegratorConfig(DCDC_Type *base, const dcdc_pulsed_integrator_config_t *config)
{
    if (config->enableUseUserIntegratorValue) /* Enable to use the user integrator value. */
    {
        base->REG7 = (base->REG7 & ~DCDC_REG7_INTEGRATOR_VALUE_MASK) | DCDC_REG7_INTEGRATOR_VALUE_SEL_MASK |
                     DCDC_REG7_INTEGRATOR_VALUE(config->userIntegratorValue);
        if (config->enablePulseRunSpeedup)
        {
            base->REG7 |= DCDC_REG7_PULSE_RUN_SPEEDUP_MASK;
        }
    }
    else
    {
        base->REG7 = 0U;
    }
}

void DCDC_GetDefaultPulsedIntegratorConfig(dcdc_pulsed_integrator_config_t *config)
{
    assert(NULL != config);

    config->enableUseUserIntegratorValue = false;
    config->userIntegratorValue = 0U;
    config->enablePulseRunSpeedup = false;
}
