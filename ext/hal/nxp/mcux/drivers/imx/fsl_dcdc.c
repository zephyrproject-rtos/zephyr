/*
 * The Clear BSD License
 * Copyright (c) 2017, NXP
 * All rights reserved.
 *
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 *  that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
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

void DCDC_SetClockSource(DCDC_Type *base, dcdc_clock_source_t clockSource)
{
    uint32_t tmp32;

    /* Configure the DCDC_REG0 register. */
    tmp32 = base->REG0 &
            ~(DCDC_REG0_XTAL_24M_OK_MASK | DCDC_REG0_DISABLE_AUTO_CLK_SWITCH_MASK | DCDC_REG0_SEL_CLK_MASK |
              DCDC_REG0_PWD_OSC_INT_MASK);
    switch (clockSource)
    {
        case kDCDC_ClockInternalOsc:
            tmp32 |= DCDC_REG0_DISABLE_AUTO_CLK_SWITCH_MASK;
            break;
        case kDCDC_ClockExternalOsc:
            /* Choose the external clock and disable the internal clock. */
            tmp32 |= DCDC_REG0_DISABLE_AUTO_CLK_SWITCH_MASK | DCDC_REG0_SEL_CLK_MASK | DCDC_REG0_PWD_OSC_INT_MASK;
            break;
        case kDCDC_ClockAutoSwitch:
            /* Set to switch from internal ring osc to xtal 24M if auto mode is enabled. */
            tmp32 |= DCDC_REG0_XTAL_24M_OK_MASK;
            break;
        default:
            break;
    }
    base->REG0 = tmp32;
}

void DCDC_GetDefaultDetectionConfig(dcdc_detection_config_t *config)
{
    assert(NULL != config);

    config->enableXtalokDetection = false;
    config->powerDownOverVoltageDetection = true;
    config->powerDownLowVlotageDetection = false;
    config->powerDownOverCurrentDetection = true;
    config->powerDownPeakCurrentDetection = true;
    config->powerDownZeroCrossDetection = true;
    config->OverCurrentThreshold = kDCDC_OverCurrentThresholdAlt0;
    config->PeakCurrentThreshold = kDCDC_PeakCurrentThresholdAlt0;
}

void DCDC_SetDetectionConfig(DCDC_Type *base, const dcdc_detection_config_t *config)
{
    assert(NULL != config);

    uint32_t tmp32;
    /* Configure the DCDC_REG0 register. */
    tmp32 = base->REG0 &
            ~(DCDC_REG0_XTALOK_DISABLE_MASK | DCDC_REG0_PWD_HIGH_VOLT_DET_MASK | DCDC_REG0_PWD_CMP_BATT_DET_MASK |
              DCDC_REG0_PWD_OVERCUR_DET_MASK | DCDC_REG0_PWD_CUR_SNS_CMP_MASK | DCDC_REG0_PWD_ZCD_MASK |
              DCDC_REG0_CUR_SNS_THRSH_MASK | DCDC_REG0_OVERCUR_TRIG_ADJ_MASK);

    tmp32 |= DCDC_REG0_CUR_SNS_THRSH(config->PeakCurrentThreshold) |
             DCDC_REG0_OVERCUR_TRIG_ADJ(config->OverCurrentThreshold);
    if (false == config->enableXtalokDetection)
    {
        tmp32 |= DCDC_REG0_XTALOK_DISABLE_MASK;
    }
    if (config->powerDownOverVoltageDetection)
    {
        tmp32 |= DCDC_REG0_PWD_HIGH_VOLT_DET_MASK;
    }
    if (config->powerDownLowVlotageDetection)
    {
        tmp32 |= DCDC_REG0_PWD_CMP_BATT_DET_MASK;
    }
    if (config->powerDownOverCurrentDetection)
    {
        tmp32 |= DCDC_REG0_PWD_OVERCUR_DET_MASK;
    }
    if (config->powerDownPeakCurrentDetection)
    {
        tmp32 |= DCDC_REG0_PWD_CUR_SNS_CMP_MASK;
    }
    if (config->powerDownZeroCrossDetection)
    {
        tmp32 |= DCDC_REG0_PWD_ZCD_MASK;
    }
    base->REG0 = tmp32;
}

