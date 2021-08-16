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
#include <arch/arc/asm-compat/assembler.h>

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
	PUSHR blink
	jl    sys_trace_idle
	POPR blink
#endif

/*
 * Workaround for iotdk board. When iotdk board enter sleep mode,
 * some peripherals (eg: uart) will power off and and board can't
 * wake up through peripherals interrupt.
 */
#ifdef CONFIG_BOARD_IOTDK
	j_s.d [blink]
	seti 0
#else
	/* z_arc_cpu_sleep_mode is 32 bit despite of platform bittnes */
	ld r1, [z_arc_cpu_sleep_mode]
	or r1, r1, (1 << 4) /* set IRQ-enabled bit */
	sleep r1
	j_s [blink]
	nop
#endif

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

/*
 * Workaround for iotdk board. When iotdk board enter sleep mode,
 * some peripherals (eg: uart) will power off and and board can't
 * wake up through peripherals interrupt.
 */
#ifdef CONFIG_BOARD_IOTDK
	j_s.d [blink]
	seti r0
#else
	/* z_arc_cpu_sleep_mode is 32 bit despite of platform bittnes */
	ld r1, [z_arc_cpu_sleep_mode]
	or r1, r1, (1 << 4) /* set IRQ-enabled bit */
	sleep r1
	j_s.d [blink]
	seti r0
#endif
