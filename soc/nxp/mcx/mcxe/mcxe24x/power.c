/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>

LOG_MODULE_DECLARE(power, CONFIG_PM_LOG_LEVEL);

#ifdef CONFIG_XIP
__ramfunc static void wait_for_flash_prefetch_and_wfi(void)
{
	/*
	 * When executing from external flash (XIP), ensure we don't
	 * fetch instructions from flash across low power entry.
	 */
	for (uint32_t i = 0; i < 8; i++) {
		arch_nop();
	}

	__DSB();
	__ISB();
	__WFI();
}
#endif /* CONFIG_XIP */

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	__set_BASEPRI(0);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		/* Cortex-M sleep: WFI with SLEEPDEEP cleared. */
		SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
		__DSB();
		__ISB();
		__WFI();
		break;

	case PM_STATE_SUSPEND_TO_IDLE:
		/* Substates align to KE-like STOP options: 0=STOP, 1=PSTOP1, 2=PSTOP2. */
		if (substate_id > 2U) {
			LOG_WRN("Unsupported substate-id %u, using 0", substate_id);
			substate_id = 0U;
		}

		/* Ensure STOPM is set to normal STOP (not VLPS/VLLS families), if present. */
		SMC->PMCTRL &= ~SMC_PMCTRL_STOPM_MASK;

#if (defined(FSL_FEATURE_SMC_HAS_PSTOPO) && FSL_FEATURE_SMC_HAS_PSTOPO)
		SMC->STOPCTRL = (SMC->STOPCTRL & (uint8_t)~SMC_STOPCTRL_PSTOPO_MASK) |
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
			LOG_DBG("stop aborted");
		}

		break;

	default:
		LOG_WRN("Unsupported power state %u", state);
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	if ((SCB->SCR & SCB_SCR_SLEEPDEEP_Msk) == SCB_SCR_SLEEPDEEP_Msk) {
		SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
	}

	__enable_irq();
	__ISB();
}
