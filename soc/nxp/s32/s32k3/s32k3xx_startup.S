/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

#define MC_RGM_BASE 0x4028C000
#define MC_RGM_DES  0x0
#define MC_RGM_FES  0x8

_ASM_FILE_PROLOGUE

GTEXT(soc_reset_hook)

SECTION_FUNC(TEXT, soc_reset_hook)

	/*
	 * On destructive reset, SRAM and TCM memories must be initialized to a known value using a
	 * 64-bit master before 32-bit masters can read or write to them. Note that SRAM retains
	 * content during functional reset through a hardware mechanism, therefore accesses do not
	 * cause any content corruption errors.
	 *
	 * This is implemented directly in ASM, to ensure no stack access is performed.
	 */

	/* If we come from a destructive reset, then ignore functional reset flags */
	ldr r1, =MC_RGM_BASE
	ldr r2, [r1, MC_RGM_DES]
	cmp r2, 0x0
	bne ECC_INIT
	ldr r2, [r1, MC_RGM_FES]
	cmp r2, 0x0
	bne ECC_END

ECC_INIT:
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

ECC_END:
	bx lr