void DCDC_GetDefaultLowPowerConfig(dcdc_low_power_config_t *config)
{
    assert(NULL != config);

    config->enableOverloadDetection = true;
    config->enableAdjustHystereticValue = false;
    config->countChargingTimePeriod = kDCDC_CountChargingTimePeriod8Cycle;
    config->countChargingTimeThreshold = kDCDC_CountChargingTimeThreshold32;
}

void DCDC_SetLowPowerConfig(DCDC_Type *base, const dcdc_low_power_config_t *config)
{
    assert(NULL != config);

    uint32_t tmp32;
    /* Configure the DCDC_REG0 register. */
    tmp32 = base->REG0 &
            ~(DCDC_REG0_EN_LP_OVERLOAD_SNS_MASK | DCDC_REG0_LP_HIGH_HYS_MASK | DCDC_REG0_LP_OVERLOAD_FREQ_SEL_MASK |
              DCDC_REG0_LP_OVERLOAD_THRSH_MASK);
    tmp32 |= DCDC_REG0_LP_OVERLOAD_FREQ_SEL(config->countChargingTimePeriod) |
             DCDC_REG0_LP_OVERLOAD_THRSH(config->countChargingTimeThreshold);
    if (config->enableOverloadDetection)
    {
        tmp32 |= DCDC_REG0_EN_LP_OVERLOAD_SNS_MASK;
    }
    if (config->enableAdjustHystereticValue)
    {
        tmp32 |= DCDC_REG0_LP_HIGH_HYS_MASK;
    }
    base->REG0 = tmp32;
}

uint32_t DCDC_GetstatusFlags(DCDC_Type *base)
{
    uint32_t tmp32 = 0U;

    if (DCDC_REG0_STS_DC_OK_MASK == (DCDC_REG0_STS_DC_OK_MASK & base->REG0))
    {
        tmp32 |= kDCDC_LockedOKStatus;
    }

    return tmp32;
}

void DCDC_ResetCurrentAlertSignal(DCDC_Type *base, bool enable)
{
    if (enable)
    {
        base->REG0 |= DCDC_REG0_CURRENT_ALERT_RESET_MASK;
    }
    else
    {
        base->REG0 &= ~DCDC_REG0_CURRENT_ALERT_RESET_MASK;
    }
}

void DCDC_GetDefaultLoopControlConfig(dcdc_loop_control_config_t *config)
{
    assert(NULL != config);

    config->enableCommonHysteresis = false;
    config->enableCommonThresholdDetection = false;
    config->enableInvertHysteresisSign = false;
    config->enableRCThresholdDetection = false;
    config->enableRCScaleCircuit = 0U;
    config->complementFeedForwardStep = 0U;
    config->controlParameterMagnitude = 2U;
    config->integralProportionalRatio = 2U;
}

void DCDC_SetLoopControlConfig(DCDC_Type *base, const dcdc_loop_control_config_t *config)
{
    assert(NULL != config);

    uint32_t tmp32;

    /* Configure the DCDC_REG1 register. */
    tmp32 = base->REG1 & ~(DCDC_REG1_LOOPCTRL_EN_HYST_MASK | DCDC_REG1_LOOPCTRL_HST_THRESH_MASK);
    if (config->enableCommonHysteresis)
    {
        tmp32 |= DCDC_REG1_LOOPCTRL_EN_HYST_MASK;
    }
    if (config->enableCommonThresholdDetection)
    {
        tmp32 |= DCDC_REG1_LOOPCTRL_HST_THRESH_MASK;
    }
    base->REG1 = tmp32;

    /* configure the DCDC_REG2 register. */
    tmp32 = base->REG2 &
            ~(DCDC_REG2_LOOPCTRL_HYST_SIGN_MASK | DCDC_REG2_LOOPCTRL_RCSCALE_THRSH_MASK |
              DCDC_REG2_LOOPCTRL_EN_RCSCALE_MASK | DCDC_REG2_LOOPCTRL_DC_FF_MASK | DCDC_REG2_LOOPCTRL_DC_R_MASK |
              DCDC_REG2_LOOPCTRL_DC_C_MASK);
    tmp32 |= DCDC_REG2_LOOPCTRL_DC_FF(config->complementFeedForwardStep) |
             DCDC_REG2_LOOPCTRL_DC_R(config->controlParameterMagnitude) |
             DCDC_REG2_LOOPCTRL_DC_C(config->integralProportionalRatio) |
             DCDC_REG2_LOOPCTRL_EN_RCSCALE(config->enableRCScaleCircuit);
    if (config->enableInvertHysteresisSign)
    {
        tmp32 |= DCDC_REG2_LOOPCTRL_HYST_SIGN_MASK;
    }
    if (config->enableRCThresholdDetection)
    {
        tmp32 |= DCDC_REG2_LOOPCTRL_RCSCALE_THRSH_MASK;
    }
    base->REG2 = tmp32;
}

