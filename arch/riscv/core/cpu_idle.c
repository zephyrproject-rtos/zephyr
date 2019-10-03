/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <irq.h>

/*
 * In RISC-V there is no conventional way to handle CPU power save.
 * Each RISC-V SOC handles it in its own way.
 * Hence, by default, z_arch_cpu_idle and z_arch_cpu_atomic_idle functions just
 * unlock interrupts and return to the caller, without issuing any CPU power
 * saving instruction.
 *
 * Nonetheless, define the default z_arch_cpu_idle and z_arch_cpu_atomic_idle
 * functions as weak functions, so that they can be replaced at the SOC-level.
 */

void __weak z_arch_cpu_idle(void)
{
	irq_unlock(SOC_MSTATUS_IEN);
}

void __weak z_arch_cpu_atomic_idle(unsigned int key)
{
	irq_unlock(key);
}
