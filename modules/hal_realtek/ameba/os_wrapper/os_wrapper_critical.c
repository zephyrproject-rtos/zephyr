/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "os_wrapper.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(os_if_critical);

static int key;
static volatile uint32_t critical_nesting;

int rtos_critical_is_in_interrupt(void)
{
#ifdef CONFIG_ARM_CORE_CM4
	return (__get_xPSR() & 0x1FF) != 0;
#else
	return __get_IPSR() != 0;
#endif
}

/*-------------------------------critical------------------------------*/
/*
 * Zephyr does not have concept of Critical section
 * disable and enable irq to avoid interrupt and context switch
 */
void rtos_critical_enter(uint32_t component_id)
{
	UNUSED(component_id);
	if (critical_nesting == 0) {
		key = irq_lock();
	}
	critical_nesting++;
}

void rtos_critical_exit(uint32_t component_id)
{
	UNUSED(component_id);
	critical_nesting--;
	if (critical_nesting == 0) {
		irq_unlock(key);
	}
}

uint32_t rtos_get_critical_state(void)
{
	return critical_nesting;
}
