/*
 * Copyright (c) 2025 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/pm.h>
#include <power.h>

void kb106x_soc_deep_sleep(void)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif
	/*
	 * PRIMASK is always cleared on ARMv7-M and ARMv8-M (not used
	 * for interrupt locking), and configuring BASEPRI to the lowest
	 * priority to ensure wake-up will cause interrupts to be serviced
	 * before entering low power state.
	 *
	 * Set PRIMASK before configuring BASEPRI to prevent interruption
	 * before wake-up.
	 */
	__disable_irq();

	SCB->SCR |= BIT(2);
	/*
	 * Set wake-up interrupt priority to the lowest and synchronize to
	 * ensure that this is visible to the WFI instruction.
	 */
	__set_BASEPRI(0);
	__ISB();

	__DSB();
	__WFI();
	SCB->SCR &= ~BIT(2);

#if defined(CONFIG_TRACING)
	sys_trace_idle_exit();
#endif
	__enable_irq();
	__ISB();
}

void kb106x_soc_idle(void)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif
	/*
	 * PRIMASK is always cleared on ARMv7-M and ARMv8-M (not used
	 * for interrupt locking), and configuring BASEPRI to the lowest
	 * priority to ensure wake-up will cause interrupts to be serviced
	 * before entering low power state.
	 *
	 * Set PRIMASK before configuring BASEPRI to prevent interruption
	 * before wake-up.
	 */
	__disable_irq();

	SCB->SCR &= ~BIT(2);
	/*
	 * Set wake-up interrupt priority to the lowest and synchronize to
	 * ensure that this is visible to the WFI instruction.
	 */
	__set_BASEPRI(0);
	__ISB();

	__DSB();
	__WFI();
#if defined(CONFIG_TRACING)
	sys_trace_idle_exit();
#endif

	__enable_irq();
	__ISB();
}

__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_ACTIVE:
		break;
	case PM_STATE_RUNTIME_IDLE:
		__fallthrough;
	case PM_STATE_SUSPEND_TO_IDLE:
		kb106x_soc_idle();
		break;
	case PM_STATE_STANDBY:
		__fallthrough;
	case PM_STATE_SUSPEND_TO_RAM:
		__fallthrough;
	case PM_STATE_SUSPEND_TO_DISK:
		__fallthrough;
	case PM_STATE_SOFT_OFF:
		kb106x_soc_deep_sleep();
		break;
	default:
		k_cpu_idle();
		break;
	}
}

__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);
}
