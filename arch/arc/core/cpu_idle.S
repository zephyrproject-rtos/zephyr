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

#include <zephyr/kernel_structs.h>
#include <offsets_short.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arc/asm-compat/assembler.h>

GTEXT(arch_cpu_idle)
GTEXT(arch_cpu_atomic_idle)
GDATA(z_arc_cpu_sleep_mode)

SECTION_VAR(BSS, z_arc_cpu_sleep_mode)
	.align 4
	.word 0

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
/*
 * @brief Put the CPU in low-power mode
 *
 * This function always exits with interrupts unlocked.
 *
 * void nanCpuIdle(void)
 */

SECTION_FUNC(TEXT, arch_cpu_idle)

#ifdef CONFIG_TRACING
	PUSHR blink
	jl    sys_trace_idle
	POPR blink
#endif

	/* z_arc_cpu_sleep_mode is 32 bit despite of platform bittnes */
	ld r1, [z_arc_cpu_sleep_mode]
	or r1, r1, (1 << 4) /* set IRQ-enabled bit */
	sleep r1
	j_s [blink]
	nop
#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
/*
 * @brief Put the CPU in low-power mode, entered with IRQs locked
 *
 * This function exits with interrupts restored to <key>.
 *
 * void arch_cpu_atomic_idle(unsigned int key)
 */

SECTION_FUNC(TEXT, arch_cpu_atomic_idle)

#ifdef CONFIG_TRACING
	PUSHR blink
	jl    sys_trace_idle
	POPR blink
#endif

	/* z_arc_cpu_sleep_mode is 32 bit despite of platform bittnes */
	ld r1, [z_arc_cpu_sleep_mode]
	or r1, r1, (1 << 4) /* set IRQ-enabled bit */
	sleep r1
	j_s.d [blink]
	seti r0
#endif
