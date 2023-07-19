/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/tracing/tracing.h>

/* xt-clang removes any NOPs more than 8. So we need to set
 * no optimization to avoid those NOPs from being removed.
 *
 * This function is simply enough and full of hand written
 * assembly that optimization is not really meaningful
 * anyway. So we can skip optimization unconditionally.
 * Re-evalulate its use and add #ifdef if this assumption
 * is no longer valid.
 */
__no_optimization
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
#undef NOP32
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
