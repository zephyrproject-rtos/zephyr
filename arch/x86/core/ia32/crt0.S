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
 *
 * Note that most addresses (functions and variables) must be in physical
 * address space. Depending on page table setup, they may or may not be
 * available in virtual address space after loading of page table.
 */

#include <arch/x86/ia32/asm.h>
#include <arch/x86/msr.h>
#include <kernel_arch_data.h>
#include <arch/cpu.h>
#include <arch/x86/multiboot.h>
#include <x86_mmu.h>

	/* exports (private APIs) */

	GTEXT(__start)

	/* externs */
	GTEXT(z_x86_prep_c)
	GTEXT(z_bss_zero)
	GTEXT(z_data_copy)

	GDATA(_idt_base_address)
	GDATA(z_interrupt_stacks)
	GDATA(z_x86_idt)
#ifndef CONFIG_GDT_DYNAMIC
	GDATA(_gdt)
#endif


#if defined(CONFIG_X86_SSE)
	GDATA(_sse_mxcsr_default_value)
#endif

SECTION_FUNC(TEXT_START, __start)

#include "../common.S"

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
	/* load 32-bit operand size GDT */
	lgdt	Z_X86_PHYS_ADDR(_gdt_rom)

	/* If we set our own GDT, update the segment registers as well.
	 */
	movw	$DATA_SEG, %ax	/* data segment selector (entry = 3) */
	movw	%ax, %ds	/* set DS */
	movw	%ax, %es	/* set ES */
	movw	%ax, %ss	/* set SS */
	xorw	%ax, %ax	/* AX = 0 */
	movw	%ax, %fs	/* Zero FS */
	movw	%ax, %gs	/* Zero GS */

	ljmp	$CODE_SEG, $Z_X86_PHYS_ADDR(__csSet)	/* set CS = 0x08 */

__csSet:
#endif /* CONFIG_SET_GDT */

#if !defined(CONFIG_FPU)
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

  #if defined(CONFIG_X86_SSE)
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

  #endif /* CONFIG_X86_SSE */

#endif /* !CONFIG_FPU */

	/*
	 * Set the stack pointer to the area used for the interrupt stack.
	 * Note this stack is used during the execution of __start() and
	 * z_cstart() until the multi-tasking kernel is initialized.  The
	 * dual-purposing of this area of memory is safe since
	 * interrupts are disabled until the first context switch.
	 *
	 * kernel/init.c enforces that the z_interrupt_stacks pointer and
	 * the ISR stack size are some multiple of ARCH_STACK_PTR_ALIGN, which
	 * is at least 4.
	 */
#ifdef CONFIG_INIT_STACKS
	movl $0xAAAAAAAA, %eax
	leal Z_X86_PHYS_ADDR(z_interrupt_stacks), %edi
#ifdef CONFIG_X86_STACK_PROTECTION
	addl $4096, %edi
#endif
	stack_size_dwords = (CONFIG_ISR_STACK_SIZE / 4)
	movl $stack_size_dwords, %ecx
	rep  stosl
#endif

	movl	$Z_X86_PHYS_ADDR(z_interrupt_stacks), %esp
#ifdef CONFIG_X86_STACK_PROTECTION
	/* In this configuration, all stacks, including IRQ stack, are declared
	 * with a 4K non-present guard page preceding the stack buffer
	 */
	addl	$(CONFIG_ISR_STACK_SIZE + 4096), %esp
#else
	addl	$CONFIG_ISR_STACK_SIZE, %esp
#endif

	/* Clear BSS */
	call	z_bss_zero

#ifdef CONFIG_XIP
	/* Copy data from flash to RAM.
	 *
	 * This is a must is CONFIG_GDT_DYNAMIC is enabled,
	 * as _gdt needs to be in RAM.
	 */
	call	z_data_copy
#endif

#ifdef CONFIG_GDT_DYNAMIC
	/* activate RAM-based Global Descriptor Table (GDT) */
	lgdt	%ds:Z_X86_PHYS_ADDR(_gdt)
#endif

#if defined(CONFIG_X86_ENABLE_TSS)
	mov $MAIN_TSS, %ax
	ltr %ax
#endif
	/* load 32-bit operand size IDT */
	lidt	Z_X86_PHYS_ADDR(z_x86_idt)

#ifdef CONFIG_X86_MMU
	/* Install page tables */
	movl $Z_X86_PHYS_ADDR(z_x86_kernel_ptables), %eax
	movl %eax, %cr3

#ifdef CONFIG_X86_PAE
	/* Enable PAE */
	movl %cr4, %eax
	orl $CR4_PAE, %eax
	movl %eax, %cr4

	/* IA32_EFER NXE bit set */
	movl $0xC0000080, %ecx
	rdmsr
	orl $0x800, %eax
	wrmsr
#endif /* CONFIG_X86_PAE */

	/* Enable paging (CR0.PG, bit 31) / write protect (CR0.WP, bit 16) */
	movl %cr0, %eax
	orl $(CR0_PG | CR0_WP), %eax
	movl %eax, %cr0
#endif /* CONFIG_X86_MMU */

#if (Z_X86_PHYS_ADDR(0x1000) != 0x1000)
	/*
	 * Page table loaded so we can start executing in
	 * virtual address space.
	 *
	 * Note that __prep_c and z_x86_prep_c() must be
	 * mapped in virtual address space in gen_mmu.py.
	 *
	 * This jump must be done due to relative
	 * addressing code emitted by the toolchain
	 * (think EIP + offset). If we are not already in
	 * virtual address space, everything afterwards
	 * would still be referenced via physical addresses
	 * and will crash if we have a kernel bigger than
	 * physical memory with demand paging (for example).
	 */
	ljmp	$CODE_SEG, $__prep_c

__prep_c:
#endif

	pushl	%ebx		/* pointer to multiboot info, or NULL */
	call	z_x86_prep_c	/* enter kernel; never returns */

#if defined(CONFIG_X86_SSE)

	/* SSE control & status register initial value */

_sse_mxcsr_default_value:
	.long	0x1f80			/* all SSE exceptions clear & masked */

#endif /* CONFIG_X86_SSE */

	 /* Interrupt Descriptor Table (IDT) definition */

z_x86_idt:
	.word	(CONFIG_IDT_NUM_VECTORS * 8) - 1 /* limit: size of IDT-1 */

	/*
	 * Physical start address = 0.  When executing natively, this
	 * will be placed at the same location as the interrupt vector table
	 * setup by the BIOS (or GRUB?).
	 */

	/* physical start address */
	.long	Z_X86_PHYS_ADDR(_idt_base_address)


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

	/* Limit on GDT */
	.word Z_X86_PHYS_ADDR(_gdt_rom_end) - Z_X86_PHYS_ADDR(_gdt_rom) - 1
	/* table address: _gdt_rom */
	.long Z_X86_PHYS_ADDR(_gdt_rom)
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
