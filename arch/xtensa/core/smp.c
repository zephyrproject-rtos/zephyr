/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/sys/util_macro.h>

#ifdef CONFIG_XTENSA_MORE_SPIN_RELAX_NOPS
/* Some compilers might "optimize out" (i.e. remove) continuous NOPs.
 * So force no optimization to avoid that.
 */
__no_optimization
void arch_spin_relax(void)
{
#define NOP1(_, __) __asm__ volatile("nop.n;");
	LISTIFY(CONFIG_XTENSA_NUM_SPIN_RELAX_NOPS, NOP1, (;))
#undef NOP1
}
#endif /* CONFIG_XTENSA_MORE_SPIN_RELAX_NOPS */


/**
 * init for multi-core/smp is done on the SoC level. Add this here for
 * compatibility with other SMP systems.
 */
int arch_smp_init(void)
{
	return 0;
}
