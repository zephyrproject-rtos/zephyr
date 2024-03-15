/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

_ASM_FILE_PROLOGUE

GTEXT(z_arm_platform_init)

SECTION_FUNC(TEXT, z_arm_platform_init)

	/*
	 * After chip power-on reset, SRAM must be initialized to a known value
	 * using a 64-bit master before 32-bit masters can read or write to RAM.
	 *
	 * This is implemented directly in ASM, to ensure no stack access is performed.
	 */

	ldr r1, = DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
	ldr r2, = DT_REG_SIZE(DT_CHOSEN(zephyr_sram))

	subs	r2, #1

	ble SRAM_LOOP_END

	movs	r0, 0
	movs	r3, 0

SRAM_LOOP:
	stm r1!, {r0,r3}
	subs r2, 8
	bge SRAM_LOOP

SRAM_LOOP_END:

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_itcm), okay)

	ldr r1, = DT_REG_ADDR(DT_CHOSEN(zephyr_itcm))
	ldr r2, = DT_REG_SIZE(DT_CHOSEN(zephyr_itcm))

	subs	r2, #1

ITCM_LOOP:
	stm r1!, {r0,r3}
	subs r2, 8
	bge ITCM_LOOP
#endif

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay)

	ldr r1, = DT_REG_ADDR(DT_CHOSEN(zephyr_dtcm))
	ldr r2, = DT_REG_SIZE(DT_CHOSEN(zephyr_dtcm))

	subs	r2, #1

DTCM_LOOP:
	stm r1!, {r0,r3}
	subs r2, 8
	bge DTCM_LOOP
#endif

	bx lr
