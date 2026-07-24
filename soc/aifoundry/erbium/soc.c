/*
 * Copyright (c) 2026 AIFoundry
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>

/*
 * Erbium does not implement WFI. Keep idle as an interrupt-enabled spin so the
 * kernel idle path does not emit an unsupported instruction.
 */
void arch_cpu_idle(void)
{
	irq_unlock(MSTATUS_IEN);
}

void arch_cpu_atomic_idle(unsigned int key)
{
	irq_unlock(key);
}
