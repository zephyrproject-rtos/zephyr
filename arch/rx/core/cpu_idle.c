/*
 * Copyright (c) 2021 KT-Elektronik, Klaucke und Partner GmbH
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>

void arch_cpu_idle(void)
{
	sys_trace_idle();

	/* The assembler instruction "wait" switches the processor to sleep mode,
	 * which stops program execution until an interrupt is triggered.
	 * All clocks that are not in a stop state continue operating, including
	 * the system timer.
	 *
	 * Also, "wait" sets the PSW I bit, activating
	 * interrupts (otherwise, the processor would never return from sleep
	 * mode). This is consistent with the Zephyr API description, according
	 * to which "In some architectures, before returning, the function
	 * unmasks interrupts unconditionally." - this is such an architecture.
	 */
	__asm__ volatile("wait");
}

void arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();

	/* The assembler instruction "wait" switches the processor to sleep mode,
	 * which stops program execution until an interrupt is triggered.
	 * All clocks that are not in a stop state continue operating, including
	 * the system timer.
	 */
	__asm__ volatile("wait");

	/* "wait" unconditionally unlocks interrupts. To restore the interrupt
	 * lockout state before calling arch_cpu_atomic_idle, interrupts have
	 * to be locked after returning from "wait" if irq_lock would NOT have
	 * unlocked interrupts (i.e. if the key indicates nested interrupt
	 * locks)
	 */
	if (key == 0) {
		irq_lock();
	}
}
