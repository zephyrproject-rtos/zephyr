/*
 * Copyright (c) 2021 Vestas Wind Systems A/S
 * Copyright 2021, 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <soc.h>

LOG_MODULE_DECLARE(power, CONFIG_PM_LOG_LEVEL);

#ifdef CONFIG_XIP
__ramfunc static void wait_for_flash_prefetch_and_wfi(void)
{
	uint32_t i;

	for (i = 0; i < 8; i++) {
		arch_nop();
	}

	/*
	 * Must execute WFI directly from RAM when XIP is enabled.
	 * Calling k_cpu_idle() may jump back into flash.
	 */
	__DSB();
	__ISB();
	__WFI();
}
#endif /* CONFIG_XIP */

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		/* Normal WAIT: WFI with SLEEPDEEP cleared. */
		SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
		__DSB();
		__ISB();
		__WFI();
		irq_unlock(0);
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		if (substate_id > 2U) {
			LOG_WRN("Unsupported substate-id %u, using 0", substate_id);
			substate_id = 0U;
		}
		/* Ensure STOPM is set to normal STOP (not VLPS/VLLS families), if present. */
		SMC->PMCTRL &= ~SMC_PMCTRL_STOPM_MASK;

#if (defined(FSL_FEATURE_SMC_HAS_PSTOPO) && FSL_FEATURE_SMC_HAS_PSTOPO)
		SMC->STOPCTRL = (SMC->STOPCTRL & ~SMC_STOPCTRL_PSTOPO_MASK) |
				SMC_STOPCTRL_PSTOPO(substate_id);
#endif

		SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
		/* readback to complete bus writes */
		(void)SMC->PMCTRL;
		__DSB();
		__ISB();
#ifdef CONFIG_XIP
		wait_for_flash_prefetch_and_wfi();
#else
		__WFI();
#endif
		if (SMC->PMCTRL & SMC_PMCTRL_STOPA_MASK) {
			LOG_DBG("partial stop aborted");
		}
		irq_unlock(0);
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
		SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
	}

	irq_unlock(0);
}
