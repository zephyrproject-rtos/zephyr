/*
 * Copyright (c) 2022, Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/linker/sections.h>

#include <zephyr/arch/common/pm_s2ram.h>

#define MAGIC (0xDABBAD00)

/**
 * CPU context for S2RAM
 */
__noinit _cpu_context_t _cpu_context;

#ifndef CONFIG_PM_S2RAM_CUSTOM_MARKING
/**
 * S2RAM Marker
 */
static __noinit uint32_t marker;

void __attribute__((naked)) pm_s2ram_mark_set(void)
{
	__asm__ volatile(
		/* Set the marker to MAGIC value */
		"str	%[_magic_val], [%[_marker]]\n"

		"bx	lr\n"
		:
		: [_magic_val] "r"(MAGIC), [_marker] "r"(&marker)
		: "r1", "r4", "memory");
}

bool __attribute__((naked)) pm_s2ram_mark_check_and_clear(void)
{
	__asm__ volatile(
		/* Set return value to 0 */
		"mov	r0, #0\n"

		/* Check the marker */
		"ldr	r3, [%[_marker]]\n"
		"cmp	r3, %[_magic_val]\n"
		"bne	exit\n"

		/*
		 * Reset the marker
		 */
		"str	r0, [%[_marker]]\n"

		/*
		 * Set return value to 1
		 */
		"mov	r0, #1\n"

		"exit:\n"
		"bx lr\n"
		:
		: [_magic_val] "r"(MAGIC), [_marker] "r"(&marker)
		: "r0", "r1", "r3", "r4", "memory");
}

#endif /* CONFIG_PM_S2RAM_CUSTOM_MARKING */
