/*******************************************************************************
 * File Name: cycfg_system.c
 *
 * Description:
 * System configuration
 * This file was automatically generated and should not be modified.
 * Tools Package 2.4.0.5721
 * mtb-pdl-cat1 3.0.0.10651
 * personalities 5.0.0.0
 * udd 3.0.0.1377
 *
 ********************************************************************************
 * Copyright 2021 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ********************************************************************************/

#include "cycfg_system.h"

#define CY_CFG_SYSCLK_ECO_ERROR 1
#define CY_CFG_SYSCLK_ALTHF_ERROR 2
#define CY_CFG_SYSCLK_PLL_ERROR 3
#define CY_CFG_SYSCLK_FLL_ERROR 4
#define CY_CFG_SYSCLK_WCO_ERROR 5
#define CY_CFG_SYSCLK_CLKBAK_ENABLED 1
#define CY_CFG_SYSCLK_CLKBAK_SOURCE CY_SYSCLK_BAK_IN_WCO
#define CY_CFG_SYSCLK_CLKFAST_ENABLED 1
#define CY_CFG_SYSCLK_CLKFAST_DIVIDER 0
#define CY_CFG_SYSCLK_FLL_ENABLED 1
#define CY_CFG_SYSCLK_FLL_MULT 500U
#define CY_CFG_SYSCLK_FLL_REFDIV 20U
#define CY_CFG_SYSCLK_FLL_CCO_RANGE CY_SYSCLK_FLL_CCO_RANGE4
#define CY_CFG_SYSCLK_FLL_ENABLE_OUTDIV true
#define CY_CFG_SYSCLK_FLL_LOCK_TOLERANCE 10U
#define CY_CFG_SYSCLK_FLL_IGAIN 9U
#define CY_CFG_SYSCLK_FLL_PGAIN 5U
#define CY_CFG_SYSCLK_FLL_SETTLING_COUNT 8U
#define CY_CFG_SYSCLK_FLL_OUTPUT_MODE CY_SYSCLK_FLLPLL_OUTPUT_OUTPUT
#define CY_CFG_SYSCLK_FLL_CCO_FREQ 355U
#define CY_CFG_SYSCLK_FLL_OUT_FREQ 100000000
#define CY_CFG_SYSCLK_CLKHF0_ENABLED 1
#define CY_CFG_SYSCLK_CLKHF0_DIVIDER CY_SYSCLK_CLKHF_NO_DIVIDE
#define CY_CFG_SYSCLK_CLKHF0_FREQ_MHZ 100UL
#define CY_CFG_SYSCLK_CLKHF0_CLKPATH CY_SYSCLK_CLKHF_IN_CLKPATH0
#define CY_CFG_SYSCLK_ILO_ENABLED 1
#define CY_CFG_SYSCLK_ILO_HIBERNATE true
#define CY_CFG_SYSCLK_IMO_ENABLED 1
#define CY_CFG_SYSCLK_CLKLF_ENABLED 1
#define CY_CFG_SYSCLK_CLKPATH0_ENABLED 1
#define CY_CFG_SYSCLK_CLKPATH0_SOURCE CY_SYSCLK_CLKPATH_IN_IMO
#define CY_CFG_SYSCLK_CLKPATH0_SOURCE_NUM 0UL
#define CY_CFG_SYSCLK_CLKPATH1_ENABLED 1
#define CY_CFG_SYSCLK_CLKPATH1_SOURCE CY_SYSCLK_CLKPATH_IN_IMO
#define CY_CFG_SYSCLK_CLKPATH1_SOURCE_NUM 0UL
#define CY_CFG_SYSCLK_CLKPATH2_ENABLED 1
#define CY_CFG_SYSCLK_CLKPATH2_SOURCE CY_SYSCLK_CLKPATH_IN_IMO
#define CY_CFG_SYSCLK_CLKPATH2_SOURCE_NUM 0UL
#define CY_CFG_SYSCLK_CLKPATH3_ENABLED 1
#define CY_CFG_SYSCLK_CLKPATH3_SOURCE CY_SYSCLK_CLKPATH_IN_IMO
#define CY_CFG_SYSCLK_CLKPATH3_SOURCE_NUM 0UL
#define CY_CFG_SYSCLK_CLKPATH4_ENABLED 1
#define CY_CFG_SYSCLK_CLKPATH4_SOURCE CY_SYSCLK_CLKPATH_IN_IMO
#define CY_CFG_SYSCLK_CLKPATH4_SOURCE_NUM 0UL
#define CY_CFG_SYSCLK_CLKPATH5_ENABLED 1
#define CY_CFG_SYSCLK_CLKPATH5_SOURCE CY_SYSCLK_CLKPATH_IN_IMO
#define CY_CFG_SYSCLK_CLKPATH5_SOURCE_NUM 0UL
#define CY_CFG_SYSCLK_CLKPERI_ENABLED 1
#define CY_CFG_SYSCLK_CLKPERI_DIVIDER 0
#define CY_CFG_SYSCLK_PLL0_ENABLED 1
#define CY_CFG_SYSCLK_PLL0_FEEDBACK_DIV 30
#define CY_CFG_SYSCLK_PLL0_REFERENCE_DIV 1
#define CY_CFG_SYSCLK_PLL0_OUTPUT_DIV 5
#define CY_CFG_SYSCLK_PLL0_LF_MODE false
#define CY_CFG_SYSCLK_PLL0_OUTPUT_MODE CY_SYSCLK_FLLPLL_OUTPUT_AUTO
#define CY_CFG_SYSCLK_PLL0_OUTPUT_FREQ 48000000
#define CY_CFG_SYSCLK_CLKSLOW_ENABLED 1
#define CY_CFG_SYSCLK_CLKSLOW_DIVIDER 0
#define CY_CFG_SYSCLK_WCO_ENABLED 1
#define CY_CFG_SYSCLK_WCO_IN_PRT GPIO_PRT0
#define CY_CFG_SYSCLK_WCO_IN_PIN 0U
#define CY_CFG_SYSCLK_WCO_OUT_PRT GPIO_PRT0
#define CY_CFG_SYSCLK_WCO_OUT_PIN 1U
#define CY_CFG_SYSCLK_WCO_BYPASS CY_SYSCLK_WCO_NOT_BYPASSED
#define CY_CFG_PWR_ENABLED 1
#define CY_CFG_PWR_INIT 1
#define CY_CFG_PWR_USING_PMIC 0
#define CY_CFG_PWR_VBACKUP_USING_VDDD 1
#define CY_CFG_PWR_LDO_VOLTAGE CY_SYSPM_LDO_VOLTAGE_LP
#define CY_CFG_PWR_USING_ULP 0
#define CY_CFG_PWR_REGULATOR_MODE_MIN false
#define CY_CFG_PWR_BKP_ERROR 6

#if defined (CY_DEVICE_SECURE)
static cy_stc_pra_system_config_t srss_0_clock_0_secureConfig;
#endif // defined (CY_DEVICE_SECURE)
#if (!defined(CY_DEVICE_SECURE))
static const cy_stc_fll_manual_config_t srss_0_clock_0_fll_0_fllConfig =
{
	.fllMult = 500U,
	.refDiv = 20U,
	.ccoRange = CY_SYSCLK_FLL_CCO_RANGE4,
	.enableOutputDiv = true,
	.lockTolerance = 10U,
	.igain = 9U,
	.pgain = 5U,
	.settlingCount = 8U,
	.outputMode = CY_SYSCLK_FLLPLL_OUTPUT_OUTPUT,
	.cco_Freq = 355U,
};
#endif // (!defined(CY_DEVICE_SECURE))
#if defined (CY_USING_HAL)
const cyhal_resource_inst_t srss_0_clock_0_pathmux_0_obj =
{
	.type = CYHAL_RSC_CLKPATH,
	.block_num = 0U,
	.channel_num = 0U,
};
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
const cyhal_resource_inst_t srss_0_clock_0_pathmux_1_obj =
{
	.type = CYHAL_RSC_CLKPATH,
	.block_num = 1U,
	.channel_num = 0U,
};
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
const cyhal_resource_inst_t srss_0_clock_0_pathmux_2_obj =
{
	.type = CYHAL_RSC_CLKPATH,
	.block_num = 2U,
	.channel_num = 0U,
};
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
const cyhal_resource_inst_t srss_0_clock_0_pathmux_3_obj =
{
	.type = CYHAL_RSC_CLKPATH,
	.block_num = 3U,
	.channel_num = 0U,
};
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
const cyhal_resource_inst_t srss_0_clock_0_pathmux_4_obj =
{
	.type = CYHAL_RSC_CLKPATH,
	.block_num = 4U,
	.channel_num = 0U,
};
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
const cyhal_resource_inst_t srss_0_clock_0_pathmux_5_obj =
{
	.type = CYHAL_RSC_CLKPATH,
	.block_num = 5U,
	.channel_num = 0U,
};
#endif // defined (CY_USING_HAL)
#if (!defined(CY_DEVICE_SECURE))
static const cy_stc_pll_manual_config_t srss_0_clock_0_pll_0_pllConfig =
{
	.feedbackDiv = 30,
	.referenceDiv = 1,
	.outputDiv = 5,
	.lfMode = false,
	.outputMode = CY_SYSCLK_FLLPLL_OUTPUT_AUTO,
};
#endif // (!defined(CY_DEVICE_SECURE))

