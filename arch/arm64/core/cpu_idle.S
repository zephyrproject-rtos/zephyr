/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ARM64 Cortex-A power management
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>

_ASM_FILE_PROLOGUE
#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
GTEXT(arch_cpu_idle)
SECTION_FUNC(TEXT, arch_cpu_idle)
#ifdef CONFIG_TRACING
	str	lr, [sp, #-16]!
	bl	sys_trace_idle
	ldr	lr, [sp], #16
#endif
	dsb	sy
	wfi
#ifdef CONFIG_TRACING
	str	lr, [sp, #-16]!
	bl	sys_trace_idle_exit
	ldr	lr, [sp], #16
#endif
	msr	daifclr, #(DAIFCLR_IRQ_BIT)
	ret
#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
GTEXT(arch_cpu_atomic_idle)
SECTION_FUNC(TEXT, arch_cpu_atomic_idle)
#ifdef CONFIG_TRACING
	stp	x0, lr, [sp, #-16]!
	bl	sys_trace_idle
	ldp	x0, lr, [sp], #16
#endif
	msr	daifset, #(DAIFSET_IRQ_BIT)
	isb
	wfe
#ifdef CONFIG_TRACING
	str	lr, [sp, #-16]!
	bl	sys_trace_idle_exit
	ldr	lr, [sp], #16
#endif
	tst	x0, #(DAIF_IRQ_BIT)
	beq	_irq_disabled
	msr	daifclr, #(DAIFCLR_IRQ_BIT)
_irq_disabled:
	ret

#endif
