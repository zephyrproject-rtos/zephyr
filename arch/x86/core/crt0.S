/* crt0.S - crt0 module for the IA-32 boards */

/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
DESCRIPTION
This module contains the initial code executed by the Zephyr Kernel ELF image
after having been loaded into RAM.

INTERNAL
The CONFIG_PROT_MODE_SWITCH configuration option is no longer used and *all*
booting scenarios (e.g. via GRUB or any other multiboot compliant bootloader)
now assume that the system is already in 32-bit protected mode and address line
A20 is enabled. However, the code associated with CONFIG_PROT_MODE_SWITCH has
been left in place should future booting scenarios arise which require its use.
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

#ifdef CONFIG_PROT_MODE_SWITCH

	/* cold start code in 16-bit real mode */

	/*
	 * Switch 'gas' into 16-bit mode, i.e. use a default operand and
	 * address size of 16 bits since the processor is executing in
	 * 16-bit real mode.  The prevents the assembler from inserting
	 * 0x66 or 0x67 instruction prefixes when using 16-bit data values
	 * or 16-bit pointers.
	 */

	.code16

	.section ".xreset", "ax"
	.balign 16,0x90

__start:
#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	/*
	 * Record BootTime from start of Kernel.
	 * Store value temporarily in Register EDI & ESI and
         * write to memory once memory access is allowed.
         * That is, once the data segment register has been setup to access
         * the .data/.rodata/.bss section of the linked image.
	 */
        rdtsc
        mov  %eax, %esi			/* low  value */
        mov  %edx, %edi			/* high value */
#endif
	/*
	 * Ensure interrupts are disabled.  Interrupts are enabled when
	 * the first kernel thread context switch occurs.
	 */

	cli


/*
 * Platforms that need to enable the A20 line to boot properly wil enable this
 * option.
 */
#ifdef CONFIG_BOOT_A20_ENABLE

	/*
	 * Set the stack pointer to just before the start of the .text
	 * section.  This needs to be performed before any BIOS invocations.
	 *
	 * Note that attempting to set the stack pointer to beyond the end of
	 * the static ELF image (indicated via the _end symbol) may result
	 * in pointer wrap-around for images with large BSS sections
	 * (since the processor is still in "real" mode and thus the stack
	 * pointer is only a 16 bits).
	 */

	movl	$__start, %esp

	/*
	 * Before switching to 32-bit protected mode, Gate-A20 must be
	 * enabled (legacy PC hardware issue).
	 *
	 * The classical method of enabling address line A20 via the 8042
	 * keyboard controller will not be performed.  Instead, it will be
	 * assumed that the board has a relatively modern BIOS which supports
	 * the following int 15h function:
	 *
	 *      - int 15h: ax=2400 -> disable A20
	 *      - int 15h: ax=2401 -> enable A20
	 *           If successful: CF clear, AH = 00h
	 *           On error: CF set, AH = status
	 *           Status: 01h keyboard controller is in secure mode
	 *                   86h function not supported
	 *      - int 15h: ax=2402 -> query status A20
	 *           status (0: disabled, 1: enabled) is returned in AL
	 *      - int 15h: ax=2403 -> query A20 support (kbd or port 92)
	 *           status (bit 0: kbd, bit 1: port 92) is returned in BX
	 *
	 *      Return values:
	 *      If successful: carry flag (CF) clear, AH = 00h
	 *      On error: carry flag (CF) set, AH = status
	 *      Status: 01h keyboard controller is in secure mode
	 *              86h function not supported
	 *
	 * If the int15h method fails, then the system control port A
	 * (I/O port 0x92) method will be used.
	 */

	movw	$0x2401, %ax		/* ax=2401 -> enable A20 */
	int	$0x15
	jnc	A20Enabled		/* CF = 1 -> try I/O port 0x92 method */

	/* try enabling Gate-A20 via system control port A (I/O port 0x92) */

	movb	$0x02, %al
	outb	%al, $0x92

	xorl	%ecx, %ecx
A20EnableWait:
	inb	$0x92, %al
	andb	$0x02, %al
	loopz	A20EnableWait

A20Enabled:
#endif /* CONFIG_BOOT_A20_ENABLE */

	/* load Interrupt Descriptor and Global Descriptor tables (IDT & GDT) */

	lgdt	%cs:_GdtRom		/* load 32-bit operand size GDT */

	movl	%cr0, %eax		/* move CR0 to EAX */
	orl	$0x1, %eax		/* set the Protection Enable (PE) bit */
	andl	$0x9fffffff, %eax	/* Enable write-back caching */
	movl	%eax, %cr0		/* move EAX to CR0 */
	jmp	%cs:vstartProtMode	/* flush prefetch input queue */

vstartProtMode:

	/*
	 * At this point, the processor is executing in 32-bit protected
	 * mode, but the CS selector hasn't been loaded yet, i.e. the
	 * processor is still effectively executing instructions from a
	 * 16-bit code segment (thus the .code16 assembler directive still
	 * needs to be in effect.
	 */

	movw	$0x10,%ax		/* data segment selector (entry = 3) */
	movw	%ax,%ds			/* set DS */
	movw	%ax,%es			/* set ES */
	movw	%ax,%fs			/* set FS */
	movw	%ax,%gs			/* set GS */
	movw	%ax,%ss			/* set SS */

	/* this is 'ljmp $0x08:$vstart32BitCode': diab 5.9.1.0 does not encode
	 * it correctly */
	.byte 0xea
	.long vstart32BitCode
	.byte 0x08
	.byte 0x00

	/*
	 * Switch 'gas' back to 32-bit data and address defaults.  This
	 * can only be performed after loading the CS segment register
	 * (via an lcall instruction, for example) with a segment selector
	 * that describes a 32-bit code segment.
	 */
	.code32

SECTION_FUNC(TEXT_START, vstart32BitCode)

#else /* !CONFIG_PROT_MODE_SWITCH */

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
	 * the first kernel thread context switch occurs.
	 */

	cli

	/*
	 * Although the bootloader sets up an Interrupt Descriptor Table (IDT)
	 * and a Global Descriptor Table (GDT), the specification encourages
	 * booted operating systems to setup their own IDT and GDT.
	 */

	lgdt	_GdtRom		/* load 32-bit operand size GDT */

#endif /* !CONFIG_PROT_MODE_SWITCH */


	lidt	_Idt		/* load 32-bit operand size IDT */


#ifndef CONFIG_PROT_MODE_SWITCH

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

#endif /* CONFIG_PROT_MODE_SWITCH */

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


#ifdef CONFIG_PROT_MODE_SWITCH
	/* Global Descriptor Table (GDT) definition */
	.section ".xreset", "ax"
	.align 8
#endif /* CONFIG_PROT_MODE_SWITCH */

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


#ifdef CONFIG_PROT_MODE_SWITCH
	/* Reset vector */
	.code16
	.section ".xresetv", "ax"
_ResetVector:
	jmp	__start
#endif /* CONFIG_PROT_MODE_SWITCH */

_GdtRomEnd:

#ifdef CONFIG_ADVANCED_IDLE
	.section .NOINIT
	.balign 4,0x90
_AdvIdleStack:
	.fill CONFIG_ADV_IDLE_STACK_SIZE
#endif
