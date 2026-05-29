/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

_ASM_FILE_PROLOGUE

GTEXT(soc_reset_hook)

SECTION_FUNC(TEXT, soc_reset_hook)

	mov r0, #0
	ldr r1, = DT_REG_ADDR(DT_NODELABEL(dtcm))
	ldr r2, = DT_REG_ADDR(DT_NODELABEL(dtcm)) + DT_REG_SIZE(DT_NODELABEL(dtcm))
DTCM_LOOP:
	cmp r1, r2
	itt lt
	strlt r0, [r1], #4
	blt DTCM_LOOP

	bx lr
