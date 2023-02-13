/*
 * Copyright (c) 2021 Vestas Wind Systems A/S
 * Copyright (c) 2021, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <soc.h>

LOG_MODULE_DECLARE(power, CONFIG_PM_LOG_LEVEL);

#ifdef CONFIG_XIP
__ramfunc static void wait_for_flash_prefetch_and_idle(void)
{
	uint32_t i;

	for (i = 0; i < 8; i++) {
		arch_nop();
	}

	k_cpu_idle();
}
#endif /* CONFIG_XIP */

__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		k_cpu_idle();
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		/* Set partial stop mode and enable deep sleep */
		SMC->STOPCTRL = SMC_STOPCTRL_PSTOPO(substate_id);
		SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
#ifdef CONFIG_XIP
		wait_for_flash_prefetch_and_idle();
#else /* CONFIG_XIP */
		k_cpu_idle();
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

__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	if (state == PM_STATE_SUSPEND_TO_IDLE) {
		/* Disable deep sleep upon exit */
		SCB->SCR &= ~(SCB_SCR_SLEEPDEEP_Msk);
	}

	irq_unlock(0);
}
