/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arch/x86/multiboot.h>
#include <sys/util.h>
#include <arch/x86/msr.h>
#include <kernel_arch_data.h>

#define NR_IDT_VECTOR	256	/* full IDT, we're not short of RAM */
#define IRQ_STACK_SIZE	4096	/* must be a multiple of 16 */

.section .locore,"ax"
.code32

.globl __start
__start:

	/*
	 * multiboot starts us here in 32-bit flat mode with interrupts
	 * and paging disabled. first, switch to our own GDT and stack.
	 */

	lgdt gdt48
	lidt idt48
	jmpl $X86_KERNEL_CS_32, $1f
1:	movw $X86_KERNEL_DS_32, %ax
	movw %ax, %ds
	movw %ax, %ss
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs

	movl $(irq_stack + IRQ_STACK_SIZE), %esp

	/*
	 * transition to long mode, by the book.
	 */

	movl %cr4, %eax			/* enable PAE */
	orl $CR4_PAE, %eax
	movl %eax, %cr4

	movl $pml4, %eax		/* load page base */
	movl %eax, %cr3

	movl $X86_EFER_MSR, %ecx	/* enable long mode */
	rdmsr
	orl $X86_EFER_MSR_LME, %eax
	wrmsr

	movl %cr0, %eax			/* enable paging */
	orl $CR0_PG, %eax
	movl %eax, %cr0

	/*
	 * jump into long mode, reload the segment registers (again).
	 */

	jmpl $X86_KERNEL_CS_64, $1f
.code64
1:	movl $X86_KERNEL_DS_64, %eax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movw %ax, %fs
	movw %ax, %gs

	/*
	 * func() is just a placeholder C function. don't replace CALL
	 * with JMP; we must honor the ABI stack alignment requirements.
	 */

	call func

stop:	jmp stop

/*
 * The multiboot header is identical (for obvious reasons) to the 32-bit
 * version in ia32/crt0.S. They should be refactored into a common file.
 */

#ifdef CONFIG_X86_MULTIBOOT
	.align	4
	.long	X86_MULTIBOOT_HEADER_MAGIC
	.long	X86_MULTIBOOT_HEADER_FLAGS
	.long	-(X86_MULTIBOOT_HEADER_MAGIC + X86_MULTIBOOT_HEADER_FLAGS)
#ifdef CONFIG_X86_MULTIBOOT_FRAMEBUF
	.fill	5,4,0				/* (unused exec layout) */
	.long	0				/* linear graphics mode */
	.long	CONFIG_X86_MULTIBOOT_FRAMEBUF_X	/* width */
	.long	CONFIG_X86_MULTIBOOT_FRAMEBUF_Y	/* height */
	.long	32				/* depth */
#endif /* CONFIG_X86_MULTIBOOT_FRAMEBUF */
#endif

/*
 * GDT - a single GDT is shared by all threads (and, eventually, all CPUs).
 * This layout must agree with the selectors in intel64/kernel_arch_data.h.
 */

.align 8

gdt:
	.word 0, 0, 0, 0		/* 0x00: null descriptor */
	.word 0xFFFF, 0, 0x9A00, 0x00CF	/* 0x08: 32-bit kernel code */
	.word 0xFFFF, 0, 0x9200, 0x00CF	/* 0x10: 32-bit kernel data */
	.word 0, 0, 0x9800, 0x0020	/* 0x18: 64-bit kernel code */

gdt48:
	.word (gdt48 - gdt - 1)
	.long gdt

/*
 * IDT. Empty for now.
 */

.align 8

idt:	.fill (NR_IDT_VECTOR*16), 1, 0
idt48:
	.word (idt48 - idt - 1)
	.long idt

/*
 * Page tables. Long mode requires them, but we don't implement any memory
 * protection yet, so these simply identity-map the first 4GB w/ 1GB pages.
 */

.align 4096

pml4:
	.long pdp + 0x03		/* 0x03 = R/W, P */
	.long 0
	.fill 4088, 1, 0

pdp:	.long 0x00000183		/* 0x183 = G, 1GB, R/W, P */
	.long 0
	.long 0x40000183
	.long 0
	.long 0x80000183
	.long 0
	.long 0xC0000183
	.long 0
	.fill 4064, 1, 0

/*
 * For now, the "IRQ stack" is just used as a scratch stack during
 * early kernel initialization.
 */

.align 16
irq_stack:
	.fill IRQ_STACK_SIZE, 1, 0xFF

