/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright (c) 2016, NXP
 * All rights reserved.
 *
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_spm.h"
#include "math.h" /* Using floor() function to convert float variable to int. */

void SPM_GetRegulatorStatus(SPM_Type *base, spm_regulator_status_t *info)
{
    assert(info);

    volatile uint32_t tmp32 = base->RSR; /* volatile here is to make sure this value is actually from the hardware. */

    info->isRadioRunForcePowerModeOn = (SPM_RSR_RFRUNFORCE_MASK == (tmp32 & SPM_RSR_RFRUNFORCE_MASK));
    info->radioLowPowerModeStatus =
        (spm_radio_low_power_mode_status_t)((tmp32 & SPM_RSR_RFPMSTAT_MASK) >> SPM_RSR_RFPMSTAT_SHIFT);
    info->mcuLowPowerModeStatus =
        (spm_mcu_low_power_mode_status_t)((tmp32 & SPM_RSR_MCUPMSTAT_MASK) >> SPM_RSR_MCUPMSTAT_SHIFT);
    info->isDcdcLdoOn =
        (0x4 == (0x4 & ((tmp32 & SPM_RSR_REGSEL_MASK) >> SPM_RSR_REGSEL_SHIFT))); /* 1<<2 responses DCDC LDO. */
    info->isRfLdoOn =
        (0x2 == (0x2 & ((tmp32 & SPM_RSR_REGSEL_MASK) >> SPM_RSR_REGSEL_SHIFT))); /* 1<<1 responses RF LDO. */
    info->isCoreLdoOn =
        (0x1 == (0x1 & ((tmp32 & SPM_RSR_REGSEL_MASK) >> SPM_RSR_REGSEL_SHIFT))); /* 1<<0 responses CORE LDO. */
}

void SPM_SetLowVoltDetectConfig(SPM_Type *base, const spm_low_volt_detect_config_t *config)
{
    uint32_t tmp32 = base->LVDSC1 &
                     ~(SPM_LVDSC1_VDD_LVDIE_MASK | SPM_LVDSC1_VDD_LVDRE_MASK | SPM_LVDSC1_VDD_LVDV_MASK |
                       SPM_LVDSC1_COREVDD_LVDIE_MASK | SPM_LVDSC1_COREVDD_LVDRE_MASK);

    /* VDD voltage detection. */
    tmp32 |= SPM_LVDSC1_VDD_LVDV(config->vddLowVoltDetectSelect);
    if (config->enableIntOnVddLowVolt)
    {
        tmp32 |= SPM_LVDSC1_VDD_LVDIE_MASK;
    }
    if (config->enableResetOnVddLowVolt)
    {
        tmp32 |= SPM_LVDSC1_VDD_LVDRE_MASK;
    }
    /* Clear the Low Voltage Detect Flag with previouse power detect setting. */
    tmp32 |= SPM_LVDSC1_VDD_LVDACK_MASK;

    /* COREVDD voltage detection. */
    if (config->enableIntOnCoreLowVolt)
    {
        tmp32 |= SPM_LVDSC1_COREVDD_LVDIE_MASK;
    }
    if (config->enableResetOnCoreLowVolt)
    {
        tmp32 |= SPM_LVDSC1_COREVDD_LVDRE_MASK;
    }
    tmp32 |= SPM_LVDSC1_COREVDD_LVDACK_MASK; /* Clear previous error flag. */

    base->LVDSC1 = tmp32;
}

void SPM_SetLowVoltWarningConfig(SPM_Type *base, const spm_low_volt_warning_config_t *config)
{
    uint32_t tmp32 = base->LVDSC2 & ~(SPM_LVDSC2_VDD_LVWV_MASK | SPM_LVDSC2_VDD_LVWIE_MASK);

    tmp32 |= SPM_LVDSC2_VDD_LVWV(config->vddLowVoltDetectSelect);
    if (config->enableIntOnVddLowVolt)
    {
        tmp32 |= SPM_LVDSC2_VDD_LVWIE_MASK;
    }
    tmp32 |= SPM_LVDSC2_VDD_LVWACK_MASK; /* Clear previous error flag. */

    base->LVDSC2 = tmp32;
}

void SPM_SetHighVoltDetectConfig(SPM_Type *base, const spm_high_volt_detect_config_t *config)
{
    uint32_t tmp32;
    
    tmp32 = base->HVDSC1 & ~(SPM_HVDSC1_VDD_HVDIE_MASK | SPM_HVDSC1_VDD_HVDRE_MASK |\
                             SPM_HVDSC1_VDD_HVDV_MASK);
    tmp32 |= SPM_HVDSC1_VDD_HVDV(config->vddHighVoltDetectSelect);
    if(config->enableIntOnVddHighVolt)
    {
        tmp32 |= SPM_HVDSC1_VDD_HVDIE_MASK;
    }
    if(config->enableResetOnVddHighVolt)
    {
        tmp32 |= SPM_HVDSC1_VDD_HVDRE_MASK;
    }
    tmp32 |= SPM_HVDSC1_VDD_HVDACK_MASK; /* Clear previous error flag. */
    
    base->HVDSC1 = tmp32;
}

