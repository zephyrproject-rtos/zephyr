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

GTEXT(k_cpu_idle)
GTEXT(k_cpu_atomic_idle)
GDATA(k_cpu_sleep_mode)

SECTION_VAR(BSS, k_cpu_sleep_mode)
	.balign 4
	.word 0

/*
 * @brief Put the CPU in low-power mode
 *
 * This function always exits with interrupts unlocked.
 *
 * void nanCpuIdle(void)
 */

SECTION_FUNC(TEXT, k_cpu_idle)

#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
	push_s blink
	jl    _sys_k_event_logger_enter_sleep
	pop_s blink
#endif

	ld r1, [k_cpu_sleep_mode]
	or r1, r1, (1 << 4) /* set IRQ-enabled bit */
	sleep r1
	j_s [blink]
	nop

/*
 * @brief Put the CPU in low-power mode, entered with IRQs locked
 *
 * This function exits with interrupts restored to <key>.
 *
 * void k_cpu_atomic_idle(unsigned int key)
 */
SECTION_FUNC(TEXT, k_cpu_atomic_idle)

#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
	push_s blink
	jl    _sys_k_event_logger_enter_sleep
	pop_s blink
#endif

	ld r1, [k_cpu_sleep_mode]
	or r1, r1, (1 << 4) /* set IRQ-enabled bit */
	sleep r1
	j_s.d [blink]
	seti r0
