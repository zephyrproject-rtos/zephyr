/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ARM64 Cortex-A power management
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>

_ASM_FILE_PROLOGUE

GTEXT(arch_cpu_idle)
SECTION_FUNC(TEXT, arch_cpu_idle)
#ifdef CONFIG_TRACING
	stp	xzr, x30, [sp, #-16]!
	bl	sys_trace_idle
	ldp	xzr, x30, [sp], #16
#endif
	dsb	sy
	wfi
	msr	daifclr, #(DAIFSET_IRQ)
	ret

GTEXT(arch_cpu_atomic_idle)
SECTION_FUNC(TEXT, arch_cpu_atomic_idle)
#ifdef CONFIG_TRACING
	stp	xzr, x30, [sp, #-16]!
	bl	sys_trace_idle
	ldp	xzr, x30, [sp], #16
#endif
	msr	daifset, #(DAIFSET_IRQ)
	isb
	wfe
	tst	x0, #(DAIF_IRQ)
	beq	_irq_disabled
	msr	daifclr, #(DAIFSET_IRQ)
_irq_disabled:
	ret