void DCDC_SetMinPowerConfig(DCDC_Type *base, const dcdc_min_power_config_t *config)
{
    assert(NULL != config);

    uint32_t tmp32;

    tmp32 = base->REG3 & ~DCDC_REG3_MINPWR_DC_HALFCLK_MASK;
    if (config->enableUseHalfFreqForContinuous)
    {
        tmp32 |= DCDC_REG3_MINPWR_DC_HALFCLK_MASK;
    }
    base->REG3 = tmp32;
}

void DCDC_AdjustTargetVoltage(DCDC_Type *base, uint32_t VDDRun, uint32_t VDDStandby)
{
    uint32_t tmp32;

    /* Unlock the step for the output. */
    base->REG3 &= ~DCDC_REG3_DISABLE_STEP_MASK;

    /* Configure the DCDC_REG3 register. */
    tmp32 = base->REG3 & ~(DCDC_REG3_TARGET_LP_MASK | DCDC_REG3_TRG_MASK);

    tmp32 |= DCDC_REG3_TARGET_LP(VDDStandby) | DCDC_REG3_TRG(VDDRun);
    base->REG3 = tmp32;

    /* DCDC_STS_DC_OK bit will be de-asserted after target register changes. After output voltage settling to new
     * target value, DCDC_STS_DC_OK will be asserted. */
    while (DCDC_REG0_STS_DC_OK_MASK != (DCDC_REG0_STS_DC_OK_MASK & base->REG0))
    {
    }
}

void DCDC_SetInternalRegulatorConfig(DCDC_Type *base, const dcdc_internal_regulator_config_t *config)
{
    assert(NULL != config);

    uint32_t tmp32;

    /* Configure the DCDC_REG1 register. */
    tmp32 = base->REG1 & ~(DCDC_REG1_REG_FBK_SEL_MASK | DCDC_REG1_REG_RLOAD_SW_MASK);
    tmp32 |= DCDC_REG1_REG_FBK_SEL(config->feedbackPoint);
    if (config->enableLoadResistor)
    {
        tmp32 |= DCDC_REG1_REG_RLOAD_SW_MASK;
    }
    base->REG1 = tmp32;
}

void DCDC_BootIntoDCM(DCDC_Type *base)
{
    base->REG0 &= ~(DCDC_REG0_PWD_ZCD_MASK | DCDC_REG0_PWD_CMP_OFFSET_MASK);
    base->REG2 = (~DCDC_REG2_LOOPCTRL_EN_RCSCALE_MASK & base->REG2) | DCDC_REG2_LOOPCTRL_EN_RCSCALE(0x4U) |
                 DCDC_REG2_DCM_SET_CTRL_MASK;
}

void DCDC_BootIntoCCM(DCDC_Type *base)
{
    base->REG0 = (~DCDC_REG0_PWD_CMP_OFFSET_MASK & base->REG0) | DCDC_REG0_PWD_ZCD_MASK;
    base->REG2 = (~DCDC_REG2_LOOPCTRL_EN_RCSCALE_MASK & base->REG2) | DCDC_REG2_LOOPCTRL_EN_RCSCALE(0x3U);
}
