/*
 * Copyright (c) 2018, Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <power/power.h>
#include <em_emu.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/*
 * Power state map:
 * SYS_POWER_STATE_SLEEP_1: EM1 Sleep
 * SYS_POWER_STATE_SLEEP_2: EM2 Deep Sleep
 * SYS_POWER_STATE_SLEEP_3: EM3 Stop
 */

/* Invoke Low Power/System Off specific Tasks */
void sys_set_power_state(enum power_states state)
{
	LOG_DBG("SoC entering power state %d", state);

	/* FIXME: When this function is entered the Kernel has disabled
	 * interrupts using BASEPRI register. This is incorrect as it prevents
	 * waking up from any interrupt which priority is not 0. Work around the
	 * issue and disable interrupts using PRIMASK register as recommended
	 * by ARM.
	 */

	/* Set PRIMASK */
	__disable_irq();
	/* Set BASEPRI to 0 */
	irq_unlock(0);

	switch (state) {
#ifdef CONFIG_SYS_POWER_SLEEP_STATES
#ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_1
	case SYS_POWER_STATE_SLEEP_1:
		EMU_EnterEM1();
		break;
#endif /* CONFIG_HAS_SYS_POWER_STATE_SLEEP_1 */
#ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_2
	case SYS_POWER_STATE_SLEEP_2:
		EMU_EnterEM2(true);
		break;
#endif /* CONFIG_HAS_SYS_POWER_STATE_SLEEP_2 */
#ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_3
	case SYS_POWER_STATE_SLEEP_3:
		EMU_EnterEM3(true);
		break;
#endif /* CONFIG_HAS_SYS_POWER_STATE_SLEEP_3 */
#endif /* CONFIG_SYS_POWER_SLEEP_STATES */
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}

	LOG_DBG("SoC leaving power state %d", state);

	/* Clear PRIMASK */
	__enable_irq();
}

/* Handle SOC specific activity after Low Power Mode Exit */
void _sys_pm_power_state_exit_post_ops(enum power_states state)
{
	ARG_UNUSED(state);
}
