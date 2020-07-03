/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CPU power management
 *
 * CPU power management routines.
 */

#include <kernel_structs.h>
#include <offsets_short.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>

GTEXT(arch_cpu_idle)
GTEXT(arch_cpu_atomic_idle)
GDATA(z_arc_cpu_sleep_mode)

SECTION_VAR(BSS, z_arc_cpu_sleep_mode)
	.align 4
	.word 0

/*
 * @brief Put the CPU in low-power mode
 *
 * This function always exits with interrupts unlocked.
 *
 * void nanCpuIdle(void)
 */

SECTION_FUNC(TEXT, arch_cpu_idle)

#ifdef CONFIG_TRACING
	push_s blink
	jl    sys_trace_idle
	pop_s blink
#endif

	ld r1, [z_arc_cpu_sleep_mode]
	or r1, r1, (1 << 4) /* set IRQ-enabled bit */
/*
 * It's found that (in nsim_hs_smp), when cpu
 * is sleeping, no response to inter-processor interrupt
 * although it's pending and interrupts are enabled.
 * (Here fire SNPS JIRA issue P10019563-41294 to trace)
 * here is a workround
 */
#if defined(CONFIG_SOC_NSIM) && defined(CONFIG_SMP)
	seti r1
_z_arc_idle_loop:
	b _z_arc_idle_loop
#else
	sleep r1
#endif
	j_s [blink]
	nop

/*
 * @brief Put the CPU in low-power mode, entered with IRQs locked
 *
 * This function exits with interrupts restored to <key>.
 *
 * void arch_cpu_atomic_idle(unsigned int key)
 */
SECTION_FUNC(TEXT, arch_cpu_atomic_idle)

#ifdef CONFIG_TRACING
	push_s blink
	jl    sys_trace_idle
	pop_s blink
#endif

	ld r1, [z_arc_cpu_sleep_mode]
	or r1, r1, (1 << 4) /* set IRQ-enabled bit */
	sleep r1
	j_s.d [blink]
	seti r0
