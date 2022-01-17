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

#if defined(CONFIG_PM)
#if defined(CONFIG_VOS0_SWITCH_METHOD_DIRECT)
static void deactivate_vos0(void)
{
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);

	while (LL_PWR_IsActiveFlag_VOS() == 0) {
	}
	while (LL_PWR_IsActiveFlag_ACTVOS() == 0) {
	}
}
#elif defined(CONFIG_VOS0_SWITCH_METHOD_ODEN)
static void deactivate_vos0(void)
{
	RESET_BIT(SYSCFG->PWRCR, SYSCFG_PWRCR_ODEN);

	while (LL_PWR_IsActiveFlag_VOS() == 0) {
	}
}
#else
#error switch out of VOS0 not implemented
#endif /* CONFIG_VOS0_SWITCH_METHOD_* */
#endif /* CONFIG_PM */

#if defined(CONFIG_PM)
static void enter_mode_sleep(uint8_t substate_id)
{
	switch (substate_id) {
	case 1: /* system mode SLEEP with VOS0*/
		break;
	default:
		LOG_DBG("Unsupported power state substate-id %u", substate_id);
		return;
	}

	k_cpu_idle();
}

static void enter_mode_stop(uint8_t substate_id)
{
	switch (substate_id) {
	case 1: /* system mode STOP with SVOS5 and flash in low-power mode*/
		deactivate_vos0();
		MODIFY_REG(PWR->CR1, PWR_CR1_LPDS, PWR_LOWPOWERREGULATOR_ON);
		CLEAR_BIT(PWR->CPUCR, (PWR_CPUCR_PDDS_D1 | PWR_CPUCR_PDDS_D2 | PWR_CPUCR_PDDS_D3));
		LL_PWR_SetStopModeRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SVOS_SCALE5);
		LL_PWR_EnableFlashPowerDown();
		break;
	default:
		LOG_DBG("Unsupported power state substate-id %u", substate_id);
		return;
	}

	LL_LPM_EnableDeepSleep();
	k_cpu_idle();
}

static void enter_mode_standby(uint8_t substate_id)
{
	switch (substate_id) {
	case 1: /* system mode standby*/
		break;
	default:
		LOG_DBG("Unsupported power state substate-id %u", substate_id);
		return;
	}

	LL_PWR_CPU_SetD1PowerMode(LL_PWR_CPU_MODE_D1STANDBY);
	LL_PWR_CPU_SetD2PowerMode(LL_PWR_CPU_MODE_D2STANDBY);
	LL_PWR_CPU_SetD3PowerMode(LL_PWR_CPU_MODE_D3STANDBY);
	LL_LPM_EnableDeepSleep();

	k_cpu_idle();
}

static void exit_mode_sleep(uint8_t substate_id)
{
	switch (substate_id) {
	case 1: /* system mode SLEEP with VOS0*/
		LL_LPM_DisableSleepOnExit();
		LL_LPM_EnableSleep();
	default:
		LOG_DBG("Unsupported power state substate-id %u", substate_id);
		return;
	}
}

static void exit_mode_stop(uint8_t substate_id)
{
	switch (substate_id) {
	case 1: /* system mode STOP with SVOS5 and flash in low-power mode*/
		activate_vos0();
		LL_LPM_DisableSleepOnExit();
		LL_LPM_EnableSleep();
		LL_PWR_DisableFlashPowerDown();
	default:
		LOG_DBG("Unsupported power state substate-id %u", substate_id);
		return;
	}
}

static void exit_mode_standby(uint8_t substate_id)
{
	LOG_ERR("Exit of mode standby occurred, should never happen");
}

/* Invoke Low Power/System Off specific Tasks */
__weak void pm_power_state_set(struct pm_state_info info)
{
	switch (info.state) {
	case PM_STATE_RUNTIME_IDLE:
		enter_mode_sleep(info.substate_id);
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		enter_mode_stop(info.substate_id);
		break;
	case PM_STATE_SOFT_OFF:
		enter_mode_standby(info.substate_id);
		break;
	default:
		LOG_DBG("Unsupported power state %u", info.state);
		return;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
__weak void pm_power_state_exit_post_ops(struct pm_state_info info)
{
	switch (info.state) {
	case PM_STATE_RUNTIME_IDLE:
		exit_mode_sleep(info.substate_id);
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		exit_mode_stop(info.substate_id);
		break;
	case PM_STATE_SOFT_OFF:
		exit_mode_standby(info.substate_id);
		break;
	default:
		LOG_DBG("Unsupported power state %u", info.state);
		break;
	}

	/*
	 * System is now in active mode.
	 * Reenable interrupts which were disabled
	 * when OS started idling code.
	 */
	irq_unlock(0);
}
#endif /* CONFIG_PM */

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
