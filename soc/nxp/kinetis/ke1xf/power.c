/*
 * Copyright (c) 2021 Vestas Wind Systems A/S
 * Copyright (c) 2021, 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <zephyr/arch/arch_interface.h>
#include <soc.h>

LOG_MODULE_DECLARE(power, CONFIG_PM_LOG_LEVEL);

#ifdef CONFIG_XIP
__ramfunc static void wait_for_flash_prefetch_and_wfi(void)
{
	uint32_t i;

	for (i = 0; i < 8; i++) {
		arch_nop();
	}

	__DSB();
	__ISB();
	__WFI();
}
#endif /* CONFIG_XIP */

static void enter_low_power(void)
{
	unsigned int key;

	key = arch_pm_state_set_prepare();
	__DSB();
	__ISB();
	__WFI();
	arch_pm_state_set_finish(key);
}

#ifdef CONFIG_XIP
static void enter_low_power_from_ram(void)
{
	unsigned int key;

	key = arch_pm_state_set_prepare();
	wait_for_flash_prefetch_and_wfi();
	arch_pm_state_set_finish(key);
}
#endif /* CONFIG_XIP */

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		enter_low_power();
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		/* Set partial stop mode and enable deep sleep */
		SMC->STOPCTRL = SMC_STOPCTRL_PSTOPO(substate_id);
		SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
#ifdef CONFIG_XIP
		enter_low_power_from_ram();
#else /* CONFIG_XIP */
		enter_low_power();
#endif /* !CONFIG_XIP */

		if (SMC->PMCTRL & SMC_PMCTRL_STOPA_MASK) {
			LOG_DBG("partial stop aborted");
		}
		break;
	default:
		LOG_WRN("Unsupported power state %u", state);
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	if (state == PM_STATE_SUSPEND_TO_IDLE) {
		/* Disable deep sleep upon exit */
		SCB->SCR &= ~(SCB_SCR_SLEEPDEEP_Msk);
	}
}
