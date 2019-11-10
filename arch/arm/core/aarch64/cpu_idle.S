/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM64 Cortex-A power management
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>

GTEXT(arch_cpu_idle)
SECTION_FUNC(TEXT, arch_cpu_idle)
	dsb	sy
	wfi
	msr	daifclr, #(DAIFSET_IRQ)
	ret

GTEXT(arch_cpu_atomic_idle)
SECTION_FUNC(TEXT, arch_cpu_atomic_idle)
	msr	daifset, #(DAIFSET_IRQ)
	isb
	wfe
	tst	x0, #(DAIF_IRQ)
	beq	_irq_disabled
	msr	daifclr, #(DAIFSET_IRQ)
_irq_disabled:
	ret
