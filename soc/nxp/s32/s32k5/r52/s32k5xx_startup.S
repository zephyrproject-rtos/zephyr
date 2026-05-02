/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

#define LPE_MC_RGM_BASE 0x42074000
#define LPE_MC_RGM_DES  0x0
#define LPE_MC_RGM_FES  0x8

_ASM_FILE_PROLOGUE

GTEXT(soc_reset_hook)

SECTION_FUNC(TEXT, soc_reset_hook)

	/* Enable peripheral port access at EL1 and EL0 */
	mrc p15, 0, r0, c15, c0, 0
	orr r0, #1
	mcr p15, 0, r0, c15, c0, 0

	/*
	 * Take exceptions in Arm mode because Zephyr ASM code for Cortex-R Aarch32
	 * is written for Arm
	 */
	mrc p15, 0, r0, c1, c0, 0
	and r0, r0, #~BIT(30)
	mcr p15, 0, r0, c1, c0, 0

#if defined(CONFIG_XIP)
	/*
	 * SRAM must be initialized to a known value using a 64-bit initiator before
	 * 32-bit initiators can read or write to them. Note that SRAM retains content
	 * during functional reset through a hardware mechanism, therefore accesses do not
	 * cause any content corruption errors.
	 *
	 * This is implemented directly in ASM, to ensure no stack access is performed.
	 */
	ldr r1, =DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
	ldr r2, =DT_REG_SIZE(DT_CHOSEN(zephyr_sram))

	subs r2, r2, #1
	ble ECC_END

	mov r0, #0
	mov r3, #0

SRAM_LOOP:
	stmia r1!, {r0, r3}
	subs r2, r2, #8
	bge SRAM_LOOP

ECC_END:

#endif /* CONFIG_XIP */

	bx lr
