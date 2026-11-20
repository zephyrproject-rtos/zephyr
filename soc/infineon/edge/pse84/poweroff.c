/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>
#include <zephyr/logging/log.h>

#include <cy_pdl.h>
#include <cy_syspm.h>

LOG_MODULE_REGISTER(soc_poweroff, CONFIG_SOC_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(infineon_hibernate_wakeup)

#define IFX_HIB_WAKEUP_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(infineon_hibernate_wakeup)

/*
 * Build the Hibernate wakeup-source mask from the infineon,hibernate-wakeup
 * devicetree node.
 */
static uint32_t ifx_hib_wakeup_source_mask(void)
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

/*
 * Arm the Hibernate wakeup source(s) described by the infineon,hibernate-wakeup
 * devicetree node.  Without one armed the chip powers down and never comes
 * back, so this runs on the way into every hibernate.
 */
static void ifx_hib_wakeup_arm(void)
{
	uint32_t src = ifx_hib_wakeup_source_mask();

	if (src != 0U) {
		Cy_SysPm_SetHibernateWakeupSource(src);
	}
}

#endif /* DT_HAS_COMPAT_STATUS_OKAY(infineon_hibernate_wakeup) */

void z_sys_poweroff(void)
{
	cy_en_syspm_status_t status;

#if DT_HAS_COMPAT_STATUS_OKAY(infineon_hibernate_wakeup)
	ifx_hib_wakeup_arm();
#endif

	status = Cy_SysPm_SystemEnterHibernate();
	LOG_ERR("Hibernate entry failed (status %d); halting", (int)status);

	(void)irq_lock();
	for (;;) {
		__WFI();
	}
}