__WEAK void __NO_RETURN cycfg_ClockStartupError(uint32_t error)
{
	(void)error; /* Suppress the compiler warning */
	while (1);
}
#if defined (CY_DEVICE_SECURE)
void init_cycfg_secure_struct(cy_stc_pra_system_config_t *secure_config)
{
	#ifdef CY_CFG_PWR_ENABLED
	secure_config->powerEnable = CY_CFG_PWR_ENABLED;
	#endif /* CY_CFG_PWR_ENABLED */

	#ifdef CY_CFG_PWR_USING_LDO
	secure_config->ldoEnable = CY_CFG_PWR_USING_LDO;
	#endif /* CY_CFG_PWR_USING_LDO */

	#ifdef CY_CFG_PWR_USING_PMIC
	secure_config->pmicEnable = CY_CFG_PWR_USING_PMIC;
	#endif /* CY_CFG_PWR_USING_PMIC */

	#ifdef CY_CFG_PWR_VBACKUP_USING_VDDD
	secure_config->vBackupVDDDEnable = CY_CFG_PWR_VBACKUP_USING_VDDD;
	#endif /* CY_CFG_PWR_VBACKUP_USING_VDDD */

	#ifdef CY_CFG_PWR_USING_ULP
	secure_config->ulpEnable = CY_CFG_PWR_USING_ULP;
	#endif /* CY_CFG_PWR_USING_ULP */

	#ifdef CY_CFG_SYSCLK_ECO_ENABLED
	secure_config->ecoEnable = CY_CFG_SYSCLK_ECO_ENABLED;
	#endif /* CY_CFG_SYSCLK_ECO_ENABLED */

	#ifdef CY_CFG_SYSCLK_EXTCLK_ENABLED
	secure_config->extClkEnable = CY_CFG_SYSCLK_EXTCLK_ENABLED;
	#endif /* CY_CFG_SYSCLK_EXTCLK_ENABLED */

	#ifdef CY_CFG_SYSCLK_ILO_ENABLED
	secure_config->iloEnable = CY_CFG_SYSCLK_ILO_ENABLED;
	#endif /* CY_CFG_SYSCLK_ILO_ENABLED */

	#ifdef CY_CFG_SYSCLK_WCO_ENABLED
	secure_config->wcoEnable = CY_CFG_SYSCLK_WCO_ENABLED;
	#endif /* CY_CFG_SYSCLK_WCO_ENABLED */

	#ifdef CY_CFG_SYSCLK_FLL_ENABLED
	secure_config->fllEnable = CY_CFG_SYSCLK_FLL_ENABLED;
	#endif /* CY_CFG_SYSCLK_FLL_ENABLED */

	#ifdef CY_CFG_SYSCLK_PLL0_ENABLED
	secure_config->pll0Enable = CY_CFG_SYSCLK_PLL0_ENABLED;
	#endif /* CY_CFG_SYSCLK_PLL0_ENABLED */

	#ifdef CY_CFG_SYSCLK_PLL1_ENABLED
	secure_config->pll1Enable = CY_CFG_SYSCLK_PLL1_ENABLED;
	#endif /* CY_CFG_SYSCLK_PLL1_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKPATH0_ENABLED
	secure_config->path0Enable = CY_CFG_SYSCLK_CLKPATH0_ENABLED;
	#endif /* CY_CFG_SYSCLK_CLKPATH0_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKPATH1_ENABLED
	secure_config->path1Enable = CY_CFG_SYSCLK_CLKPATH1_ENABLED;
	#endif /* CY_CFG_SYSCLK_CLKPATH1_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKPATH2_ENABLED
	secure_config->path2Enable = CY_CFG_SYSCLK_CLKPATH2_ENABLED;
	#endif /* CY_CFG_SYSCLK_CLKPATH2_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKPATH3_ENABLED
	secure_config->path3Enable = CY_CFG_SYSCLK_CLKPATH3_ENABLED;
	#endif /* CY_CFG_SYSCLK_CLKPATH3_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKPATH4_ENABLED
	secure_config->path4Enable = CY_CFG_SYSCLK_CLKPATH4_ENABLED;
	#endif /* CY_CFG_SYSCLK_CLKPATH4_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKPATH5_ENABLED
	secure_config->path5Enable = CY_CFG_SYSCLK_CLKPATH5_ENABLED;
	#endif /* CY_CFG_SYSCLK_CLKPATH5_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKFAST_ENABLED
	secure_config->clkFastEnable = CY_CFG_SYSCLK_CLKFAST_ENABLED;
	#endif /* CY_CFG_SYSCLK_CLKFAST_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKPERI_ENABLED
	secure_config->clkPeriEnable = CY_CFG_SYSCLK_CLKPERI_ENABLED;
	#endif /* CY_CFG_SYSCLK_CLKPERI_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKSLOW_ENABLED
	secure_config->clkSlowEnable = CY_CFG_SYSCLK_CLKSLOW_ENABLED;
	#endif /* CY_CFG_SYSCLK_CLKSLOW_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKHF0_ENABLED
	secure_config->clkHF0Enable = CY_CFG_SYSCLK_CLKHF0_ENABLED;
	#endif /* CY_CFG_SYSCLK_CLKHF0_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKHF1_ENABLED
	secure_config->clkHF1Enable = CY_CFG_SYSCLK_CLKHF1_ENABLED;
	#endif /* CY_CFG_SYSCLK_CLKHF1_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKHF2_ENABLED
	secure_config->clkHF2Enable = CY_CFG_SYSCLK_CLKHF2_ENABLED;
	#endif /* CY_CFG_SYSCLK_CLKHF2_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKHF3_ENABLED
	secure_config->clkHF3Enable = CY_CFG_SYSCLK_CLKHF3_ENABLED;
	#endif /* CY_CFG_SYSCLK_CLKHF3_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKHF4_ENABLED
	secure_config->clkHF4Enable = CY_CFG_SYSCLK_CLKHF4_ENABLED;
	#endif /* CY_CFG_SYSCLK_CLKHF4_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKHF5_ENABLED
	secure_config->clkHF5Enable = CY_CFG_SYSCLK_CLKHF5_ENABLED;
	#endif /* CY_CFG_SYSCLK_CLKHF5_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKPUMP_ENABLED
	secure_config->clkPumpEnable = CY_CFG_SYSCLK_CLKPUMP_ENABLED;
	#endif /* CY_CFG_SYSCLK_CLKPUMP_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKLF_ENABLED
	secure_config->clkLFEnable = CY_CFG_SYSCLK_CLKLF_ENABLED;
	#endif /* CY_CFG_SYSCLK_CLKLF_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKBAK_ENABLED
	secure_config->clkBakEnable = CY_CFG_SYSCLK_CLKBAK_ENABLED;
	#endif /* CY_CFG_SYSCLK_CLKBAK_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKTIMER_ENABLED
	secure_config->clkTimerEnable = CY_CFG_SYSCLK_CLKTIMER_ENABLED;
	#endif /* CY_CFG_SYSCLK_CLKTIMER_ENABLED */

	#ifdef CY_CFG_SYSCLK_CLKALTSYSTICK_ENABLED
	    #error Configuration Error : ALT SYSTICK cannot be enabled for Secure devices.
	#endif /* CY_CFG_SYSCLK_CLKALTSYSTICK_ENABLED */

	#ifdef CY_CFG_SYSCLK_PILO_ENABLED
	secure_config->piloEnable = CY_CFG_SYSCLK_PILO_ENABLED;
	#endif /* CY_CFG_SYSCLK_PILO_ENABLED */

	#ifdef CY_CFG_SYSCLK_ALTHF_ENABLED
	secure_config->clkAltHfEnable = CY_CFG_SYSCLK_ALTHF_ENABLED;
	#endif /* CY_CFG_SYSCLK_ALTHF_ENABLED */

	#ifdef CY_CFG_PWR_LDO_VOLTAGE
	secure_config->ldoVoltage = CY_CFG_PWR_LDO_VOLTAGE;
	#endif /* CY_CFG_PWR_LDO_VOLTAGE */

	#ifdef CY_CFG_PWR_REGULATOR_MODE_MIN
	secure_config->pwrCurrentModeMin = CY_CFG_PWR_REGULATOR_MODE_MIN;
	#endif /* CY_CFG_PWR_REGULATOR_MODE_MIN */

	#ifdef CY_CFG_PWR_BUCK_VOLTAGE
	secure_config->buckVoltage = CY_CFG_PWR_BUCK_VOLTAGE;
	#endif /* CY_CFG_PWR_BUCK_VOLTAGE */

	#ifdef CY_CFG_SYSCLK_ECO_FREQ
	secure_config->ecoFreqHz = CY_CFG_SYSCLK_ECO_FREQ;
	#endif /* CY_CFG_SYSCLK_ECO_FREQ */

	#ifdef CY_CFG_SYSCLK_ECO_CLOAD
	secure_config->ecoLoad = CY_CFG_SYSCLK_ECO_CLOAD;
	#endif /* CY_CFG_SYSCLK_ECO_CLOAD */

	#ifdef CY_CFG_SYSCLK_ECO_ESR
	secure_config->ecoEsr = CY_CFG_SYSCLK_ECO_ESR;
	#endif /* CY_CFG_SYSCLK_ECO_ESR */

	#ifdef CY_CFG_SYSCLK_ECO_DRIVE_LEVEL
	secure_config->ecoDriveLevel = CY_CFG_SYSCLK_ECO_DRIVE_LEVEL;
	#endif /* CY_CFG_SYSCLK_ECO_DRIVE_LEVEL */

	#ifdef CY_CFG_SYSCLK_ECO_GPIO_IN_PRT
	secure_config->ecoInPort = CY_CFG_SYSCLK_ECO_GPIO_IN_PRT;
	#endif /* CY_CFG_SYSCLK_ECO_GPIO_IN_PRT */

	#ifdef CY_CFG_SYSCLK_ECO_GPIO_OUT_PRT
	secure_config->ecoOutPort = CY_CFG_SYSCLK_ECO_GPIO_OUT_PRT;
	#endif /* CY_CFG_SYSCLK_ECO_GPIO_OUT_PRT */

	#ifdef CY_CFG_SYSCLK_ECO_GPIO_IN_PIN
	secure_config->ecoInPinNum = CY_CFG_SYSCLK_ECO_GPIO_IN_PIN;
	#endif /* CY_CFG_SYSCLK_ECO_GPIO_IN_PIN */

	#ifdef CY_CFG_SYSCLK_ECO_GPIO_OUT_PIN
	secure_config->ecoOutPinNum = CY_CFG_SYSCLK_ECO_GPIO_OUT_PIN;
	#endif /* CY_CFG_SYSCLK_ECO_GPIO_OUT_PIN */

	#ifdef CY_CFG_SYSCLK_EXTCLK_FREQ
	secure_config->extClkFreqHz = CY_CFG_SYSCLK_EXTCLK_FREQ;
	#endif /* CY_CFG_SYSCLK_EXTCLK_FREQ */

	#ifdef CY_CFG_SYSCLK_EXTCLK_GPIO_PRT
	secure_config->extClkPort = CY_CFG_SYSCLK_EXTCLK_GPIO_PRT;
	#endif /* CY_CFG_SYSCLK_EXTCLK_GPIO_PRT */

	#ifdef CY_CFG_SYSCLK_EXTCLK_GPIO_PIN
	secure_config->extClkPinNum = CY_CFG_SYSCLK_EXTCLK_GPIO_PIN;
	#endif /* CY_CFG_SYSCLK_EXTCLK_GPIO_PIN */

	#ifdef CY_CFG_SYSCLK_EXTCLK_GPIO_HSIOM
	secure_config->extClkHsiom = CY_CFG_SYSCLK_EXTCLK_GPIO_HSIOM;
	#endif /* CY_CFG_SYSCLK_EXTCLK_GPIO_HSIOM */

	#ifdef CY_CFG_SYSCLK_ILO_HIBERNATE
	secure_config->iloHibernateON = CY_CFG_SYSCLK_ILO_HIBERNATE;
	#endif /* CY_CFG_SYSCLK_ILO_HIBERNATE */

	#ifdef CY_CFG_SYSCLK_WCO_BYPASS
	secure_config->bypassEnable = CY_CFG_SYSCLK_WCO_BYPASS;
	#endif /* CY_CFG_SYSCLK_WCO_BYPASS */

	#ifdef CY_CFG_SYSCLK_WCO_IN_PRT
	secure_config->wcoInPort = CY_CFG_SYSCLK_WCO_IN_PRT;
	#endif /* CY_CFG_SYSCLK_WCO_IN_PRT */

	#ifdef CY_CFG_SYSCLK_WCO_OUT_PRT
	secure_config->wcoOutPort = CY_CFG_SYSCLK_WCO_OUT_PRT;
	#endif /* CY_CFG_SYSCLK_WCO_OUT_PRT */

	#ifdef CY_CFG_SYSCLK_WCO_IN_PIN
	secure_config->wcoInPinNum = CY_CFG_SYSCLK_WCO_IN_PIN;
	#endif /* CY_CFG_SYSCLK_WCO_IN_PIN */

	#ifdef CY_CFG_SYSCLK_WCO_OUT_PIN
	secure_config->wcoOutPinNum = CY_CFG_SYSCLK_WCO_OUT_PIN;
	#endif /* CY_CFG_SYSCLK_WCO_OUT_PIN */

	#ifdef CY_CFG_SYSCLK_FLL_OUT_FREQ
	secure_config->fllOutFreqHz = CY_CFG_SYSCLK_FLL_OUT_FREQ;
	#endif /* CY_CFG_SYSCLK_FLL_OUT_FREQ */

	#ifdef CY_CFG_SYSCLK_FLL_MULT
	secure_config->fllMult = CY_CFG_SYSCLK_FLL_MULT;
	#endif /* CY_CFG_SYSCLK_FLL_MULT */

	#ifdef CY_CFG_SYSCLK_FLL_REFDIV
	secure_config->fllRefDiv = CY_CFG_SYSCLK_FLL_REFDIV;
	#endif /* CY_CFG_SYSCLK_FLL_REFDIV */

	#ifdef CY_CFG_SYSCLK_FLL_CCO_RANGE
	secure_config->fllCcoRange = CY_CFG_SYSCLK_FLL_CCO_RANGE;
	#endif /* CY_CFG_SYSCLK_FLL_CCO_RANGE */

	#ifdef CY_CFG_SYSCLK_FLL_ENABLE_OUTDIV
	secure_config->enableOutputDiv = CY_CFG_SYSCLK_FLL_ENABLE_OUTDIV;
	#endif /* CY_CFG_SYSCLK_FLL_ENABLE_OUTDIV */

	#ifdef CY_CFG_SYSCLK_FLL_LOCK_TOLERANCE
	secure_config->lockTolerance = CY_CFG_SYSCLK_FLL_LOCK_TOLERANCE;
	#endif /* CY_CFG_SYSCLK_FLL_LOCK_TOLERANCE */

	#ifdef CY_CFG_SYSCLK_FLL_IGAIN
	secure_config->igain = CY_CFG_SYSCLK_FLL_IGAIN;
	#endif /* CY_CFG_SYSCLK_FLL_IGAIN */

	#ifdef CY_CFG_SYSCLK_FLL_PGAIN
	secure_config->pgain = CY_CFG_SYSCLK_FLL_PGAIN;
	#endif /* CY_CFG_SYSCLK_FLL_PGAIN */

	#ifdef CY_CFG_SYSCLK_FLL_SETTLING_COUNT
	secure_config->settlingCount = CY_CFG_SYSCLK_FLL_SETTLING_COUNT;
	#endif /* CY_CFG_SYSCLK_FLL_SETTLING_COUNT */

	#ifdef CY_CFG_SYSCLK_FLL_OUTPUT_MODE
	secure_config->outputMode = CY_CFG_SYSCLK_FLL_OUTPUT_MODE;
	#endif /* CY_CFG_SYSCLK_FLL_OUTPUT_MODE */

	#ifdef CY_CFG_SYSCLK_FLL_CCO_FREQ
	secure_config->ccoFreq = CY_CFG_SYSCLK_FLL_CCO_FREQ;
	#endif /* CY_CFG_SYSCLK_FLL_CCO_FREQ */

	#ifdef CY_CFG_SYSCLK_PLL0_FEEDBACK_DIV
	secure_config->pll0FeedbackDiv = CY_CFG_SYSCLK_PLL0_FEEDBACK_DIV;
	#endif /* CY_CFG_SYSCLK_PLL0_FEEDBACK_DIV */

	#ifdef CY_CFG_SYSCLK_PLL0_REFERENCE_DIV
	secure_config->pll0ReferenceDiv = CY_CFG_SYSCLK_PLL0_REFERENCE_DIV;
	#endif /* CY_CFG_SYSCLK_PLL0_REFERENCE_DIV */

	#ifdef CY_CFG_SYSCLK_PLL0_OUTPUT_DIV
	secure_config->pll0OutputDiv = CY_CFG_SYSCLK_PLL0_OUTPUT_DIV;
	#endif /* CY_CFG_SYSCLK_PLL0_OUTPUT_DIV */

	#ifdef CY_CFG_SYSCLK_PLL0_LF_MODE
	secure_config->pll0LfMode = CY_CFG_SYSCLK_PLL0_LF_MODE;
	#endif /* CY_CFG_SYSCLK_PLL0_LF_MODE */

	#ifdef CY_CFG_SYSCLK_PLL0_OUTPUT_MODE
	secure_config->pll0OutputMode = CY_CFG_SYSCLK_PLL0_OUTPUT_MODE;
	#endif /* CY_CFG_SYSCLK_PLL0_OUTPUT_MODE */

	#ifdef CY_CFG_SYSCLK_PLL0_OUTPUT_FREQ
	secure_config->pll0OutFreqHz = CY_CFG_SYSCLK_PLL0_OUTPUT_FREQ;
	#endif /* CY_CFG_SYSCLK_PLL0_OUTPUT_FREQ */

	#ifdef CY_CFG_SYSCLK_PLL1_FEEDBACK_DIV
	secure_config->pll1FeedbackDiv = CY_CFG_SYSCLK_PLL1_FEEDBACK_DIV;
	#endif /* CY_CFG_SYSCLK_PLL1_FEEDBACK_DIV */

	#ifdef CY_CFG_SYSCLK_PLL1_REFERENCE_DIV
	secure_config->pll1ReferenceDiv = CY_CFG_SYSCLK_PLL1_REFERENCE_DIV;
	#endif /* CY_CFG_SYSCLK_PLL1_REFERENCE_DIV */

	#ifdef CY_CFG_SYSCLK_PLL1_OUTPUT_DIV
	secure_config->pll1OutputDiv = CY_CFG_SYSCLK_PLL1_OUTPUT_DIV;
	#endif /* CY_CFG_SYSCLK_PLL1_OUTPUT_DIV */

	#ifdef CY_CFG_SYSCLK_PLL1_LF_MODE
	secure_config->pll1LfMode = CY_CFG_SYSCLK_PLL1_LF_MODE;
	#endif /* CY_CFG_SYSCLK_PLL1_LF_MODE */

	#ifdef CY_CFG_SYSCLK_PLL1_OUTPUT_MODE
	secure_config->pll1OutputMode = CY_CFG_SYSCLK_PLL1_OUTPUT_MODE;
	#endif /* CY_CFG_SYSCLK_PLL1_OUTPUT_MODE */

	#ifdef CY_CFG_SYSCLK_PLL1_OUTPUT_FREQ
	secure_config->pll1OutFreqHz = CY_CFG_SYSCLK_PLL1_OUTPUT_FREQ;
	#endif /* CY_CFG_SYSCLK_PLL1_OUTPUT_FREQ */

	#ifdef CY_CFG_SYSCLK_CLKPATH0_SOURCE
	secure_config->path0Src = CY_CFG_SYSCLK_CLKPATH0_SOURCE;
	#endif /* CY_CFG_SYSCLK_CLKPATH0_SOURCE */

	#ifdef CY_CFG_SYSCLK_CLKPATH1_SOURCE
	secure_config->path1Src = CY_CFG_SYSCLK_CLKPATH1_SOURCE;
	#endif /* CY_CFG_SYSCLK_CLKPATH1_SOURCE */

	#ifdef CY_CFG_SYSCLK_CLKPATH2_SOURCE
	secure_config->path2Src = CY_CFG_SYSCLK_CLKPATH2_SOURCE;
	#endif /* CY_CFG_SYSCLK_CLKPATH2_SOURCE */

	#ifdef CY_CFG_SYSCLK_CLKPATH3_SOURCE
	secure_config->path3Src = CY_CFG_SYSCLK_CLKPATH3_SOURCE;
	#endif /* CY_CFG_SYSCLK_CLKPATH3_SOURCE */

	#ifdef CY_CFG_SYSCLK_CLKPATH4_SOURCE
	secure_config->path4Src = CY_CFG_SYSCLK_CLKPATH4_SOURCE;
	#endif /* CY_CFG_SYSCLK_CLKPATH4_SOURCE */

	#ifdef CY_CFG_SYSCLK_CLKPATH5_SOURCE
	secure_config->path5Src = CY_CFG_SYSCLK_CLKPATH5_SOURCE;
	#endif /* CY_CFG_SYSCLK_CLKPATH5_SOURCE */

	#ifdef CY_CFG_SYSCLK_CLKFAST_DIVIDER
	secure_config->clkFastDiv = CY_CFG_SYSCLK_CLKFAST_DIVIDER;
	#endif /* CY_CFG_SYSCLK_CLKFAST_DIVIDER */

	#ifdef CY_CFG_SYSCLK_CLKPERI_DIVIDER
	secure_config->clkPeriDiv = CY_CFG_SYSCLK_CLKPERI_DIVIDER;
	#endif /* CY_CFG_SYSCLK_CLKPERI_DIVIDER */

	#ifdef CY_CFG_SYSCLK_CLKSLOW_DIVIDER
	secure_config->clkSlowDiv = CY_CFG_SYSCLK_CLKSLOW_DIVIDER;
	#endif /* CY_CFG_SYSCLK_CLKSLOW_DIVIDER */

	#ifdef CY_CFG_SYSCLK_CLKHF0_CLKPATH
	secure_config->hf0Source = CY_CFG_SYSCLK_CLKHF0_CLKPATH;
	#endif /* CY_CFG_SYSCLK_CLKHF0_CLKPATH */

	#ifdef CY_CFG_SYSCLK_CLKHF0_DIVIDER
	secure_config->hf0Divider = CY_CFG_SYSCLK_CLKHF0_DIVIDER;
	#endif /* CY_CFG_SYSCLK_CLKHF0_DIVIDER */

	#ifdef CY_CFG_SYSCLK_CLKHF0_FREQ_MHZ
	secure_config->hf0OutFreqMHz = CY_CFG_SYSCLK_CLKHF0_FREQ_MHZ;
	#endif /* CY_CFG_SYSCLK_CLKHF0_FREQ_MHZ */

	#ifdef CY_CFG_SYSCLK_CLKHF1_CLKPATH
	secure_config->hf1Source = CY_CFG_SYSCLK_CLKHF1_CLKPATH;
	#endif /* CY_CFG_SYSCLK_CLKHF1_CLKPATH */

	#ifdef CY_CFG_SYSCLK_CLKHF1_DIVIDER
	secure_config->hf1Divider = CY_CFG_SYSCLK_CLKHF1_DIVIDER;
	#endif /* CY_CFG_SYSCLK_CLKHF1_DIVIDER */

	#ifdef CY_CFG_SYSCLK_CLKHF1_FREQ_MHZ
	secure_config->hf1OutFreqMHz = CY_CFG_SYSCLK_CLKHF1_FREQ_MHZ;
	#endif /* CY_CFG_SYSCLK_CLKHF1_FREQ_MHZ */

	#ifdef CY_CFG_SYSCLK_CLKHF2_CLKPATH
	secure_config->hf2Source = CY_CFG_SYSCLK_CLKHF2_CLKPATH;
	#endif /* CY_CFG_SYSCLK_CLKHF2_CLKPATH */

	#ifdef CY_CFG_SYSCLK_CLKHF2_DIVIDER
	secure_config->hf2Divider = CY_CFG_SYSCLK_CLKHF2_DIVIDER;
	#endif /* CY_CFG_SYSCLK_CLKHF2_DIVIDER */

	#ifdef CY_CFG_SYSCLK_CLKHF2_FREQ_MHZ
	secure_config->hf2OutFreqMHz = CY_CFG_SYSCLK_CLKHF2_FREQ_MHZ;
	#endif /* CY_CFG_SYSCLK_CLKHF2_FREQ_MHZ */

	#ifdef CY_CFG_SYSCLK_CLKHF3_CLKPATH
	secure_config->hf3Source = CY_CFG_SYSCLK_CLKHF3_CLKPATH;
	#endif /* CY_CFG_SYSCLK_CLKHF3_CLKPATH */

	#ifdef CY_CFG_SYSCLK_CLKHF3_DIVIDER
	secure_config->hf3Divider = CY_CFG_SYSCLK_CLKHF3_DIVIDER;
	#endif /* CY_CFG_SYSCLK_CLKHF3_DIVIDER */

	#ifdef CY_CFG_SYSCLK_CLKHF3_FREQ_MHZ
	secure_config->hf3OutFreqMHz = CY_CFG_SYSCLK_CLKHF3_FREQ_MHZ;
	#endif /* CY_CFG_SYSCLK_CLKHF3_FREQ_MHZ */

	#ifdef CY_CFG_SYSCLK_CLKHF4_CLKPATH
	secure_config->hf4Source = CY_CFG_SYSCLK_CLKHF4_CLKPATH;
	#endif /* CY_CFG_SYSCLK_CLKHF4_CLKPATH */

	#ifdef CY_CFG_SYSCLK_CLKHF4_DIVIDER
	secure_config->hf4Divider = CY_CFG_SYSCLK_CLKHF4_DIVIDER;
	#endif /* CY_CFG_SYSCLK_CLKHF4_DIVIDER */

	#ifdef CY_CFG_SYSCLK_CLKHF4_FREQ_MHZ
	secure_config->hf4OutFreqMHz = CY_CFG_SYSCLK_CLKHF4_FREQ_MHZ;
	#endif /* CY_CFG_SYSCLK_CLKHF4_FREQ_MHZ */

	#ifdef CY_CFG_SYSCLK_CLKHF5_CLKPATH
	secure_config->hf5Source = CY_CFG_SYSCLK_CLKHF5_CLKPATH;
	#endif /* CY_CFG_SYSCLK_CLKHF5_CLKPATH */

	#ifdef CY_CFG_SYSCLK_CLKHF5_DIVIDER
	secure_config->hf5Divider = CY_CFG_SYSCLK_CLKHF5_DIVIDER;
	#endif /* CY_CFG_SYSCLK_CLKHF5_DIVIDER */

	#ifdef CY_CFG_SYSCLK_CLKHF5_FREQ_MHZ
	secure_config->hf5OutFreqMHz = CY_CFG_SYSCLK_CLKHF5_FREQ_MHZ;
	#endif /* CY_CFG_SYSCLK_CLKHF5_FREQ_MHZ */

	#ifdef CY_CFG_SYSCLK_CLKPUMP_SOURCE
	secure_config->pumpSource = CY_CFG_SYSCLK_CLKPUMP_SOURCE;
	#endif /* CY_CFG_SYSCLK_CLKPUMP_SOURCE */

	#ifdef CY_CFG_SYSCLK_CLKPUMP_DIVIDER
	secure_config->pumpDivider = CY_CFG_SYSCLK_CLKPUMP_DIVIDER;
	#endif /* CY_CFG_SYSCLK_CLKPUMP_DIVIDER */

	#ifdef CY_CFG_SYSCLK_CLKLF_SOURCE
	secure_config->clkLfSource = CY_CFG_SYSCLK_CLKLF_SOURCE;
	#endif /* CY_CFG_SYSCLK_CLKLF_SOURCE */

	#ifdef CY_CFG_SYSCLK_CLKBAK_SOURCE
	secure_config->clkBakSource = CY_CFG_SYSCLK_CLKBAK_SOURCE;
	#endif /* CY_CFG_SYSCLK_CLKBAK_SOURCE */

	#ifdef CY_CFG_SYSCLK_CLKTIMER_SOURCE
	secure_config->clkTimerSource = CY_CFG_SYSCLK_CLKTIMER_SOURCE;
	#endif /* CY_CFG_SYSCLK_CLKTIMER_SOURCE */

	#ifdef CY_CFG_SYSCLK_CLKTIMER_DIVIDER
	secure_config->clkTimerDivider = CY_CFG_SYSCLK_CLKTIMER_DIVIDER;
	#endif /* CY_CFG_SYSCLK_CLKTIMER_DIVIDER */

	#ifdef CY_CFG_SYSCLK_CLKALTSYSTICK_SOURCE
	secure_config->clkSrcAltSysTick = CY_CFG_SYSCLK_CLKALTSYSTICK_SOURCE;
	#endif /* CY_CFG_SYSCLK_CLKALTSYSTICK_SOURCE */

	#ifdef CY_CFG_SYSCLK_ALTHF_BLE_ECO_CLOAD
	secure_config->altHFcLoad = CY_CFG_SYSCLK_ALTHF_BLE_ECO_CLOAD;
	#endif /* CY_CFG_SYSCLK_ALTHF_BLE_ECO_CLOAD */

	#ifdef CY_CFG_SYSCLK_ALTHF_BLE_ECO_TIME
	secure_config->altHFxtalStartUpTime = CY_CFG_SYSCLK_ALTHF_BLE_ECO_TIME;
	#endif /* CY_CFG_SYSCLK_ALTHF_BLE_ECO_TIME */

	#ifdef CY_CFG_SYSCLK_ALTHF_BLE_ECO_FREQ
	secure_config->altHFclkFreq = CY_CFG_SYSCLK_ALTHF_BLE_ECO_FREQ;
	#endif /* CY_CFG_SYSCLK_ALTHF_BLE_ECO_FREQ */

	#ifdef CY_CFG_SYSCLK_ALTHF_BLE_ECO_CLK_DIV
	secure_config->altHFsysClkDiv = CY_CFG_SYSCLK_ALTHF_BLE_ECO_CLK_DIV;
	#endif /* CY_CFG_SYSCLK_ALTHF_BLE_ECO_CLK_DIV */

	#ifdef CY_CFG_SYSCLK_ALTHF_BLE_ECO_VOL_REGULATOR
	secure_config->altHFvoltageReg = CY_CFG_SYSCLK_ALTHF_BLE_ECO_VOL_REGULATOR;
	#endif  /* CY_CFG_SYSCLK_ALTHF_BLE_ECO_VOL_REGULATOR */
}
#endif          // defined (CY_DEVICE_SECURE)
#if (!defined(CY_DEVICE_SECURE))
__STATIC_INLINE void Cy_SysClk_ClkBakInit()
{
	Cy_SysClk_ClkBakSetSource(CY_SYSCLK_BAK_IN_WCO);
}
#endif // (!defined(CY_DEVICE_SECURE))
#if (!defined(CY_DEVICE_SECURE))
__STATIC_INLINE void Cy_SysClk_ClkFastInit()
{
	Cy_SysClk_ClkFastSetDivider(0U);
}
#endif // (!defined(CY_DEVICE_SECURE))
#if (!defined(CY_DEVICE_SECURE))
__STATIC_INLINE void Cy_SysClk_FllInit()
{
	if (CY_SYSCLK_SUCCESS != Cy_SysClk_FllManualConfigure(&srss_0_clock_0_fll_0_fllConfig)) {
		cycfg_ClockStartupError(CY_CFG_SYSCLK_FLL_ERROR);
	}
	if (CY_SYSCLK_SUCCESS != Cy_SysClk_FllEnable(200000UL)) {
		cycfg_ClockStartupError(CY_CFG_SYSCLK_FLL_ERROR);
	}
}
#endif // (!defined(CY_DEVICE_SECURE))
#if (!defined(CY_DEVICE_SECURE))
__STATIC_INLINE void Cy_SysClk_ClkHf0Init()
{
	Cy_SysClk_ClkHfSetSource(0U, CY_CFG_SYSCLK_CLKHF0_CLKPATH);
	Cy_SysClk_ClkHfSetDivider(0U, CY_SYSCLK_CLKHF_NO_DIVIDE);
}
#endif // (!defined(CY_DEVICE_SECURE))
#if (!defined(CY_DEVICE_SECURE))
__STATIC_INLINE void Cy_SysClk_IloInit()
{
	/* The WDT is unlocked in the default startup code */
	Cy_SysClk_IloEnable();
	Cy_SysClk_IloHibernateOn(true);
}
#endif // (!defined(CY_DEVICE_SECURE))
#if (!defined(CY_DEVICE_SECURE))
__STATIC_INLINE void Cy_SysClk_ClkLfInit()
{
	/* The WDT is unlocked in the default startup code */
	Cy_SysClk_ClkLfSetSource(CY_SYSCLK_CLKLF_IN_WCO);
}
#endif // (!defined(CY_DEVICE_SECURE))
#if (!defined(CY_DEVICE_SECURE))
__STATIC_INLINE void Cy_SysClk_ClkPath0Init()
{
	Cy_SysClk_ClkPathSetSource(0U, CY_CFG_SYSCLK_CLKPATH0_SOURCE);
}
#endif // (!defined(CY_DEVICE_SECURE))
#if (!defined(CY_DEVICE_SECURE))
__STATIC_INLINE void Cy_SysClk_ClkPath1Init()
{
	Cy_SysClk_ClkPathSetSource(1U, CY_CFG_SYSCLK_CLKPATH1_SOURCE);
}
#endif // (!defined(CY_DEVICE_SECURE))
#if (!defined(CY_DEVICE_SECURE))
__STATIC_INLINE void Cy_SysClk_ClkPath2Init()
{
	Cy_SysClk_ClkPathSetSource(2U, CY_CFG_SYSCLK_CLKPATH2_SOURCE);
}
#endif // (!defined(CY_DEVICE_SECURE))
#if (!defined(CY_DEVICE_SECURE))
__STATIC_INLINE void Cy_SysClk_ClkPath3Init()
{
	Cy_SysClk_ClkPathSetSource(3U, CY_CFG_SYSCLK_CLKPATH3_SOURCE);
}
#endif // (!defined(CY_DEVICE_SECURE))
#if (!defined(CY_DEVICE_SECURE))
__STATIC_INLINE void Cy_SysClk_ClkPath4Init()
{
	Cy_SysClk_ClkPathSetSource(4U, CY_CFG_SYSCLK_CLKPATH4_SOURCE);
}
#endif // (!defined(CY_DEVICE_SECURE))
#if (!defined(CY_DEVICE_SECURE))
__STATIC_INLINE void Cy_SysClk_ClkPath5Init()
{
	Cy_SysClk_ClkPathSetSource(5U, CY_CFG_SYSCLK_CLKPATH5_SOURCE);
}
#endif // (!defined(CY_DEVICE_SECURE))
#if (!defined(CY_DEVICE_SECURE))
__STATIC_INLINE void Cy_SysClk_ClkPeriInit()
{
	Cy_SysClk_ClkPeriSetDivider(0U);
}
#endif // (!defined(CY_DEVICE_SECURE))
#if (!defined(CY_DEVICE_SECURE))
__STATIC_INLINE void Cy_SysClk_Pll0Init()
{
	if (CY_SYSCLK_SUCCESS != Cy_SysClk_PllManualConfigure(1U, &srss_0_clock_0_pll_0_pllConfig)) {
		cycfg_ClockStartupError(CY_CFG_SYSCLK_PLL_ERROR);
	}
	if (CY_SYSCLK_SUCCESS != Cy_SysClk_PllEnable(1U, 10000u)) {
		cycfg_ClockStartupError(CY_CFG_SYSCLK_PLL_ERROR);
	}
}
#endif // (!defined(CY_DEVICE_SECURE))
#if (!defined(CY_DEVICE_SECURE))
__STATIC_INLINE void Cy_SysClk_ClkSlowInit()
{
	Cy_SysClk_ClkSlowSetDivider(0U);
}
#endif // (!defined(CY_DEVICE_SECURE))
#if (!defined(CY_DEVICE_SECURE))
__STATIC_INLINE void Cy_SysClk_WcoInit()
{
	(void)Cy_GPIO_Pin_FastInit(GPIO_PRT0, 0U, 0x00U, 0x00U, HSIOM_SEL_GPIO);
	(void)Cy_GPIO_Pin_FastInit(GPIO_PRT0, 1U, 0x00U, 0x00U, HSIOM_SEL_GPIO);
	if (CY_SYSCLK_SUCCESS != Cy_SysClk_WcoEnable(1000000UL)) {
		cycfg_ClockStartupError(CY_CFG_SYSCLK_WCO_ERROR);
	}
}
#endif // (!defined(CY_DEVICE_SECURE))
#if (!defined(CY_DEVICE_SECURE))
__STATIC_INLINE void init_cycfg_power(void)
{
	/* Reset the Backup domain on POR, XRES, BOD only if Backup domain is supplied by VDDD */
	#if (CY_CFG_PWR_VBACKUP_USING_VDDD)
	    #ifdef CY_CFG_SYSCLK_ILO_ENABLED
	if (0u == Cy_SysLib_GetResetReason() /* POR, XRES, or BOD */) {
		Cy_SysLib_ResetBackupDomain();
		Cy_SysClk_IloDisable();
		Cy_SysClk_IloInit();
	}
	    #endif      /* CY_CFG_SYSCLK_ILO_ENABLED */
	#endif          /* CY_CFG_PWR_VBACKUP_USING_VDDD */
	/* Configure core regulator */
	#if !(defined(CY_DEVICE_SECURE))
	    #if defined (CY_IP_M4CPUSS)
		#if CY_CFG_PWR_USING_LDO
	Cy_SysPm_LdoSetVoltage(CY_SYSPM_LDO_VOLTAGE_LP);
		#else
	Cy_SysPm_BuckEnable(CY_SYSPM_BUCK_OUT1_VOLTAGE_LP);
		#endif  /* CY_CFG_PWR_USING_LDO */
	    #endif      /* defined (CY_IP_M4CPUSS) */
	    #if CY_CFG_PWR_REGULATOR_MODE_MIN
	Cy_SysPm_SystemSetMinRegulatorCurrent();
	    #else
	Cy_SysPm_SystemSetNormalRegulatorCurrent();
	    #endif      /* CY_CFG_PWR_REGULATOR_MODE_MIN */
	#endif          /* !(defined(CY_DEVICE_SECURE)) */
	/* Configure PMIC */
	Cy_SysPm_UnlockPmic();
	#if CY_CFG_PWR_USING_PMIC
	Cy_SysPm_PmicEnableOutput();
	#else
	Cy_SysPm_PmicDisableOutput();
	#endif  /* CY_CFG_PWR_USING_PMIC */
}
#endif          // (!defined(CY_DEVICE_SECURE))


