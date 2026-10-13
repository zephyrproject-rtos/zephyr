/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Hexagon power management
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/arch/hexagon/arch.h>
#include <zephyr/arch/cpu.h>
#include <hexagon_vm.h>

#ifdef CONFIG_PM

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		arch_cpu_idle();
		break;

	case PM_STATE_SUSPEND_TO_IDLE: {
		unsigned int key = arch_irq_lock();

		hexagon_vm_wait();
		arch_irq_unlock(key);
		break;
	}

	case PM_STATE_STANDBY: {
		unsigned int key = arch_irq_lock();

		hexagon_vm_yield();
		arch_irq_unlock(key);
		break;
	}

	default:
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	ARG_UNUSED(state);

	/*
	 * Re-enable guest interrupts unconditionally.  PM_STATE_SUSPEND_TO_IDLE
	 * and PM_STATE_STANDBY both call arch_irq_unlock() before returning from
	 * pm_state_set(), so IE is already re-enabled on those paths.
	 * PM_STATE_RUNTIME_IDLE calls arch_cpu_idle() which enables IE via
	 * vmsetie but does not restore a saved key, so IE is also enabled when
	 * pm_state_set() returns.  The irq_unlock(0) here is the authoritative
	 * re-enable that the Zephyr PM layer requires; irq_unlock(0) means
	 * "enable IRQs" on Hexagon (key value 0 = interrupts enabled).
	 */
	irq_unlock(0);
}

#endif /* CONFIG_PM */
