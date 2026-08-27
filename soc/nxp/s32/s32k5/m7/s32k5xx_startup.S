/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

_ASM_FILE_PROLOGUE

GTEXT(soc_early_reset_hook)

SECTION_FUNC(TEXT, soc_early_reset_hook)

	/*
	 * SRAM must be initialized to a known value using a 64-bit initiator before
	 * 32-bit initiators can read or write to them. Note that SRAM retains content
	 * during functional reset through a hardware mechanism, therefore accesses do not
	 * cause any content corruption errors.
	 *
	 * This is implemented directly in ASM, to ensure no stack access is performed.
	 */
	ldr r1, = DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
	ldr r2, = DT_REG_SIZE(DT_CHOSEN(zephyr_sram))

	subs	r2, #1

	ble ECC_END

	movs	r0, 0
	movs	r3, 0

SRAM_LOOP:
	stm r1!, {r0,r3}
	subs r2, 8
	bge SRAM_LOOP

ECC_END:
	bx lr
