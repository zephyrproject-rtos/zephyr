/*
 * Copyright (c) 2026 Beken Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * First stage for the BK7258.
 *
 * The BootROM starts the image at flash offset 0, and that is not where the
 * application lives: flash entry has to hand execution over to the application.
 * This image lives at offset 0, disables the watchdogs the BootROM leaves
 * running, starts the application at the address it is linked for, and then
 * waits.
 *
 * It is linked at 0x02000000, the XIP window that maps flash offset 0, and is
 * built and merged with the application image by CMakeLists.txt. It is not
 * part of the Zephyr image and does not use Zephyr headers, so it can be
 * assembled before the kernel is linked.
 */

	.syntax unified
	.cpu cortex-m33
	.thumb

	.equ	FLASH_XIP_BASE, 0x02000000
	.equ	APP_XIP_ADDR,   0x02150000	/* board dts: flash0 */
	.equ	STACK_TOP,      0x28010000	/* top of the 64 KB bank below sram0 */

	.equ	VTOR,           0xE000ED08

	.equ	AON_WDT_BASE,   0x44000600
	.equ	WDT_BASE,       0x44800000
	.equ	WDT_GLOBAL_CTRL, 0x08
	.equ	WDT_CTRL,       0x10
	.equ	WDT_KEY_FIRST,  0x5a0000
	.equ	WDT_KEY_SECOND, 0xa50000

	.equ	SYS_CPU1_CTRL,  0x44010014
	.equ	SYS_CPU1_PWR_DW,   (1 << 1)
	.equ	SYS_CPU1_RXEVT_SEL, (1 << 5)
	.equ	SYS_CPU1_SW_RST,    (1 << 0)

/*
 * What the BootROM reads from an image at flash offset 0. It takes the initial
 * stack pointer and the entry point from the first two words, the way a
 * Cortex-M takes them from a vector table, and it starts the image only if the
 * magic below is present at offset 0x100.
 *
 * The core is a Cortex-M, so it would also read the two fault entries from
 * here if a fault occurred before the image is up. They point at the loop
 * below instead of at a vector table.
 *
 * Bit 0 of an entry address selects the instruction set. It is set here by
 * hand: the BootROM does not do what an exception return would do, it loads
 * the word as is and branches to it, and a zero bit would switch the core to
 * ARM state, which it does not have. That ends in an INVSTATE fault before the
 * first instruction of this image runs.
 */
	.section .bk_boot_header, "a"
	.globl	__bk_first_stage
__bk_first_stage:
	.word	STACK_TOP
	.word	_reset + 1
	.word	_fault + 1
	.word	_fault + 1

	.org	0x100
	.ascii	"BK7236"
	.byte	0x10

	.org	0x140
	.globl	_reset
	.thumb_func
_reset:
	/* Point VTOR at this image's header, so the entries above are the ones
	 * used if an exception arrives before the application is up.
	 */
	ldr	r0, =VTOR
	ldr	r1, =FLASH_XIP_BASE
	str	r1, [r0]

	ldr	r0, =STACK_TOP
	msr	msp, r0

	/*
	 * Both watchdogs are running when the BootROM hands over: left alone,
	 * the NMI watchdog raises an NMI about 2.5 s into the boot and the AON
	 * watchdog resets the chip about 45 s in. A zero period, written with
	 * the two-key sequence, disables a watchdog.
	 */
	ldr	r0, =AON_WDT_BASE
	ldr	r1, =WDT_KEY_FIRST
	str	r1, [r0]
	ldr	r1, =WDT_KEY_SECOND
	str	r1, [r0]

	ldr	r0, =WDT_BASE
	ldr	r1, [r0, #WDT_GLOBAL_CTRL]
	orr	r1, r1, #(1 << 1)		/* bypass the clock gate */
	str	r1, [r0, #WDT_GLOBAL_CTRL]
	ldr	r1, =WDT_KEY_FIRST
	str	r1, [r0, #WDT_CTRL]
	ldr	r1, =WDT_KEY_SECOND
	str	r1, [r0, #WDT_CTRL]

	/*
	 * Start the application. The boot address occupies bits 31:8, so the
	 * value written is the address itself, not the address shifted down.
	 * This is the sequence the firmware the device ships with uses, and its
	 * order matters: power up, select the external event, set the address,
	 * then restart.
	 */
	ldr	r0, =SYS_CPU1_CTRL

	/* Power up first, and let the rail settle before the next write. */
	ldr	r1, [r0]
	bic	r1, r1, #SYS_CPU1_PWR_DW
	str	r1, [r0]
	dsb	sy
	ldr	r3, =0x20000
2:	subs	r3, r3, #1
	bne	2b

	/* Then the external event and the boot address. */
	ldr	r1, [r0]
	orr	r1, r1, #SYS_CPU1_RXEVT_SEL
	ldr	r2, =0xFFFFFF00
	bic	r1, r1, r2			/* clear the boot address */
	ldr	r2, =APP_XIP_ADDR
	orr	r1, r1, r2
	str	r1, [r0]
	dsb	sy

	/* And finally release the reset. */
	ldr	r1, [r0]
	orr	r1, r1, #SYS_CPU1_SW_RST
	str	r1, [r0]

	dsb	sy
	isb	sy

1:	wfi
	b	1b

	.thumb_func
_fault:
	b	_fault
