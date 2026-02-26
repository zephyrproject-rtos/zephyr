/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/sections.h>

/* Require CONFIG_XIP=n */
#define ROM_START	(CONFIG_FLASH_BASE_ADDRESS + DT_REG_ADDR(DT_CHOSEN(zephyr_code_partition)))
#define RAM_START	CONFIG_SRAM_BASE_ADDRESS
#define APP_SIZE	_flash_used

_ASM_FILE_PROLOGUE

GTEXT(loader_program)

/*
 * Loader program
 */
SECTION_FUNC(loader_text, loader_program)
	ldr r0, =ROM_START         // Src in ROM
	ldr r1, =RAM_START         // Des in RAM
	ldr r2, =APP_SIZE

	cmp r2, #0
	beq exit

loop:
	ldrb r3, [r0], #1    // Load byte from src and increment src pointer
	strb r3, [r1], #1    // Store byte to des and increment des pointer
	subs r2, r2, #1      // Decrement size and updates the condition flags
	bne loop             // If size is not 0, repeat loop

done:
	dsb sy
	ldr pc, =__start

exit:
	wfi
