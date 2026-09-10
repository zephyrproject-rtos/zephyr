/*
 * Copyright 2024, 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/arch/arch_interface.h>

static void enter_low_power(void)
{
	unsigned int key;

	key = arch_pm_state_set_prepare();
	__DSB();
	__ISB();
	__WFI();
	arch_pm_state_set_finish(key);
}

void pm_state_set(enum pm_state state, uint8_t id)
{
	ARG_UNUSED(id);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		enter_low_power();
		break;
	default:

		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(id);
}
