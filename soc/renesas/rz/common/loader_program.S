/*
 * Copyright (c) 2025-2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/sections.h>

_ASM_FILE_PROLOGUE

GTEXT(loader_program)

#if defined(CONFIG_SOC_FAMILY_RENESAS_RZ_TCM_LOADER)

/*
 * Loader program for soc/renesas/rz/scripts/linker.ld
 *
 * Runs from the BTCM, where the boot ROM copied it, before any stack, cache
 * or MPU set-up. Copies every block of the copy table built by the linker
 * script from flash to its run address, then enters the kernel.
 */
SECTION_FUNC(loader_text, loader_program)
#if CONFIG_SOC_FAMILY_RENESAS_RZ_LOADER_DELAY > 0
	/* Leave time to attach a debugger */
	ldr	r0, =CONFIG_SOC_FAMILY_RENESAS_RZ_LOADER_DELAY
delay:
	subs	r0, r0, #1
	bne	delay
#endif

	ldr	r4, =__loader_copy_table_start
	ldr	r5, =__loader_copy_table_end

next_block:
	cmp	r4, r5
	bhs	copy_done
	ldm	r4!, {r0, r1, r2}	/* load address, run address, size */
	cmp	r2, #0
	beq	next_block

	/* Copy by words when the addresses and the size allow it */
	orr	r3, r0, r1
	orr	r3, r3, r2
	tst	r3, #3
	bne	copy_bytes

copy_words:
	ldr	r3, [r0], #4
	str	r3, [r1], #4
	subs	r2, r2, #4
	bne	copy_words
	b	next_block

copy_bytes:
	ldrb	r3, [r0], #1
	strb	r3, [r1], #1
	subs	r2, r2, #1
	bne	copy_bytes
	b	next_block

copy_done:
	/* Complete the copies, then drop instructions fetched before them */
	dsb	sy
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0	/* ICIALLU */
	dsb	sy
	isb
	ldr	pc, =__start

#else /* CONFIG_SOC_FAMILY_RENESAS_RZ_TCM_LOADER */

/* Require CONFIG_XIP=n */
#define ROM_START	DT_REG_ADDR(DT_CHOSEN(zephyr_code_partition))
#define APP_SIZE	_flash_used

#define RAM_START DT_CHOSEN_SRAM_ADDR

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

#endif /* CONFIG_SOC_FAMILY_RENESAS_RZ_TCM_LOADER */
