/*
 * Copyright (c) 2018 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

void __weak arch_cpu_idle(void)
{
	/* idle calls lock_irq() function before falling on this function.
	 * but the unlock key is lost. so unlock irq unconditionally
	 * irq_unlock function will enable unlock interrupt if key has
	 * Enable Trap bit on.
	 */
	irq_unlock(0);
}
