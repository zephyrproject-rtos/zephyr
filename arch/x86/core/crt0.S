/* crt0.S - crt0 module for the IA-32 boards */

/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
DESCRIPTION
This module contains the initial code executed by the Zephyr Kernel ELF image
after having been loaded into RAM.
*/	

#define _ASMLANGUAGE

#include <arch/x86/asm.h>

	/* exports (private APIs) */

	GTEXT(__start)

	/* externs */
	GTEXT(_Cstart)

	GDATA(_idt_base_address)
	GDATA(_interrupt_stack)


#if defined(CONFIG_SSE)
	GDATA(_sse_mxcsr_default_value)
#endif /* CONFIG_SSE */

#if defined(CONFIG_BOOT_TIME_MEASUREMENT)
	GDATA(__start_tsc)
#endif

#ifdef CONFIG_ADVANCED_IDLE
	GDATA(_AdvIdleCheckSleep)
	GDATA(_AdvIdleStart)
#endif /* CONFIG_ADVANCED_IDLE */


	/* processor is executing in 32-bit protected mode */

	.balign 16,0x90

SECTION_FUNC(TEXT_START, __start)

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	/*
	 * Record BootTime from start of Kernel.
	 * Store value temporarily in Register edi & esi and
         * write to memory once memory access is allowed.
         * That is, once the data segment register has been setup to access
         * the .data/.rodata/.bss section of the linked image.
	 */
        rdtsc
        mov  %eax, %esi			/* low  value */
        mov  %edx, %edi			/* high value */
#endif

	/* Enable write-back caching by clearing the NW and CD bits */
	movl	%cr0, %eax
	andl	$0x9fffffff, %eax
	movl	%eax, %cr0

	/*
	 * Ensure interrupts are disabled.  Interrupts are enabled when
	 * the first context switch occurs.
	 */

	cli

	/*
	 * Although the bootloader sets up an Interrupt Descriptor Table (IDT)
	 * and a Global Descriptor Table (GDT), the specification encourages
	 * booted operating systems to setup their own IDT and GDT.
	 */

	lgdt	_GdtRom		/* load 32-bit operand size GDT */
	lidt	_Idt		/* load 32-bit operand size IDT */


#ifdef CONFIG_BOOTLOADER_UNKNOWN
	/*
	 * Where we do not do the protected mode switch and the
	 * bootloader is unknown, do not make the assumption that the segment
	 * registers are set correctly.
	 *
	 * This is a special case for the ia32 platform, which must work for
	 * multiple platforms (QEMU, generic PC board, etc.). With other
	 * platforms the bootloader is well known so assumptions can be made.
	 */
	movw	$0x10, %ax	/* data segment selector (entry = 3) */
	movw	%ax, %ds	/* set DS */
	movw	%ax, %es	/* set ES */
	movw	%ax, %fs	/* set FS */
	movw	%ax, %gs	/* set GS */
	movw	%ax, %ss	/* set SS */

	/* this is 'ljmp $0x08:$_csSet': diab 5.9.1.0 does not encode
	 * it correctly */
	.byte 0xea
	.long __csSet
	.byte 0x08
	.byte 0x00

__csSet:
#endif /* CONFIG_BOOTLOADER_UNKNOWN */


#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	/*
	 * Store rdtsc result from temporary regiter ESI & EDI into memory.
	 */
        mov  %esi, __start_tsc    	/* low  value */
        mov  %edi, __start_tsc+4    	/* high value */
#endif

#ifdef CONFIG_ADVANCED_IDLE

	/*
	 * Set up the temporary stack to call the _AdvIdleCheckSleep routine
	 * We use the separate stack here in order to avoid the memory
	 * corruption if the system recovers from deep sleep
	 */
	movl	$_AdvIdleStack, %esp
	addl	$CONFIG_ADV_IDLE_STACK_SIZE, %esp

	/* align to stack boundary: ROUND_DOWN (%esp, 4) */

	andl	$0xfffffffc, %esp

	/*
	 * Invoke _AdvIdleCheckSleep() routine that checks if we are restoring
	 * from deep sleep or not. The routine returns non-zero if the kernel
	 * is recovering from deep sleep and to 0 if a cold boot is needed. The
	 * kernel can skip floating point initialization, BSS initialization,
	 * and data initialization if recovering from deep sleep.
	 */

	call	_AdvIdleCheckSleep
	cmpl	$0, %eax
	jne	memInitDone

#endif /* CONFIG_ADVANCED_IDLE */

#if !defined(CONFIG_FLOAT)
	/*
	 * Force an #NM exception for floating point instructions
	 * since FP support hasn't been configured
	 */

	movl	%cr0, %eax		/* move CR0 to EAX */
	orl	$0x2e, %eax		/* CR0[NE+TS+EM+MP]=1 */
	movl	%eax, %cr0		/* move EAX to CR0 */
