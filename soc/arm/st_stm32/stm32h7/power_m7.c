/*
 * Copyright (c) 2021 SILA Embedded Solutions GmbH <office@embedded-solutions.at>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <pm/pm.h>
#include <soc.h>
#include <init.h>

#include <stm32h7xx_ll_pwr.h>
#include <stm32h7xx_ll_cortex.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#if !defined(SMPS) && \
		(defined(CONFIG_POWER_SUPPLY_DIRECT_SMPS) || \
		defined(CONFIG_POWER_SUPPLY_SMPS_1V8_SUPPLIES_LDO) || \
		defined(CONFIG_POWER_SUPPLY_SMPS_2V5_SUPPLIES_LDO) || \
		defined(CONFIG_POWER_SUPPLY_SMPS_1V8_SUPPLIES_EXT_AND_LDO) || \
		defined(CONFIG_POWER_SUPPLY_SMPS_2V5_SUPPLIES_EXT_AND_LDO) || \
		defined(CONFIG_POWER_SUPPLY_SMPS_1V8_SUPPLIES_EXT) || \
		defined(CONFIG_POWER_SUPPLY_SMPS_2V5_SUPPLIES_EXT))
#error Unsupported configuration: Selected SoC do not support SMPS
#endif

#if defined(CONFIG_VOS0_SWITCH_METHOD_DIRECT)
static void activate_vos0(void)
{
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE0);

	/*
	 * We have to wait for ACTVOSRDY and VOSRDY at this point,
	 * as voltage scaling was changed from VOS3 to VOS0.
	 */
	while (LL_PWR_IsActiveFlag_VOS() == 0) {
	}
	while (LL_PWR_IsActiveFlag_ACTVOS() == 0) {
	}
}
#elif defined(CONFIG_VOS0_SWITCH_METHOD_ODEN)
static void activate_vos0(void)
{
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);

	while (LL_PWR_IsActiveFlag_VOS() == 0) {
	}

	SET_BIT(SYSCFG->PWRCR, SYSCFG_PWRCR_ODEN);

	while (LL_PWR_IsActiveFlag_VOS() == 0) {
	}
}
#else
#error switch into VOS0 not implemented
#endif /* CONFIG_VOS0_SWITCH_METHOD_* */

static int stm32_power_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#if defined(CONFIG_POWER_SUPPLY_DIRECT_SMPS)
	LL_PWR_ConfigSupply(LL_PWR_DIRECT_SMPS_SUPPLY);
#elif defined(CONFIG_POWER_SUPPLY_SMPS_1V8_SUPPLIES_LDO)
	LL_PWR_ConfigSupply(LL_PWR_SMPS_1V8_SUPPLIES_LDO);
#elif defined(CONFIG_POWER_SUPPLY_SMPS_2V5_SUPPLIES_LDO)
	LL_PWR_ConfigSupply(LL_PWR_SMPS_2V5_SUPPLIES_LDO);
#elif defined(CONFIG_POWER_SUPPLY_SMPS_1V8_SUPPLIES_EXT_AND_LDO)
	LL_PWR_ConfigSupply(LL_PWR_SMPS_1V8_SUPPLIES_EXT_AND_LDO);
#elif defined(CONFIG_POWER_SUPPLY_SMPS_2V5_SUPPLIES_EXT_AND_LDO)
	LL_PWR_ConfigSupply(LL_PWR_SMPS_2V5_SUPPLIES_EXT_AND_LDO);
#elif defined(CONFIG_POWER_SUPPLY_SMPS_1V8_SUPPLIES_EXT)
	LL_PWR_ConfigSupply(LL_PWR_SMPS_1V8_SUPPLIES_EXT);
#elif defined(CONFIG_POWER_SUPPLY_SMPS_2V5_SUPPLIES_EXT)
	LL_PWR_ConfigSupply(LL_PWR_SMPS_2V5_SUPPLIES_EXT);
#elif defined(CONFIG_POWER_SUPPLY_EXTERNAL_SOURCE)
	LL_PWR_ConfigSupply(LL_PWR_EXTERNAL_SOURCE_SUPPLY);
#else
	LL_PWR_ConfigSupply(LL_PWR_LDO_SUPPLY);
#endif /* CONFIG_POWER_SUPPLY_* */

	activate_vos0();

	return 0;
}

SYS_INIT(stm32_power_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
