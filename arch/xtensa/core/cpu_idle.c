/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tracing/tracing.h>

void arch_cpu_idle(void)
{
	sys_trace_idle();

	/* Just spin forever with interrupts unmasked, for platforms
	 * where WAITI can't be used or where its behavior is
	 * complicated (Intel DSPs will power gate on idle entry under
	 * some circumstances)
	 */
	if (IS_ENABLED(CONFIG_XTENSA_CPU_IDLE_SPIN)) {
		__asm__ volatile("rsil a0, 0");
		__asm__ volatile("loop_forever: j loop_forever");
		return;
	}

	/* Cribbed from SOF: workaround for a bug in some versions of
	 * the LX6 IP.  Preprocessor ugliness avoids the need to
	 * figure out how to get the compiler to unroll a loop.
	 */
	if (IS_ENABLED(CONFIG_XTENSA_WAITI_BUG)) {
#define NOP4 __asm__ volatile("nop; nop; nop; nop");
#define NOP32 NOP4 NOP4 NOP4 NOP4 NOP4 NOP4 NOP4 NOP4
#define NOP128() NOP32 NOP32 NOP32 NOP32
		NOP128();
#undef NOP128
#undef NOP16
#undef NOP4
		__asm__ volatile("isync; extw");
	}

	__asm__ volatile ("waiti 0");
}

void arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	__asm__ volatile ("waiti 0\n\t"
			  "wsr.ps %0\n\t"
			  "rsync" :: "a"(key));
}
