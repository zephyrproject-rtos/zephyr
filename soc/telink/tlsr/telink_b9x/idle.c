/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/tracing/tracing.h>

#if CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE || CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE

/* Due to silicon bug in B92 platform, the WFI emulation is implemented */
static volatile bool __irq_pending;

static ALWAYS_INLINE void riscv_idle(unsigned int key)
{
	sys_trace_idle();
	__irq_pending = true;
	irq_unlock(key);

	while (__irq_pending) {
	}
}

void __soc_handle_irq(unsigned long mcause)
{
	__irq_pending = false;
	__asm__("li t1, 1");
	__asm__("sll t0, t1, a0");
	__asm__("csrrc t1, mip, t0");
}
#endif /* CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE || CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE */

#if CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
void arch_cpu_idle(void)
{
	riscv_idle(MSTATUS_IEN);
}
#endif /* CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE */


#if CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
void arch_cpu_atomic_idle(unsigned int key)
{
	riscv_idle(key);
}
#endif /* CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE */
