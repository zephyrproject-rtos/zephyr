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
	mrs	x0, daif
	b	arch_cpu_atomic_idle

GTEXT(arch_cpu_atomic_idle)
SECTION_FUNC(TEXT, arch_cpu_atomic_idle)
#ifdef CONFIG_TRACING
	stp	x0, x30, [sp, #-16]!
	bl	sys_trace_idle
	ldp	x0, x30, [sp], #16
#endif
	msr	daifset, #(DAIFSET_IRQ_BIT)
	/*
	 * Ensure outstanding accesses are complete before WFI. Required by ARM spec,
	 * but not actually necessary on most ARM processors.
	 * Has to be done after masking interrupts, as returning from an interrupt is
	 * not considered a dsb.
	 */
	dsb	sy
	/*
	 * Use WFI not WFE. We have interrupts masked here, and wfe does not wake
	 * on masked interrupts (at least not with interrupts taken to EL1.)
	 *
	 * This does mean that events (SEV and friends) won't wake us, which is unfortunate.
	 */
	wfi
	tst	x0, #(DAIF_IRQ_BIT)
	beq	_irq_disabled
	msr	daifclr, #(DAIFCLR_IRQ_BIT)
_irq_disabled:
	ret
