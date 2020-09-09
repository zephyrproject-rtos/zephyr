/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
  * @file
  *
  * @brief Workaround aarch64 QEMU not responding to host OS signals
  * during 'wfi'.
  * See https://github.com/zephyrproject-rtos/sdk-ng/issues/255
  */

#include <arch/cpu.h>

void arch_cpu_idle(void)
{
	/* Do nothing but unconditionally unlock interrupts and return to the
	 * caller.
	 */

	__asm__ volatile("msr daifclr, %0\n\t"
			 : : "i" (DAIFSET_IRQ)
			 : "memory", "cc");
}

void arch_cpu_atomic_idle(unsigned int key)
{
	/* Do nothing but restore IRQ state */
	arch_irq_unlock(key);
}
