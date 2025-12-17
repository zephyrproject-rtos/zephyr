/*
 * Copyright (c) 2024-2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/arch/common/pm_s2ram.h>

#include <mxc_device.h>
#include <mcr_regs.h>
#include <wrap_max32_lp.h>
#include <wrap_max32_sys.h>

#include <zephyr/logging/log.h>
#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

extern uint8_t pm_get_s2ram_retention_mask(void);

#if defined(CONFIG_PM_S2RAM) && defined(CONFIG_CORTEX_M_SYSTICK_LPM_TIMER_COUNTER)
static const struct device *idle_timer =
	DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_cortex_m_idle_timer));
#endif /* defined(CONFIG_PM_S2RAM) && defined(CONFIG_CORTEX_M_SYSTICK_LPM_TIMER_COUNTER) */

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	/* Set PRIMASK */
	__disable_irq();
	/* Set BASEPRI to 0 */
	irq_unlock(0);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		LOG_DBG("entering PM state runtime idle");
		MXC_LP_EnterSleepMode();
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		LOG_DBG("entering PM state suspend to idle");
		Wrap_MXC_LP_EnterMicroPowerMode();
		break;
	case PM_STATE_STANDBY:
		LOG_DBG("entering PM state standby");
		Wrap_MXC_LP_EnterStandbyMode();
		break;
	case PM_STATE_SUSPEND_TO_RAM:
#ifdef CONFIG_PM_S2RAM
		LOG_DBG("entering PM state suspend to ram");

#ifdef CONFIG_CORTEX_M_SYSTICK_LPM_TIMER_COUNTER
		if (idle_timer) {
			/* This does not actually suspend the idle-mode timer; it just decrements
			 * the pm usage counter so that device can be resumed once the system
			 * wakes up.
			 */
			pm_device_runtime_put(idle_timer);
		}
#endif /* CONFIG_CORTEX_M_SYSTICK_LPM_TIMER_COUNTER */

		/* SRAM retention configurable via Kconfig */
		Wrap_MXC_LP_EnableRetentionReg();
		Wrap_MXC_LP_EnableSramRetention(pm_get_s2ram_retention_mask());

		/* Suspend to RAM */
		arch_pm_s2ram_suspend(Wrap_MXC_LP_EnterBackupMode);
#else
		LOG_WRN("PM_STATE_SUSPEND_TO_RAM must be enabled by Kconfig option!");
#endif /* CONFIG_PM_S2RAM */
		break;
	case PM_STATE_SOFT_OFF:
		LOG_DBG("entering PM state powerdown");
		Wrap_MXC_LP_EnterPowerDownMode();
		/*Code not reach here */
	default:
		LOG_DBG("Unsupported power state %u", state);
		return;
	}
	LOG_DBG("wakeup from sleep mode");
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	/* Set run mode config after wakeup */
	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		LOG_DBG("exited PM state runtime idle");
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		LOG_DBG("exited PM state suspend to idle");
		break;
	case PM_STATE_SUSPEND_TO_RAM:
#ifdef CONFIG_PM_S2RAM
		max32xx_system_init();
		Wrap_MXC_LP_DisableSramRetention();
		Wrap_MXC_LP_ExitBackupMode();

#ifdef CONFIG_CORTEX_M_SYSTICK_LPM_TIMER_COUNTER
		if (idle_timer) {
			/* Resume the idle-mode timer */
			pm_device_runtime_get(idle_timer);
		}
#endif /* CONFIG_CORTEX_M_SYSTICK_LPM_TIMER_COUNTER */

		LOG_DBG("exited PM state suspend to ram");
#else
		LOG_WRN("PM_STATE_SUSPEND_TO_RAM must be enabled by Kconfig option!");
#endif /* CONFIG_PM_S2RAM */
		break;
	case PM_STATE_STANDBY:
		LOG_DBG("exited PM state standby");
		break;
	default:
		break;
	}

	/* Clear PRIMASK after wakeup */
	__enable_irq();
}
