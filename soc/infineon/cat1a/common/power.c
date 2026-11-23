/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/pm/pm.h>
#include <zephyr/logging/log.h>

#include <soc.h>
#include <cy_syspm.h>
#include <cyhal_syspm.h>

LOG_MODULE_REGISTER(soc_power, CONFIG_SOC_LOG_LEVEL);

/*
 * A peripheral mid-transfer refuses the transition, and hibernate has no later
 * attempt to fall back on. Interrupts are locked here, so only a hardware FIFO
 * can drain during the window; an interrupt-driven transfer never completes.
 * Budgeted for a full 128-byte SCB FIFO at the slowest common baud rate.
 */
#define IFX_HIBERNATE_WAIT_US 250000
#define IFX_HIBERNATE_POLL_US 1000

static void hibernate_enter(void)
{
	uint32_t waited = 0;

	do {
		/* Does not return once a peripheral stops refusing. */
		(void)Cy_SysPm_SystemEnterHibernate();
		k_busy_wait(IFX_HIBERNATE_POLL_US);
		waited += IFX_HIBERNATE_POLL_US;
	} while (waited < IFX_HIBERNATE_WAIT_US);

	LOG_WRN("Hibernate entry refused");
}

/* The legacy HAL SysPm callbacks save, restore and veto on behalf of the
 * peripherals, so nothing here deals with them individually.
 */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	cy_rslt_t rslt;

	ARG_UNUSED(substate_id);

	/* Wake is only possible from PRIMASK, so drop BASEPRI before the WFI. */
	__disable_irq();
	irq_unlock(0);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		LOG_DBG("Entering PM state runtime idle");
		(void)cyhal_syspm_sleep();
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		LOG_DBG("Entering PM state suspend to idle");
		rslt = cyhal_syspm_deepsleep();

		/* The HAL leaves SLEEPDEEP set, which would deepen a later idle WFI. */
		SCB_SCR &= (uint32_t)~SCB_SCR_SLEEPDEEP_Msk;

		if (rslt != CY_RSLT_SUCCESS) {
			/* A peripheral refused, so nothing slept; idle retries. */
			LOG_DBG("DeepSleep refused (0x%08x)", (unsigned int)rslt);
		}
		break;
	case PM_STATE_SOFT_OFF:
		LOG_DBG("Entering PM state soft-off (hibernate)");
		hibernate_enter();
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	__enable_irq();
}

/* Without an armed source, hibernate is left only by reset or power cycle. */
#if DT_HAS_COMPAT_STATUS_OKAY(infineon_hibernate_wakeup)

#define IFX_HIB_WAKEUP_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(infineon_hibernate_wakeup)

static uint32_t hib_wakeup_source_mask(void)
{
	uint32_t src = 0U;

#if DT_NODE_HAS_PROP(IFX_HIB_WAKEUP_NODE, wakeup_pin0_trigger)
	src |= (DT_ENUM_IDX(IFX_HIB_WAKEUP_NODE, wakeup_pin0_trigger) == 0)
		       ? CY_SYSPM_HIBERNATE_PIN0_LOW
		       : CY_SYSPM_HIBERNATE_PIN0_HIGH;
#endif
#if DT_NODE_HAS_PROP(IFX_HIB_WAKEUP_NODE, wakeup_pin1_trigger)
	src |= (DT_ENUM_IDX(IFX_HIB_WAKEUP_NODE, wakeup_pin1_trigger) == 0)
		       ? CY_SYSPM_HIBERNATE_PIN1_LOW
		       : CY_SYSPM_HIBERNATE_PIN1_HIGH;
#endif
#if DT_NODE_HAS_PROP(IFX_HIB_WAKEUP_NODE, wakeup_lpcomp0_trigger)
	src |= (DT_ENUM_IDX(IFX_HIB_WAKEUP_NODE, wakeup_lpcomp0_trigger) == 0)
		       ? CY_SYSPM_HIBERNATE_LPCOMP0_LOW
		       : CY_SYSPM_HIBERNATE_LPCOMP0_HIGH;
#endif
#if DT_NODE_HAS_PROP(IFX_HIB_WAKEUP_NODE, wakeup_lpcomp1_trigger)
	src |= (DT_ENUM_IDX(IFX_HIB_WAKEUP_NODE, wakeup_lpcomp1_trigger) == 0)
		       ? CY_SYSPM_HIBERNATE_LPCOMP1_LOW
		       : CY_SYSPM_HIBERNATE_LPCOMP1_HIGH;
#endif
#if DT_PROP(IFX_HIB_WAKEUP_NODE, wakeup_wdt)
	src |= CY_SYSPM_HIBERNATE_WDT;
#endif
#if DT_PROP(IFX_HIB_WAKEUP_NODE, wakeup_rtc_alarm)
	src |= CY_SYSPM_HIBERNATE_RTC_ALARM;
#endif

	return src;
}

static void hib_wakeup_arm(void)
{
	uint32_t src = hib_wakeup_source_mask();

	/* The setter only ORs bits in, so start from a known-clear mask. */
	Cy_SysPm_ClearHibernateWakeupSource(CY_SYSPM_HIB_WAKEUP_SOURCE_MASK);

	if (src == 0U) {
		return;
	}

	Cy_SysPm_SetHibernateWakeupSource(src);
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(infineon_hibernate_wakeup) */

static int ifx_pm_init(void)
{
	cy_rslt_t rslt = cyhal_syspm_init();

	if (rslt != CY_RSLT_SUCCESS) {
		LOG_ERR("cyhal_syspm_init failed: 0x%08x", (unsigned int)rslt);
		return -EIO;
	}

	/* Note Cy_SysClk_DeepSleepCallback is not registered here.
	 * It only acts when the FLL or a PLL is sourced from the ECO.
	 * Register it here if ECO sourcing is added.
	 */

#if DT_HAS_COMPAT_STATUS_OKAY(infineon_hibernate_wakeup)
	hib_wakeup_arm();
#endif

	return 0;
}

SYS_INIT(ifx_pm_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
