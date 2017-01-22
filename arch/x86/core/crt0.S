/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Crt0 module for the IA-32 boards
 *
 * This module contains the initial code executed by the Zephyr Kernel ELF image
 * after having been loaded into RAM.
 */

#include <arch/x86/asm.h>

	/* exports (private APIs) */

	GTEXT(__start)

	/* externs */
	GTEXT(_Cstart)

	GDATA(_idt_base_address)
	GDATA(_interrupt_stack)
	GDATA(_Idt)
#ifndef CONFIG_GDT_DYNAMIC
	GDATA(_gdt)
#endif


#if defined(CONFIG_SSE)
	GDATA(_sse_mxcsr_default_value)
#endif

#if defined(CONFIG_BOOT_TIME_MEASUREMENT)
	GDATA(__start_tsc)
#endif

#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
	GTEXT(_sys_soc_resume_from_deep_sleep)
#endif


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
#if CONFIG_SET_GDT
	lgdt	_gdt_rom		/* load 32-bit operand size GDT */
#endif
	lidt	_Idt		/* load 32-bit operand size IDT */


#ifdef CONFIG_SET_GDT
	/* If we set our own GDT, update the segment registers as well.
	 */
	movw	$0x10, %ax	/* data segment selector (entry = 3) */
	movw	%ax, %ds	/* set DS */
	movw	%ax, %es	/* set ES */
	movw	%ax, %fs	/* set FS */
	movw	%ax, %gs	/* set GS */
	movw	%ax, %ss	/* set SS */

	ljmp	$0x08, $__csSet	/* set CS = 0x08 */

__csSet:
#endif /* CONFIG_SET_GDT */


#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	/*
	 * Store rdtsc result from temporary regiter ESI & EDI into memory.
	 */
        mov  %esi, __start_tsc    	/* low  value */
        mov  %edi, __start_tsc+4    	/* high value */
#endif

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

	/*
	 * Set the stack pointer to the area used for the interrupt stack.
	 * Note this stack is used during the execution of __start() and
	 * _Cstart() until the multi-tasking kernel is initialized.  The
	 * dual-purposing of this area of memory is safe since
	 * interrupts are disabled until the first context switch.
	 *
	 * nano_init.c enforces that the _interrupt_stack pointer and
	 * the ISR stack size are some multiple of STACK_ALIGN, which
	 * is at least 4.
	 *
	 * This is also used to call the _sys_soc_resume_from_deep_sleep()
	 * routine to avoid memory corruption if the system is resuming from
	 * deep sleep. It is important that _sys_soc_resume_from_deep_sleep()
	 * restores the stack pointer to what it was at deep sleep before
         * enabling interrupts.  This is necessary to avoid
	 * interfering with interrupt handler use of this stack.
	 * If it is a cold boot then _sys_soc_resume_from_deep_sleep() should
	 * not do anything and must return immediately.
	 */
#ifdef CONFIG_INIT_STACKS
	movl $0xAAAAAAAA, %eax
	leal _interrupt_stack, %edi
	stack_size_dwords = (CONFIG_ISR_STACK_SIZE / 4)
	movl $stack_size_dwords, %ecx
	rep  stosl
#endif

	movl	$_interrupt_stack, %esp
	addl	$CONFIG_ISR_STACK_SIZE, %esp

#if defined(CONFIG_SYS_POWER_DEEP_SLEEP) && \
             !defined(CONFIG_BOOTLOADER_CONTEXT_RESTORE)
	/*
	 * Invoke _sys_soc_resume_from_deep_sleep() hook to handle resume from
	 * deep sleep. It should first check whether system is recovering from
	 * deep sleep state. If it is, then this function should restore
	 * states and resume at the point system went to deep sleep.
	 * In this case this function will never return.
	 *
	 * If system is not recovering from deep sleep then it is a
	 * cold boot.  In this case, this function would immediately
	 * return and execution falls through to cold boot path.
	 */

	call _sys_soc_resume_from_deep_sleep

#endif

#ifdef CONFIG_XIP
	/*
	 * copy DATA section from ROM to RAM region
	 *	 DATA is followed by BSS section.
	 */

	movl	$__data_ram_start, %edi /* DATA in RAM (dest) */
	movl	$__data_rom_start, %esi /* DATA in ROM (src) */
	movl	$__data_num_words, %ecx /* Size of DATA in quad bytes */

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

#ifdef CONFIG_GDT_DYNAMIC
	/* activate RAM-based Global Descriptor Table (GDT) */
	lgdt	%ds:_gdt
#endif

	/* Jump to C portion of kernel initialization and never return */

	jmp	_Cstart

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

#ifdef CONFIG_SET_GDT

	/* GDT should be aligned on 8-byte boundary for best processor
	 * performance, see Section 3.5.1 of IA architecture SW developer
	 * manual, Vol 3.
	 */

	.balign 8

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
	 * The 'A' (accessed) bit in the type field is set for all the
	 * data/code segment descriptors to accommodate placing these entries
	 * in ROM, to prevent the processor from freaking out when it tries
	 * and fails to set it.
	 */

#ifndef CONFIG_GDT_DYNAMIC
_gdt:
#endif
_gdt_rom:

	/* Entry 0 (selector=0x0000): The "NULL descriptor". The CPU never
	 * actually looks at this entry, so we stuff 6-byte the pseudo
	 * descriptor here */
	.word _gdt_rom_end - _gdt_rom - 1	/* Limit on GDT */
	.long _gdt_rom				/* table address: _gdt_rom */
	.word   0x0000

	/* Entry 1 (selector=0x0008): Code descriptor: DPL0 */

	.word   0xffff		/* limit: xffff */
	.word   0x0000		/* base : xxxx0000 */
	.byte   0x00		/* base : xx00xxxx */
	.byte   0x9b		/* Accessed, Code e/r, Present, DPL0 */
	.byte   0xcf		/* limit: fxxxx, Page Gra, 32bit */
	.byte   0x00		/* base : 00xxxxxx */

	/* Entry 2 (selector=0x0010): Data descriptor: DPL0 */

	.word   0xffff		/* limit: xffff */
	.word   0x0000		/* base : xxxx0000 */
	.byte   0x00		/* base : xx00xxxx */
	.byte   0x93		/* Accessed, Data r/w, Present, DPL0 */
	.byte   0xcf		/* limit: fxxxx, Page Gra, 32bit */
	.byte   0x00		/* base : 00xxxxxx */

_gdt_rom_end:
#endif