#else
	/*
	 * Permit use of x87 FPU instructions
	 *
	 * Note that all floating point exceptions are masked by default,
	 * and that _no_ handler for x87 FPU exceptions (#MF) is provided.
	 */

	movl	%cr0, %eax		/* move CR0 to EAX */
	orl	$0x22, %eax		/* CR0[NE+MP]=1 */
	andl	$~0xc, %eax		/* CR0[TS+EM]=0 */
	movl	%eax, %cr0		/* move EAX to CR0 */

	fninit				/* set x87 FPU to its default state */

  #if defined(CONFIG_SSE)
	/*
	 * Permit use of SSE instructions
	 *
	 * Note that all SSE exceptions are masked by default,
	 * and that _no_ handler for SSE exceptions (#XM) is provided.
	 */

	movl	%cr4, %eax		/* move CR4 to EAX */
	orl	$0x200, %eax		/* CR4[OSFXSR] = 1 */
	andl	$~0x400, %eax		/* CR4[OSXMMEXCPT] = 0 */
	movl	%eax, %cr4		/* move EAX to CR4 */

	ldmxcsr _sse_mxcsr_default_value   /* initialize SSE control/status reg */

  #endif /* CONFIG_SSE */

#endif /* !CONFIG_FLOAT */

#ifdef CONFIG_XIP
	/*
	 * copy DATA section from ROM to RAM region
	 *	 DATA is followed by BSS section.
	 *       Given that BSS section is initialized after this copy, we can
	 *	 safely over-write into the next section.
	 * Note: __data_num_words is a multiple of 4 bytes
	 *       rounded up to next 4 bytes.
	 *	 Note: the sections might not be 4 byte aligned.
	 */

	movl	$__data_ram_start, %edi /* DATA in RAM (dest) */
	movl	$__data_rom_start, %esi /* DATA in ROM (src) */
	movl	$__data_num_words, %ecx /* Size of DATA in quad bytes */
	je	copyDataDone

  #ifdef CONFIG_SSE
	/* copy 16 bytes at a time using XMM until < 16 bytes remain */

	movl	%ecx ,%edx		/* save number of quad bytes */
	shrl	$2, %ecx		/* How many 16 bytes? */
	je	dataWords

dataDQ:
	movdqu	(%esi), %xmm0
	movdqu	%xmm0, (%edi)
	addl	$16, %esi
	addl	$16, %edi
	loop	dataDQ

dataWords:
	movl	%edx, %ecx	/* restore # quad bytes */
	andl	$0x3, %ecx	/* only need to copy at most 3 quad bytes */
  #endif /* CONFIG_SSE */

	rep
	movsl				/* copy data 4 bytes at a time */
copyDataDone:

#endif /* CONFIG_XIP */

	/*
	 * Clear BSS: bzero (__bss_start, __bss_num_words*4)
	 *
	 * It's assumed that BSS size will be a multiple of a long (4 bytes),
	 * and aligned on a double word (32-bit) boundary
	 */

#ifdef CONFIG_SSE

	/* use XMM register to clear 16 bytes at a time */

	pxor	%xmm0, %xmm0		/* zero out xmm0 register */
	movl	$__bss_start, %edi	/* load BSS start address */
	movl	$__bss_num_words, %ecx	/* number of quad bytes in .bss */
	movl	%ecx, %edx		/* make a copy of # quad bytes */
	shrl	$2, %ecx		/* How many multiples of 16 byte ? */
	je	bssWords
bssDQ:
	movdqu	%xmm0, (%edi)		/* zero 16 bytes... */
	addl	$16, %edi
	loop	bssDQ

	/* fall through to handle the remaining double words (32-bit chunks) */

bssWords:
	xorl	%eax, %eax		/* fill memory with 0 */
	movl	%edx, %ecx		/* move # quad bytes into ECX (for rep) */
	andl	$0x3, %ecx		/* only need to zero at most 3 quad bytes */
	cld
	rep
	stosl				/* zero memory per 4 bytes */

#else /* !CONFIG_SSE */

	/* clear out BSS double words (32-bits at a time) */

	xorl	%eax, %eax		/* fill memory with 0 */
	movl	$__bss_start, %edi	/* load BSS start address */
	movl	$__bss_num_words, %ecx	/* number of quad bytes */
	cld
	rep
	stosl				/* zero memory per 4 bytes */

#endif /* CONFIG_SSE */

memInitDone:

	/*
	 * Set the stack pointer to the area used for the interrupt stack.
	 * Note this stack is only used during the execution of __start() and
	 * _Cstart(), i.e. only until the multi-tasking kernel is
	 * initialized.  The dual-purposing of this area of memory is safe since
	 * interrupts are disabled until the first context switch.
	 */

	movl	$_interrupt_stack, %esp
	addl	$CONFIG_ISR_STACK_SIZE, %esp

	/* align to stack boundary: ROUND_DOWN (%esp, 4) */

	andl	$0xfffffffc, %esp

	/* activate RAM-based Global Descriptor Table (GDT) */

	lgdt	%ds:_gdt



