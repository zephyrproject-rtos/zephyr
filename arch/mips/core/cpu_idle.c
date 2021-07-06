/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/arch_interface.h>

void arch_cpu_idle(void)
{
	irq_unlock(1);
}

void arch_cpu_atomic_idle(unsigned int key)
{
	irq_unlock(key);
}