void SPM_SetRfLdoConfig(SPM_Type *base, const spm_rf_ldo_config_t *config)
{
    uint32_t tmp32 = 0U;

    switch (config->lowPowerMode)
    {
        case kSPM_RfLdoRemainInHighPowerInLowPowerModes:
            tmp32 |= SPM_RFLDOLPCNFG_LPSEL_MASK;
            break;
        default: /* kSPM_RfLdoEnterLowPowerInLowPowerModes. */
            break;
    }
    base->RFLDOLPCNFG = tmp32;

    tmp32 = SPM_RFLDOSC_IOSSSEL(config->softStartDuration) | SPM_RFLDOSC_IOREGVSEL(config->rfIoRegulatorVolt);
    if (config->enableCurSink)
    {
        tmp32 |= SPM_RFLDOSC_ISINKEN_MASK;
    }
    base->RFLDOSC = tmp32;
}

void SPM_SetDcdcBattMonitor(SPM_Type *base, uint32_t batAdcVal)
{
    /* Clear the value and disable it at first. */
    base->DCDCC2 &= ~(SPM_DCDCC2_DCDC_BATTMONITOR_BATT_VAL_MASK | SPM_DCDCC2_DCDC_BATTMONITOR_EN_BATADJ_MASK);
    if (0U != batAdcVal)
    {
        /* When setting the value to BATT_VAL field, it should be zero before. */
        base->DCDCC2 |= SPM_DCDCC2_DCDC_BATTMONITOR_BATT_VAL(batAdcVal);
        base->DCDCC2 |= SPM_DCDCC2_DCDC_BATTMONITOR_EN_BATADJ_MASK;
    }
}

void SPM_EnableVddxStepLock(SPM_Type *base, bool enable)
{
    if (enable)
    {
        base->DCDCC3 |= (SPM_DCDCC3_DCDC_VDD1P8CTRL_DISABLE_STEP_MASK | SPM_DCDCC3_DCDC_VDD1P2CTRL_DISABLE_STEP_MASK);
    }
    else
    {
        base->DCDCC3 &= ~(SPM_DCDCC3_DCDC_VDD1P8CTRL_DISABLE_STEP_MASK | SPM_DCDCC3_DCDC_VDD1P2CTRL_DISABLE_STEP_MASK);
    }
}

void SPM_BypassDcdcBattMonitor(SPM_Type *base, bool enable, uint32_t value)
{
    if (enable)
    {
        /* Set the user-defined value before enable the bypass. */
        base->DCDCC3 = (base->DCDCC3 & ~SPM_DCDCC3_DCDC_VBAT_VALUE_MASK) | SPM_DCDCC3_DCDC_VBAT_VALUE(value);
        /* Enable the bypass and load the user-defined value. */
        base->DCDCC3 |= SPM_DCDCC3_DCDC_BYPASS_ADC_MEAS_MASK;
    }
    else
    {
        base->DCDCC3 &= ~SPM_DCDCC3_DCDC_BYPASS_ADC_MEAS_MASK;
    }
}

void SPM_SetDcdcIntegratorConfig(SPM_Type *base, const spm_dcdc_integrator_config_t *config)
{
    int32_t tmp32u;
    double dutyCycle;

    if (NULL == config)
    {
        base->DCDCC4 = 0U;
    }
    else
    {
        dutyCycle = ((config->vdd1p2Value / config->vBatValue)*32 - 16)*8192;
        tmp32u = (int32_t)(dutyCycle);
        base->DCDCC4 = SPM_DCDCC4_PULSE_RUN_SPEEDUP_MASK | SPM_DCDCC4_INTEGRATOR_VALUE_SELECT_MASK |
                       SPM_DCDCC4_INTEGRATOR_VALUE(tmp32u);
    }
} 
 


void SPM_SetLowPowerReqOutPinConfig(SPM_Type *base, const spm_low_power_req_out_pin_config_t *config)
{
    if ((NULL == config) || (config->pinOutEnable))
    {
        base->LPREQPINCNTRL = 0U;
    }
    else
    {
        base->LPREQPINCNTRL =
            SPM_LPREQPINCNTRL_POLARITY(config->pinOutPol) | SPM_LPREQPINCNTRL_LPREQOE_MASK; /* Enable the output. */
    }
}
