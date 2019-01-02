/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <soc_power.h>
#include <nrf_power.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* Invoke Low Power/System Off specific Tasks */
void sys_set_power_state(enum power_states state)
{
	switch (state) {
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
 #ifdef CONFIG_SYS_POWER_STATE_DEEP_SLEEP_SUPPORTED
	case SYS_POWER_STATE_DEEP_SLEEP:
		nrf_power_system_off();
		break;
 #endif
#endif
	default:
		LOG_ERR("Unsupported power state %u", state);
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void sys_power_state_post_ops(enum power_states state)
{
	switch (state) {
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
 #ifdef CONFIG_SYS_POWER_STATE_DEEP_SLEEP_SUPPORTED
	case SYS_POWER_STATE_DEEP_SLEEP:
		/* Nothing to do. */
		break;
 #endif
#endif
	default:
		LOG_ERR("Unsupported power state %u", state);
		break;
	}

	/*
	 * System is now in active mode. Reenable interrupts which were disabled
	 * when OS started idling code.
	 */
	irq_unlock(0);
}

bool sys_is_valid_power_state(enum power_states state)
{
	switch (state) {
#ifdef CONFIG_SYS_POWER_LOW_POWER_STATE
 #ifdef CONFIG_SYS_POWER_STATE_CPU_LPS_SUPPORTED
	case SYS_POWER_STATE_CPU_LPS:
		return true;
 #endif
 #ifdef CONFIG_SYS_POWER_STATE_CPU_LPS_1_SUPPORTED
	case SYS_POWER_STATE_CPU_LPS_1:
		return true;
 #endif
 #ifdef CONFIG_SYS_POWER_STATE_CPU_LPS_2_SUPPORTED
	case SYS_POWER_STATE_CPU_LPS_2:
		return true;
 #endif
#endif /* CONFIG_SYS_POWER_LOW_POWER_STATE */

#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
 #ifdef CONFIG_SYS_POWER_STATE_DEEP_SLEEP_SUPPORTED
	case SYS_POWER_STATE_DEEP_SLEEP:
		return true;
 #endif
 #ifdef CONFIG_SYS_POWER_STATE_DEEP_SLEEP_1_SUPPORTED
	case SYS_POWER_STATE_DEEP_SLEEP_1:
		return true;
 #endif
 #ifdef CONFIG_SYS_POWER_STATE_DEEP_SLEEP_2_SUPPORTED
	case SYS_POWER_STATE_DEEP_SLEEP_2:
		return true;
 #endif
#endif /* CONFIG_SYS_POWER_DEEP_SLEEP */

	default:
		return false;
	}

	/* Not reached */
}

/* Overrides the weak ARM implementation:
   Set general purpose retention register and reboot */
void sys_arch_reboot(int type)
{
	nrf_power_gpregret_set((uint8_t)type);
	NVIC_SystemReset();
}