void init_cycfg_system(void)
{
    #if defined(CY_DEVICE_SECURE)
	cy_en_pra_status_t configStatus;
	init_cycfg_secure_struct(&srss_0_clock_0_secureConfig);
	#if (((CY_CFG_SYSCLK_CLKPATH0_SOURCE_NUM >= 3UL) && (CY_CFG_SYSCLK_CLKPATH0_SOURCE_NUM <= 5UL))  && (CY_CFG_SYSCLK_CLKHF0_CLKPATH_NUM == 0UL))
	    #error Configuration Error : ALTHF, ILO, PILO cannot drive HF0.
	#endif
	#if (((CY_CFG_SYSCLK_CLKPATH1_SOURCE_NUM >= 3UL) && (CY_CFG_SYSCLK_CLKPATH1_SOURCE_NUM <= 5UL)) && (CY_CFG_SYSCLK_CLKHF0_CLKPATH_NUM == 1UL))
	    #error Configuration Error : ALTHF, ILO, PILO cannot drive HF0.
	#endif
	#if (((CY_CFG_SYSCLK_CLKPATH2_SOURCE_NUM >= 3UL) && (CY_CFG_SYSCLK_CLKPATH2_SOURCE_NUM <= 5UL)) && (CY_CFG_SYSCLK_CLKHF0_CLKPATH_NUM == 2UL))
	    #error Configuration Error : ALTHF, ILO, PILO cannot drive HF0.
	#endif
	#if (((CY_CFG_SYSCLK_CLKPATH3_SOURCE_NUM >= 3UL) && (CY_CFG_SYSCLK_CLKPATH3_SOURCE_NUM <= 5UL)) && (CY_CFG_SYSCLK_CLKHF0_CLKPATH_NUM == 3UL))
	    #error Configuration Error : ALTHF, ILO, PILO cannot drive HF0.
	#endif
	#if (((CY_CFG_SYSCLK_CLKPATH4_SOURCE_NUM >= 3UL) && (CY_CFG_SYSCLK_CLKPATH4_SOURCE_NUM <= 5UL)) && (CY_CFG_SYSCLK_CLKHF0_CLKPATH_NUM == 4UL))
	    #error Configuration Error : ALTHF, ILO, PILO cannot drive HF0.
	#endif
	#if (((CY_CFG_SYSCLK_CLKPATH5_SOURCE_NUM >= 3UL) && (CY_CFG_SYSCLK_CLKPATH5_SOURCE_NUM <= 5UL)) && (CY_CFG_SYSCLK_CLKHF0_CLKPATH_NUM == 5UL))
	    #error Configuration Error : ALTHF, ILO, PILO cannot drive HF0.
	#endif

	configStatus = CY_PRA_FUNCTION_CALL_RETURN_PARAM(CY_PRA_MSG_TYPE_SYS_CFG_FUNC,
							 CY_PRA_FUNC_INIT_CYCFG_DEVICE,
							 &srss_0_clock_0_secureConfig);
	if (configStatus != CY_PRA_STATUS_SUCCESS) {
		cycfg_ClockStartupError(configStatus);
	}

	#ifdef CY_CFG_SYSCLK_EXTCLK_FREQ
	Cy_SysClk_ExtClkSetFrequency(CY_CFG_SYSCLK_EXTCLK_FREQ);
	#endif /* CY_CFG_SYSCLK_EXTCLK_FREQ */
    #else /* defined(CY_DEVICE_SECURE) */

	/* Set worst case memory wait states (! ultra low power, 150 MHz), will update at the end */
	Cy_SysLib_SetWaitStates(false, 150UL);
	#ifdef CY_CFG_PWR_ENABLED
	    #ifdef CY_CFG_PWR_INIT
	init_cycfg_power();
	    #else
		#warning Power system will not be configured. Update power personality to v1.20 or later.
	    #endif      /* CY_CFG_PWR_INIT */
	#endif          /* CY_CFG_PWR_ENABLED */

	/* Reset the core clock path to default and disable all the FLLs/PLLs */
	Cy_SysClk_ClkHfSetDivider(0U, CY_SYSCLK_CLKHF_NO_DIVIDE);
	Cy_SysClk_ClkFastSetDivider(0U);
	Cy_SysClk_ClkPeriSetDivider(1U);
	Cy_SysClk_ClkSlowSetDivider(0U);
	for (uint32_t pll = CY_SRSS_NUM_PLL; pll > 0UL; --pll) { /* PLL 1 is the first PLL. 0 is invalid. */
		(void)Cy_SysClk_PllDisable(pll);
	}
	Cy_SysClk_ClkPathSetSource(CY_SYSCLK_CLKHF_IN_CLKPATH1, CY_SYSCLK_CLKPATH_IN_IMO);

	if ((CY_SYSCLK_CLKHF_IN_CLKPATH0 == Cy_SysClk_ClkHfGetSource(0UL)) &&
	    (CY_SYSCLK_CLKPATH_IN_WCO == Cy_SysClk_ClkPathGetSource(CY_SYSCLK_CLKHF_IN_CLKPATH0))) {
		Cy_SysClk_ClkHfSetSource(0U, CY_SYSCLK_CLKHF_IN_CLKPATH1);
	}

	Cy_SysClk_FllDisable();
	Cy_SysClk_ClkPathSetSource(CY_SYSCLK_CLKHF_IN_CLKPATH0, CY_SYSCLK_CLKPATH_IN_IMO);
	Cy_SysClk_ClkHfSetSource(0UL, CY_SYSCLK_CLKHF_IN_CLKPATH0);
	#ifdef CY_IP_MXBLESS
	(void)Cy_BLE_EcoReset();
	#endif


	/* Enable all source clocks */
	#ifdef CY_CFG_SYSCLK_PILO_ENABLED
	Cy_SysClk_PiloInit();
	#endif

	#ifdef CY_CFG_SYSCLK_WCO_ENABLED
	Cy_SysClk_WcoInit();
	#endif

	#ifdef CY_CFG_SYSCLK_CLKLF_ENABLED
	Cy_SysClk_ClkLfInit();
	#endif

	#if (defined(CY_IP_M4CPUSS) && CY_CFG_SYSCLK_ALTHF_ENABLED)

	Cy_SysClk_AltHfInit();
	#endif /* (defined(CY_IP_M4CPUSS) && CY_CFG_SYSCLK_ALTHF_ENABLED */


	#ifdef CY_CFG_SYSCLK_ECO_ENABLED
	Cy_SysClk_EcoInit();
	#endif

	#ifdef CY_CFG_SYSCLK_EXTCLK_ENABLED
	Cy_SysClk_ExtClkInit();
	#endif

	/* Configure CPU clock dividers */
	#ifdef CY_CFG_SYSCLK_CLKFAST_ENABLED
	Cy_SysClk_ClkFastInit();
	#endif

	#ifdef CY_CFG_SYSCLK_CLKPERI_ENABLED
	Cy_SysClk_ClkPeriInit();
	#endif

	#ifdef CY_CFG_SYSCLK_CLKSLOW_ENABLED
	Cy_SysClk_ClkSlowInit();
	#endif

	#if ((CY_CFG_SYSCLK_CLKPATH0_SOURCE_NUM == 0x6UL) && (CY_CFG_SYSCLK_CLKHF0_CLKPATH_NUM == 0U))
	/* Configure HFCLK0 to temporarily run from IMO to initialize other clocks */
	Cy_SysClk_ClkPathSetSource(1UL, CY_SYSCLK_CLKPATH_IN_IMO);
	Cy_SysClk_ClkHfSetSource(0UL, CY_SYSCLK_CLKHF_IN_CLKPATH1);
	#else
	    #ifdef CY_CFG_SYSCLK_CLKPATH1_ENABLED
	Cy_SysClk_ClkPath1Init();
	    #endif
	#endif

	/* Configure Path Clocks */
	#ifdef CY_CFG_SYSCLK_CLKPATH0_ENABLED
	Cy_SysClk_ClkPath0Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKPATH2_ENABLED
	Cy_SysClk_ClkPath2Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKPATH3_ENABLED
	Cy_SysClk_ClkPath3Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKPATH4_ENABLED
	Cy_SysClk_ClkPath4Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKPATH5_ENABLED
	Cy_SysClk_ClkPath5Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKPATH6_ENABLED
	Cy_SysClk_ClkPath6Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKPATH7_ENABLED
	Cy_SysClk_ClkPath7Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKPATH8_ENABLED
	Cy_SysClk_ClkPath8Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKPATH9_ENABLED
	Cy_SysClk_ClkPath9Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKPATH10_ENABLED
	Cy_SysClk_ClkPath10Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKPATH11_ENABLED
	Cy_SysClk_ClkPath11Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKPATH12_ENABLED
	Cy_SysClk_ClkPath12Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKPATH13_ENABLED
	Cy_SysClk_ClkPath13Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKPATH14_ENABLED
	Cy_SysClk_ClkPath14Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKPATH15_ENABLED
	Cy_SysClk_ClkPath15Init();
	#endif

	/* Configure and enable FLL */
	#ifdef CY_CFG_SYSCLK_FLL_ENABLED
	Cy_SysClk_FllInit();
	#endif

	Cy_SysClk_ClkHf0Init();

	#if ((CY_CFG_SYSCLK_CLKPATH0_SOURCE_NUM == 0x6UL) && (CY_CFG_SYSCLK_CLKHF0_CLKPATH_NUM == 0U))
	    #ifdef CY_CFG_SYSCLK_CLKPATH1_ENABLED
	/* Apply the ClkPath1 user setting */
	Cy_SysClk_ClkPath1Init();
	    #endif
	#endif

	/* Configure and enable PLLs */
	#ifdef CY_CFG_SYSCLK_PLL0_ENABLED
	Cy_SysClk_Pll0Init();
	#endif
	#ifdef CY_CFG_SYSCLK_PLL1_ENABLED
	Cy_SysClk_Pll1Init();
	#endif
	#ifdef CY_CFG_SYSCLK_PLL2_ENABLED
	Cy_SysClk_Pll2Init();
	#endif
	#ifdef CY_CFG_SYSCLK_PLL3_ENABLED
	Cy_SysClk_Pll3Init();
	#endif
	#ifdef CY_CFG_SYSCLK_PLL4_ENABLED
	Cy_SysClk_Pll4Init();
	#endif
	#ifdef CY_CFG_SYSCLK_PLL5_ENABLED
	Cy_SysClk_Pll5Init();
	#endif
	#ifdef CY_CFG_SYSCLK_PLL6_ENABLED
	Cy_SysClk_Pll6Init();
	#endif
	#ifdef CY_CFG_SYSCLK_PLL7_ENABLED
	Cy_SysClk_Pll7Init();
	#endif
	#ifdef CY_CFG_SYSCLK_PLL8_ENABLED
	Cy_SysClk_Pll8Init();
	#endif
	#ifdef CY_CFG_SYSCLK_PLL9_ENABLED
	Cy_SysClk_Pll9Init();
	#endif
	#ifdef CY_CFG_SYSCLK_PLL10_ENABLED
	Cy_SysClk_Pll10Init();
	#endif
	#ifdef CY_CFG_SYSCLK_PLL11_ENABLED
	Cy_SysClk_Pll11Init();
	#endif
	#ifdef CY_CFG_SYSCLK_PLL12_ENABLED
	Cy_SysClk_Pll12Init();
	#endif
	#ifdef CY_CFG_SYSCLK_PLL13_ENABLED
	Cy_SysClk_Pll13Init();
	#endif
	#ifdef CY_CFG_SYSCLK_PLL14_ENABLED
	Cy_SysClk_Pll14Init();
	#endif

	/* Configure HF clocks */
	#ifdef CY_CFG_SYSCLK_CLKHF1_ENABLED
	Cy_SysClk_ClkHf1Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKHF2_ENABLED
	Cy_SysClk_ClkHf2Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKHF3_ENABLED
	Cy_SysClk_ClkHf3Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKHF4_ENABLED
	Cy_SysClk_ClkHf4Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKHF5_ENABLED
	Cy_SysClk_ClkHf5Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKHF6_ENABLED
	Cy_SysClk_ClkHf6Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKHF7_ENABLED
	Cy_SysClk_ClkHf7Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKHF8_ENABLED
	Cy_SysClk_ClkHf8Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKHF9_ENABLED
	Cy_SysClk_ClkHf9Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKHF10_ENABLED
	Cy_SysClk_ClkHf10Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKHF11_ENABLED
	Cy_SysClk_ClkHf11Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKHF12_ENABLED
	Cy_SysClk_ClkHf12Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKHF13_ENABLED
	Cy_SysClk_ClkHf13Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKHF14_ENABLED
	Cy_SysClk_ClkHf14Init();
	#endif
	#ifdef CY_CFG_SYSCLK_CLKHF15_ENABLED
	Cy_SysClk_ClkHf15Init();
	#endif

	/* Configure miscellaneous clocks */
	#ifdef CY_CFG_SYSCLK_CLKTIMER_ENABLED
	Cy_SysClk_ClkTimerInit();
	#endif

	#ifdef CY_CFG_SYSCLK_CLKALTSYSTICK_ENABLED
	Cy_SysClk_ClkAltSysTickInit();
	#endif

	#ifdef CY_CFG_SYSCLK_CLKPUMP_ENABLED
	Cy_SysClk_ClkPumpInit();
	#endif

	#ifdef CY_CFG_SYSCLK_CLKBAK_ENABLED
	Cy_SysClk_ClkBakInit();
	#endif

	/* Configure default enabled clocks */
	#ifdef CY_CFG_SYSCLK_ILO_ENABLED
	Cy_SysClk_IloInit();
	#endif

	#ifndef CY_CFG_SYSCLK_IMO_ENABLED
	    #error the IMO must be enabled for proper chip operation
	#endif

	#ifndef CY_CFG_SYSCLK_CLKHF0_ENABLED
	    #error the CLKHF0 must be enabled for proper chip operation
	#endif

    #endif /* defined(CY_DEVICE_SECURE) */

    #ifdef CY_CFG_SYSCLK_MFO_ENABLED
	Cy_SysClk_MfoInit();
    #endif

    #ifdef CY_CFG_SYSCLK_CLKMF_ENABLED
	Cy_SysClk_ClkMfInit();
    #endif

    #if (!defined(CY_DEVICE_SECURE))
	/* Set accurate flash wait states */
	#if (defined (CY_CFG_PWR_ENABLED) && defined (CY_CFG_SYSCLK_CLKHF0_ENABLED))
	Cy_SysLib_SetWaitStates(CY_CFG_PWR_USING_ULP != 0, CY_CFG_SYSCLK_CLKHF0_FREQ_MHZ);
	#endif

	/* Update System Core Clock values for correct Cy_SysLib_Delay functioning */
	SystemCoreClockUpdate();
	#ifndef CY_CFG_SYSCLK_ILO_ENABLED
	    #ifdef CY_CFG_SYSCLK_CLKLF_ENABLED
	/* Wait 4 ILO cycles in case of unfinished CLKLF clock source transition */
	Cy_SysLib_DelayUs(200U);
	    #endif
	Cy_SysClk_IloDisable();
	Cy_SysClk_IloHibernateOn(false);
	#endif

    #endif /* (!defined(CY_DEVICE_SECURE)) */


#if defined (CY_USING_HAL)
	cyhal_hwmgr_reserve(&srss_0_clock_0_pathmux_0_obj);
#endif // defined (CY_USING_HAL)

#if defined (CY_USING_HAL)
	cyhal_hwmgr_reserve(&srss_0_clock_0_pathmux_1_obj);
#endif // defined (CY_USING_HAL)

#if defined (CY_USING_HAL)
	cyhal_hwmgr_reserve(&srss_0_clock_0_pathmux_2_obj);
#endif // defined (CY_USING_HAL)

#if defined (CY_USING_HAL)
	cyhal_hwmgr_reserve(&srss_0_clock_0_pathmux_3_obj);
#endif // defined (CY_USING_HAL)

#if defined (CY_USING_HAL)
	cyhal_hwmgr_reserve(&srss_0_clock_0_pathmux_4_obj);
#endif // defined (CY_USING_HAL)

#if defined (CY_USING_HAL)
	cyhal_hwmgr_reserve(&srss_0_clock_0_pathmux_5_obj);
#endif // defined (CY_USING_HAL)
}