#if defined (CONFIG_ADVANCED_IDLE)
	/*
	 * Invoke _AdvIdleStart(_Cstart, _gdt, _GlobalTss) by jumping to it.
	 * If it's a cold boot, this routine jumps to _Cstart and the normal
	 * kernel boot sequence continues; otherwise, it uses the TSS info
	 * saved in the GDT to resumes kernel processing at the point it was
	 * when the system went into deep sleep; that is, _AdvIdleFunc()
	 * completes and returns a non-zero value.
	 */

	pushl	$_GlobalTss
	pushl	$_gdt
	pushl	$_Cstart
	call	_AdvIdleStart
#else
	/* Jump to C portion of kernel initialization and never return */

	jmp	_Cstart

#endif /* CONFIG_ADVANCED_IDLE */

#if defined(CONFIG_SSE)

	/* SSE control & status register initial value */

_sse_mxcsr_default_value:
	.long	0x1f80			/* all SSE exceptions clear & masked */

#endif /* CONFIG_SSE */

	 /* Interrupt Descriptor Table (IDT) definition */

_Idt:
	.word	(CONFIG_IDT_NUM_VECTORS * 8) - 1 /* limit: size of IDT-1 */

	/*
	 * Physical start address = 0.  When executing natively, this
	 * will be placed at the same location as the interrupt vector table
	 * setup by the BIOS (or GRUB?).
	 */

	.long	_idt_base_address		/* physical start address */


#ifdef CONFIG_BOOTLOADER_UNKNOWN
	/* Multiboot header definition is needed for some bootloaders */

	/*
	 * The multiboot header must be in the first 8 Kb of the kernel image
	 * (not including the ELF section header(s)) and be aligned on a
	 * 4 byte boundary.
	 */

	.balign 4,0x90

	.long	0x1BADB002	/* multiboot magic number */

	/*
	 * Flags = no bits are being set, specifically bit 16 is not being
	 * set since the supplied kernel image is an ELF file, and the
	 * multiboot loader shall use the information from the program and
	 * section header to load and boot the kernel image.
	 */

	.long	0x00000000

	/*
	 * checksum = 32-bit unsigned value which, when added to the other
	 * magic fields (i.e. "magic" and "flags"), must have a 32-bit
	 * unsigned sum of zero.
	 */

	.long	-(0x1BADB002 + 0)
#endif /* CONFIG_BOOTLOADER_UNKNOWN */


_GdtRom:
	.word _GdtRomEnd - _GdtRomEntries - 1   /* Limit on GDT */
	.long _GdtRomEntries			/* table address: _GdtRomEntries */


	.balign 16,0x90

	/*
	 * The following 3 GDT entries implement the so-called "basic
	 * flat model", i.e. a single code segment descriptor and a single
	 * data segment descriptor, giving the kernel access to a continuous,
	 * unsegmented address space.  Both segment descriptors map the entire
	 * linear address space (i.e. 0 to 4 GB-1), thus the segmentation
	 * mechanism will never generate "out of limit memory reference"
	 * exceptions even if physical memory does not reside at the referenced
	 * address.
	 *
	 * The 'A' (accessed) bit in the type field is _not_ set for all the
	 * data/code segment descriptors to accommodate placing these entries
	 * in ROM, since such use is not planned for this platform.
	 */

_GdtRomEntries:

	/* Entry 0 (selector=0x0000): The "NULL descriptor" */

	.word   0x0000
	.word   0x0000
	.byte   0x00
	.byte   0x00
	.byte   0x00
	.byte   0x00

	/* Entry 1 (selector=0x0008): Code descriptor: DPL0 */

	.word   0xffff		/* limit: xffff */
	.word   0x0000		/* base : xxxx0000 */
	.byte   0x00		/* base : xx00xxxx */
	.byte   0x9a		/* Code e/r, Present, DPL0 */
	.byte   0xcf		/* limit: fxxxx, Page Gra, 32bit */
	.byte   0x00		/* base : 00xxxxxx */

	/* Entry 2 (selector=0x0010): Data descriptor: DPL0 */

	.word   0xffff		/* limit: xffff */
	.word   0x0000		/* base : xxxx0000 */
	.byte   0x00		/* base : xx00xxxx */
	.byte   0x92		/* Data r/w, Present, DPL0 */
	.byte   0xcf		/* limit: fxxxx, Page Gra, 32bit */
	.byte   0x00		/* base : 00xxxxxx */

_GdtRomEnd:

#ifdef CONFIG_ADVANCED_IDLE
	.section .NOINIT
	.balign 4,0x90
_AdvIdleStack:
	.fill CONFIG_ADV_IDLE_STACK_SIZE
#endif
