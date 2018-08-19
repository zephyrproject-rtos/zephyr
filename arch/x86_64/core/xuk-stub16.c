/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "serial.h"
#include "x86_64-hw.h"
#include "shared-page.h"

/*
 * 16 bit boot stub.  This code gets copied into a low memory page and
 * used as the bootstrap code for SMP processors, which always start
 * in real mode.  It is compiled with gcc's -m16 switch, which is a
 * wrapper around the assembler's .code16gcc directive which cleverly
 * takes 32 bit assembly and "fixes" it with appropriate address size
 * prefixes to run in real mode on a 386.
 *
 * It is just code!  We have the .text segment and NOTHING ELSE.  No
 * static or global variables can be used, nor const read-only data.
 * Neither is the linker run, so nothing can be relocated and all
 * symbolic references need to be to addresses within this file.  In
 * fact, any relocations that do sneak in will be left at zero at
 * runtime!
 */

__asm__("   cli\n"
	"   xor %ax, %ax\n"
	"   mov %ax, %ss\n"
	"   mov %ax, %ds\n"
	"   mov $80000, %esp\n" /* FIXME: put stack someplace officiallerish */
	"   jmp _start16\n");

void _start16(void)
{
#ifdef XUK_DEBUG
	serial_putc('1'); serial_putc('6'); serial_putc('\n');
#endif

	/* First, serialize on a simple spinlock.  Note there's a
	 * theoretical flaw here in that we are on a shared stack with the
	 * other CPUs here and we don't *technically* know that "oldlock"
	 * does not get written to the (clobberable!) stack memory.  But
	 * in practice the compiler does the right thing here and we spin
	 * in registers until exiting the loop, at which point we are the
	 * only users of the stack, and thus safe.
	 */
	int oldlock;

	do {
		__asm__ volatile("pause; mov $1, %%eax; xchg %%eax, (%1)"
				 : "=a"(oldlock) : "m"(_shared.smpinit_lock));
	} while (oldlock);

	/* Put a red banner at the top of the screen to announce our
	 * presence
	 */
	volatile unsigned short *vga = (unsigned short *)0xb8000;

	for (int i = 0; i < 240; i++)
		vga[i] = 0xcc20;

	/* Spin again waiting on the BSP processor to give us a stack.  We
	 * won't use it until the entry code of stub32, but we want to
	 * make sure it's there before we jump.
	 */
	while (!_shared.smpinit_stack) {
	}

	/* Load the GDT the CPU0 already prepared for us */
	__asm__ volatile ("lgdtw (%0)\n" : : "r"(_shared.gdt16_addr));

	/* Enter protected mode by setting the bottom bit of CR0 */
	int cr0;

	__asm__ volatile ("mov %%cr0, %0\n" : "=r"(cr0));
	cr0 |= 1;
	__asm__ volatile ("mov %0, %%cr0\n" : : "r"(cr0));

	/* Set up data and stack segments */
	short ds = GDT_SELECTOR(2);

	__asm__ volatile ("mov %0, %%ds; mov %0, %%ss" : : "r"(ds));

	/* Far jump to the 32 bit entry point, passing a cookie in EAX to
	 * tell it what we're doing
	 */
	int magic = BOOT_MAGIC_STUB16;

	__asm__ volatile ("ljmpl  $0x8,$0x100000" : : "a"(magic));

	while (1) {
		__asm__("hlt");
	}
}
