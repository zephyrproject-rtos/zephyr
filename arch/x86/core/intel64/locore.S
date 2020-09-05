/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <arch/x86/multiboot.h>
#include <sys/util.h>
#include <arch/x86/msr.h>
#include <kernel_arch_data.h>
#include <offsets_short.h>
#include <drivers/interrupt_controller/loapic.h>
#include <arch/cpu.h>
#include <sys/mem_manage.h>

/*
 * Definitions/macros for enabling paging
 */

/* Long mode, no-execute, syscall */
#define EFER_BITS (X86_EFER_MSR_LME | X86_EFER_MSR_NXE | X86_EFER_MSR_SCE)

/* Paging, write-protect */
#define CR0_BITS (CR0_PG | CR0_WP)

/* PAE, SSE */
#define CR4_BITS (CR4_PAE | CR4_OSFXSR)

.macro set_efer
	movl $X86_EFER_MSR, %ecx
	rdmsr
	orl $EFER_BITS, %eax
	wrmsr
.endm

.macro install_pagetables_32
	movl %cr4, %eax
	orl $CR4_BITS, %eax
	movl %eax, %cr4
	clts

	/* Page tables created at build time by gen_mmu.py */
	movl $z_x86_kernel_ptables, %eax
	movl %eax, %cr3

	set_efer

	movl %cr0, %eax
	orl $CR0_BITS, %eax
	movl %eax, %cr0
.endm

.macro install_pagetables_64
	/* Here, we are already in long mode with paging enabled and
	 * just need to switch to our own page tables, but let's be
	 * paranoid and ensure CR4, CR0, and EFER_MSR are set up
	 * exactly how we expect. Logic is the same as install_pagetables_32
	 */
	movq %cr4, %rax
	orq $CR4_BITS, %rax
	movq %rax, %cr4
	clts

	movq $z_x86_kernel_ptables, %rax
	movq %rax, %cr3

	set_efer

	movq %cr0, %rax
	/* Use 32-bit instructions due to assembler fussiness with large
	 * immediate values with `orq`, CR0_PG is bit 31. We don't ever set any
	 * high bits in cr0 anyway.
	 */
	orl $CR0_BITS, %eax
	movq %rax, %cr0
.endm

.section .locore,"ax"

#if CONFIG_MP_NUM_CPUS > 1

	/*
	 * APs are sent here on startup, in real mode. This
	 * is first because we want it on a page boundary.
	 */

.code16
.global x86_ap_start
x86_ap_start:

	/*
	 * First, we move to 32-bit protected mode, and set up the
	 * same flat environment that the BSP gets from the loader.
	 */

	lgdt gdt48
	lidt idt48
	movw $1, %ax
	lmsw %ax
	jmpl $X86_KERNEL_CS_32, $1f
.code32
1:	movw $X86_KERNEL_DS_32, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movw %ax, %fs

	/*
	 * Now, reverse-map our local APIC ID to our logical CPU ID
	 * so we can locate our x86_cpuboot[] bundle. Put it in EBP.
	 */

	movl CONFIG_LOAPIC_BASE_ADDRESS+LOAPIC_ID, %eax
	shrl $24, %eax
	andl $0x0F, %eax		/* local APIC ID -> EAX */

	movl $x86_cpuboot, %ebp
	xorl %ebx, %ebx
1:	cmpl $CONFIG_MP_NUM_CPUS, %ebx
	jz unknown_loapic_id
	cmpb %al, x86_cpu_loapics(%ebx)
	je go64				/* proceed to 64-bit mode */
	incl %ebx
	addl $__X86_CPUBOOT_SIZEOF, %ebp
	jmp 1b

unknown_loapic_id:
	jmp unknown_loapic_id

#endif /* CONFIG_MP_NUM_CPUS > 1 */

.code32
.globl __start
__start:
	/*
	 * kernel execution begins here in 32-bit mode, with flat-mode
	 * descriptors in all segment registers, interrupts disabled.
	 * first, let common code do things like detect multiboot info.
	 */

	lgdt gdt48
	lidt idt48

#include "../common.S"

	/*
	 * N.B.: if multiboot info struct is present, "common.S"
	 * has left a pointer to it in EBX. do not clobber (yet).
	 *
	 * next, clear the BSS. note we're still in 32-bit mode,
	 * so the BSS must fit entirely in the first 4GB of RAM.
	 */
	cld
	xorl %eax, %eax
	movl $__bss_start, %edi
	movl $__bss_num_dwords, %ecx
	rep stosl

	movl $x86_cpuboot, %ebp		/* BSP is always logical CPU id 0 */
	movl %ebx, __x86_cpuboot_t_arg_OFFSET(%ebp)	/* multiboot info */


go64:	/* Install page tables and transition to long mode */
	install_pagetables_32
	jmpl $X86_KERNEL_CS, $enter_code64

	/* Long mode entry point.  Arrive here from the code
	 * immediately above (shared between main CPU startup and AP
	 * startup), or from EFI entry in __start64.
	 *
	 * Here we reload the segment registers,
	 * and configure per-CPU stuff: GS, task register, stack.
	 */
	.code64
enter_code64:
	movl $X86_KERNEL_DS, %eax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movw %ax, %fs

	movw __x86_cpuboot_t_tr_OFFSET(%rbp), %ax
	ltr %ax

	/* Set up MSRs for GS / KERNEL_GS base */
	movq __x86_cpuboot_t_gs_base_OFFSET(%rbp), %rax
	movq %rax, %rdx
	shrq $32, %rdx
	/* X86_KERNEL_GS_BASE and X86_GS_BASE are swapped by the 'swapgs'
	 * instruction.
	 */
	movl $X86_KERNEL_GS_BASE, %ecx
	wrmsr
	/* X86_GS_BASE shadows base fields of %gs, effectively setting %gs */
	movl $X86_GS_BASE, %ecx
	wrmsr

	movq __x86_cpuboot_t_sp_OFFSET(%rbp), %rsp
	movq %rsp, %gs:__x86_tss64_t_ist1_OFFSET

	/* finally, complete environment for the C runtime and go. */
	cld	/* GCC presumes a clear direction flag */

#ifdef CONFIG_INIT_STACKS
	movq $0xAAAAAAAAAAAAAAAA, %rax
	movq %rsp, %rdi
	subq $CONFIG_ISR_STACK_SIZE, %rdi
	movq $(CONFIG_ISR_STACK_SIZE >> 3), %rcx
	rep stosq
#endif

	/* Enter C domain now that we have a stack set up, never to return */
	movq %rbp, %rdi
	call z_x86_cpu_init

	/* 64 bit OS entry point, used by EFI support.  UEFI
	 * guarantees an identity-mapped page table that covers
	 * physical memory, and the loader stub already used it to
	 * write all of the Zephyr image, so we know it works for what
	 * we need.  Other things need fixups to match what multiboot
	 * 32 bit startup does.
	 */
.globl __start64
__start64:
	/* Zero the TSC */
	xorq %rax, %rax
	xorq %rdx, %rdx
	movq $X86_TIME_STAMP_COUNTER_MSR, %rcx
	wrmsr

	lidt idt80
	lgdt gdt80

	install_pagetables_64

	/* Disable 8259 PIT.  Almost certainly not needed on modern
	 * UEFI platforms taking this code path, but...
	 */
	movb $0xff, %al
	outb %al, $0x21
	outb %al, $0xA1

	/* Far call into the Zephyr code segment */
	movq $x86_cpuboot, %rbp
	mov jmpdesc, %rax
	jmp *%rax
jmpdesc:
	.quad enter_code64
	.short X86_KERNEL_CS

/*
 * void x86_sse_init(struct k_thread *thread);
 *
 * Initialize floating-point state to something sane. If 'thread' is
 * not NULL, then the resulting FP state is saved to thread->arch.sse.
 */

.global x86_sse_init
x86_sse_init:
	fninit
	ldmxcsr mxcsr
	testq %rdi, %rdi
	jz 1f
	fxsave _thread_offset_to_sse(%rdi)
1:	retq

mxcsr:	.long X86_MXCSR_SANE

/*
 * void z_x86_switch(void *switch_to, void **switched_from);
 *
 * Note that switch_handle for us is simply a pointer to the containing
 * 'struct k_thread', thus:
 *
 * RDI = (struct k_thread *) switch_to
 * RSI = (struct k_thread **) address of output thread switch_handle field
 */

.globl z_x86_switch
z_x86_switch:
	/* RSI contains the switch_handle field to which we are
	 * notionally supposed to store.  Offset it to get back to the
	 * thread handle instead.
	 */
	subq $___thread_t_switch_handle_OFFSET, %rsi

	andb $~X86_THREAD_FLAG_ALL, _thread_offset_to_flags(%rsi)

	popq %rax
	movq %rax, _thread_offset_to_rip(%rsi)
	pushfq
	popq %rax
	movq %rax, _thread_offset_to_rflags(%rsi)
	movq %rsp, _thread_offset_to_rsp(%rsi)
	movq %rbx, _thread_offset_to_rbx(%rsi)
	movq %rbp, _thread_offset_to_rbp(%rsi)
	movq %r12, _thread_offset_to_r12(%rsi)
	movq %r13, _thread_offset_to_r13(%rsi)
	movq %r14, _thread_offset_to_r14(%rsi)
	movq %r15, _thread_offset_to_r15(%rsi)
#ifdef CONFIG_USERSPACE
	/* We're always in supervisor mode if we get here, the other case
	 * is when __resume is invoked from irq_dispatch
	 */
	movq $X86_KERNEL_CS, _thread_offset_to_cs(%rsi)
	movq $X86_KERNEL_DS, _thread_offset_to_ss(%rsi)
#endif
	/* Store the handle (i.e. our thread struct address) into the
	 * switch handle field, this is a synchronization signal that
	 * must occur after the last data from the old context is
	 * saved.
	 */
	movq %rsi, ___thread_t_switch_handle_OFFSET(%rsi)

	movq %gs:__x86_tss64_t_ist1_OFFSET, %rsp

	/* fall through to __resume */

/*
 * Entry:
 *   RSP = top of CPU interrupt stack
 *   RDI = (struct k_thread *) thread to resume
 */

__resume:
#ifdef CONFIG_USERSPACE
#ifndef CONFIG_X86_KPTI
	/* If KPTI is enabled we're always on the kernel's page tables in
	 * this context and the appropriate page table switch takes place
	 * when trampolining back to user mode
	 */
	pushq %rdi	/* Caller-saved, stash it */
	call z_x86_swap_update_page_tables
	popq %rdi
#endif /* CONFIG_X86_KPTI */

	/* Set up exception return stack frame */
	pushq _thread_offset_to_ss(%rdi)	/* SS */
#else
	pushq $X86_KERNEL_DS			/* SS */
#endif /* CONFIG_USERSPACE */
	pushq _thread_offset_to_rsp(%rdi)	/* RSP */
	pushq _thread_offset_to_rflags(%rdi)	/* RFLAGS */
#ifdef CONFIG_USERSPACE
	pushq _thread_offset_to_cs(%rdi)	/* CS */
#else
	pushq $X86_KERNEL_CS			/* CS */
#endif
	pushq _thread_offset_to_rip(%rdi)	/* RIP */

#ifdef CONFIG_ASSERT
	/* Poison the old thread's saved RIP pointer with a
	 * recognizable value near NULL, to easily catch reuse of the
	 * thread object across CPUs in SMP.  Strictly speaking this
	 * is not an assertion, but it's very cheap and worth having
	 * on during routine testing.
	 */
	movq $0xB9, _thread_offset_to_rip(%rdi)
#endif

	movq _thread_offset_to_rbx(%rdi), %rbx
	movq _thread_offset_to_rbp(%rdi), %rbp
	movq _thread_offset_to_r12(%rdi), %r12
	movq _thread_offset_to_r13(%rdi), %r13
	movq _thread_offset_to_r14(%rdi), %r14
	movq _thread_offset_to_r15(%rdi), %r15
#ifdef CONFIG_USERSPACE
	/* Set correct privilege elevation stack to manually switch to in
	 * z_x86_syscall_entry_stub()
	 */
	movq _thread_offset_to_psp(%rdi), %rax
	movq %rax, %gs:__x86_tss64_t_psp_OFFSET
#endif

	testb $X86_THREAD_FLAG_ALL, _thread_offset_to_flags(%rdi)
	jz 1f

	fxrstor _thread_offset_to_sse(%rdi)
	movq _thread_offset_to_rax(%rdi), %rax
	movq _thread_offset_to_rcx(%rdi), %rcx
	movq _thread_offset_to_rdx(%rdi), %rdx
	movq _thread_offset_to_rsi(%rdi), %rsi
	movq _thread_offset_to_r8(%rdi), %r8
	movq _thread_offset_to_r9(%rdi), %r9
	movq _thread_offset_to_r10(%rdi), %r10
	movq _thread_offset_to_r11(%rdi), %r11
	movq _thread_offset_to_rdi(%rdi), %rdi  /* do last :-) */

#ifdef CONFIG_USERSPACE
	/* Swap GS register values if we are returning to user mode */
	testb $0x3, 8(%rsp)
	jz 1f
#ifdef CONFIG_X86_KPTI
	jmp z_x86_trampoline_to_user
#else
	swapgs
#endif /* CONFIG_X86_KPTI */
#endif /* CONFIG_USERSPACE */
1:
#ifdef CONFIG_X86_BOUNDS_CHECK_BYPASS_MITIGATION
	/* swapgs variant of Spectre V1. Disable speculation past this point */
	lfence
#endif /* CONFIG_X86_BOUNDS_CHECK_BYPASS_MITIGATION */

	iretq




#define EXCEPT_CODE(nr) vector_ ## nr: pushq $nr; jmp except
#define EXCEPT(nr)	vector_ ## nr: pushq $0; pushq $nr; jmp except

/*
 * When we arrive at 'except' from one of the EXCEPT(X) stubs,
 * we're on the exception stack with irqs unlocked (or the trampoline stack
 * with irqs locked if KPTI is enabled) and it contains:
 *
 *   SS
 *   RSP
 *   RFLAGS
 *   CS
 *   RIP
 *   Error Code if pushed by CPU, else 0
 *   Vector number <- RSP points here
 *
 */

except: /*
	 * finish struct NANO_ESF on stack. 'vector' .. 'ss' are
	 * already there from hardware trap and EXCEPT_*() stub.
	 */

	pushq %r11

#ifdef CONFIG_USERSPACE
	/* Swap GS register values and page tables if we came from user mode */
	testb $0x3, 32(%rsp)
	jz 1f
	swapgs
#ifdef CONFIG_X86_KPTI
	/* Load kernel's page table */
	movq $z_x86_kernel_ptables, %r11
	movq %r11, %cr3
#endif /* CONFIG_X86_KPTI */
1:
#ifdef CONFIG_X86_BOUNDS_CHECK_BYPASS_MITIGATION
	/* swapgs variant of Spectre V1. Disable speculation past this point */
	lfence
#endif /* CONFIG_X86_BOUNDS_CHECK_BYPASS_MITIGATION */
#ifdef CONFIG_X86_KPTI
	/* Save old trampoline stack pointer in R11 */
	movq %rsp, %r11

	/* Switch to the exception stack */
	movq %gs:__x86_tss64_t_ist7_OFFSET, %rsp

	/* Transplant trampoline stack contents */
	pushq 56(%r11)	/* SS */
	pushq 48(%r11)	/* RSP */
	pushq 40(%r11)	/* RFLAGS */
	pushq 32(%r11)	/* CS */
	pushq 24(%r11)	/* RIP */
	pushq 16(%r11)	/* Error code */
	pushq 8(%r11)	/* Vector */
	pushq (%r11)	/* Stashed R15 */
	movq $0, (%r11) /* Cover our tracks */

	/* We're done, it's safe to re-enable interrupts. */
	sti
#endif /* CONFIG_X86_KPTI */
#endif /* CONFIG_USERSPACE */

	/* In addition to r11, push the rest of the caller-saved regs */
	/* Positioning of this fxsave is important, RSP must be 16-byte
	 * aligned
	 */
	subq $X86_FXSAVE_SIZE, %rsp
	fxsave (%rsp)
	pushq %r10
	pushq %r9
	pushq %r8
	pushq %rdi
	pushq %rsi
	pushq %rdx
	pushq %rcx
	pushq %rax
	pushq %rbp
#ifdef CONFIG_EXCEPTION_DEBUG
	/* Callee saved regs */
	pushq %r15
	pushq %r14
	pushq %r13
	pushq %r12
	pushq %rbx
#endif /* CONFIG_EXCEPTION_DEBUG */
	movq %rsp, %rdi

	call z_x86_exception

	/* If we returned, the exception was handled successfully and the
	 * thread may resume (the pushed RIP may have been modified)
	 */
#ifdef CONFIG_EXCEPTION_DEBUG
	popq %rbx
	popq %r12
	popq %r13
	popq %r14
	popq %r15
#endif /* CONFIG_EXCEPTION_DEBUG */
	popq %rbp
	popq %rax
	popq %rcx
	popq %rdx
	popq %rsi
	popq %rdi
	popq %r8
	popq %r9
	popq %r10
	fxrstor (%rsp)
	addq $X86_FXSAVE_SIZE, %rsp
	popq %r11

	/* Drop the vector/err code pushed by the HW or EXCEPT_*() stub */
	add $16, %rsp

#ifdef CONFIG_USERSPACE
	/* Swap GS register values if we are returning to user mode */
	testb $0x3, 8(%rsp)
	jz 1f
	cli
#ifdef CONFIG_X86_KPTI
	jmp z_x86_trampoline_to_user
#else
	swapgs
#endif /* CONFIG_X86_KPTI */
1:
#endif /* CONFIG_USERSPACE */

	iretq

EXCEPT      ( 0); EXCEPT      ( 1); EXCEPT      ( 2); EXCEPT      ( 3)
EXCEPT      ( 4); EXCEPT      ( 5); EXCEPT      ( 6); EXCEPT      ( 7)
EXCEPT_CODE ( 8); EXCEPT      ( 9); EXCEPT_CODE (10); EXCEPT_CODE (11)
EXCEPT_CODE (12); EXCEPT_CODE (13); EXCEPT_CODE (14); EXCEPT      (15)
EXCEPT      (16); EXCEPT_CODE (17); EXCEPT      (18); EXCEPT      (19)
EXCEPT      (20); EXCEPT      (21); EXCEPT      (22); EXCEPT      (23)
EXCEPT      (24); EXCEPT      (25); EXCEPT      (26); EXCEPT      (27)
EXCEPT      (28); EXCEPT      (29); EXCEPT      (30); EXCEPT      (31)

/* Vector reserved for handling a kernel oops; treat as an exception
 * and not an interrupt
 */
EXCEPT(Z_X86_OOPS_VECTOR);

/*
 * When we arrive at 'irq' from one of the IRQ(X) stubs,
 * we're on the "freshest" IRQ stack (or the trampoline stack if we came from
 * user mode and KPTI is enabled) and it contains:
 *
 *   SS
 *   RSP
 *   RFLAGS
 *   CS
 *   RIP
 *   (vector number - IV_IRQS) <-- RSP points here
 */

.globl x86_irq_funcs	/* see irq_manage.c .. */
.globl x86_irq_args	/* .. for these definitions */

irq:
	pushq %rsi

#ifdef CONFIG_USERSPACE
	/* Swap GS register values if we came in from user mode */
	testb $0x3, 24(%rsp)
	jz 1f
	swapgs
#ifdef CONFIG_X86_KPTI
	/* Load kernel's page table */
	movq $z_x86_kernel_ptables, %rsi
	movq %rsi, %cr3
#endif /* CONFIG_X86_KPTI */
1:
#ifdef CONFIG_X86_BOUNDS_CHECK_BYPASS_MITIGATION
	/* swapgs variant of Spectre V1. Disable speculation past this point */
	lfence
#endif /* CONFIG_X86_BOUNDS_CHECK_BYPASS_MITIGATION */
#ifdef CONFIG_X86_KPTI
	/* Save old trampoline stack pointer in RSI */
	movq %rsp, %rsi

	/* Switch to the interrupt stack stack */
	movq %gs:__x86_tss64_t_ist1_OFFSET, %rsp

	/* Transplant trampoline stack contents */
	pushq 48(%rsi)	/* SS */
	pushq 40(%rsi)	/* RSP */
	pushq 32(%rsi)	/* RFLAGS */
	pushq 24(%rsi)	/* CS */
	pushq 16(%rsi)	/* RIP */
	pushq 8(%rsi)	/* Vector */
	pushq (%rsi)	/* Stashed RSI value */
	movq $0, (%rsi) /* Cover our tracks, stashed RSI might be sensitive */
#endif /* CONFIG_X86_KPTI */
#endif /* CONFIG_USERSPACE */

	movq %gs:__x86_tss64_t_cpu_OFFSET, %rsi

	/*
	 * Bump the IRQ nesting count and move to the next IRQ stack.
	 * That's sufficient to safely re-enable interrupts, so if we
	 * haven't reached the maximum nesting depth yet, do it.
	 */

	incl ___cpu_t_nested_OFFSET(%rsi)
	subq $CONFIG_ISR_SUBSTACK_SIZE, %gs:__x86_tss64_t_ist1_OFFSET
	cmpl $CONFIG_ISR_DEPTH, ___cpu_t_nested_OFFSET(%rsi)
	jz 1f
	sti
1:	cmpl $1, ___cpu_t_nested_OFFSET(%rsi)
	je irq_enter_unnested

	/*
	 * if we're a nested interrupt, we have to dump the state to the
	 * stack. we play some games here to re-arrange the stack thusly:
	 *
	 * SS RSP RFLAGS CS RIP RAX RSI
	 * RCX RDX RDI R8 R9 R10 R11
	 * X86_FXSAVE_SIZE bytes of SSE data <-- RSP points here
	 *
	 * note that the final value of RSP must be 16-byte aligned here,
	 * both to satisfy FXSAVE/FXRSTOR but also to honor the C ABI.
	 */

irq_enter_nested: /* Nested IRQ: dump register state to stack. */
	pushq %rcx
	movq 16(%rsp), %rcx /* RCX = vector */
	movq %rax, 16(%rsp) /* looks like we pushed RAX, not the vector */
	pushq %rdx
	pushq %rdi
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	subq $X86_FXSAVE_SIZE, %rsp
	fxsave (%rsp)
	jmp irq_dispatch

irq_enter_unnested: /* Not nested: dump state to thread struct for __resume */
	movq ___cpu_t_current_OFFSET(%rsi), %rsi
	orb $X86_THREAD_FLAG_ALL, _thread_offset_to_flags(%rsi)
	fxsave _thread_offset_to_sse(%rsi)
	movq %rbx, _thread_offset_to_rbx(%rsi)
	movq %rbp, _thread_offset_to_rbp(%rsi)
	movq %r12, _thread_offset_to_r12(%rsi)
	movq %r13, _thread_offset_to_r13(%rsi)
	movq %r14, _thread_offset_to_r14(%rsi)
	movq %r15, _thread_offset_to_r15(%rsi)
	movq %rax, _thread_offset_to_rax(%rsi)
	movq %rcx, _thread_offset_to_rcx(%rsi)
	movq %rdx, _thread_offset_to_rdx(%rsi)
	movq %rdi, _thread_offset_to_rdi(%rsi)
	movq %r8, _thread_offset_to_r8(%rsi)
	movq %r9, _thread_offset_to_r9(%rsi)
	movq %r10, _thread_offset_to_r10(%rsi)
	movq %r11, _thread_offset_to_r11(%rsi)
	popq %rax /* RSI */
	movq %rax, _thread_offset_to_rsi(%rsi)
	popq %rcx /* vector number */
	popq %rax /* RIP */
	movq %rax, _thread_offset_to_rip(%rsi)
	popq %rax /* CS */
#ifdef CONFIG_USERSPACE
	movq %rax, _thread_offset_to_cs(%rsi)
#endif
	popq %rax /* RFLAGS */
	movq %rax, _thread_offset_to_rflags(%rsi)
	popq %rax /* RSP */
	movq %rax, _thread_offset_to_rsp(%rsi)
	popq %rax /* SS */
#ifdef CONFIG_USERSPACE
	movq %rax, _thread_offset_to_ss(%rsi)
#endif

irq_dispatch:
	movq x86_irq_funcs(,%rcx,8), %rbx
	movq x86_irq_args(,%rcx,8), %rdi
	call *%rbx

	xorq %rax, %rax
#ifdef CONFIG_X2APIC
	xorl %edx, %edx
	movl $(X86_X2APIC_BASE_MSR + (LOAPIC_EOI >> 4)), %ecx
	wrmsr
#else /* xAPIC */
	movq z_loapic_regs, %rdx
	movq %rax, LOAPIC_EOI(%rdx)
#endif /* CONFIG_X2APIC */

	movq %gs:__x86_tss64_t_cpu_OFFSET, %rsi

	cli
	addq $CONFIG_ISR_SUBSTACK_SIZE, %gs:__x86_tss64_t_ist1_OFFSET
	decl ___cpu_t_nested_OFFSET(%rsi)
	jnz irq_exit_nested

	/* not nested; ask the scheduler who's up next and resume it */

	movq ___cpu_t_current_OFFSET(%rsi), %rdi
	call z_get_next_switch_handle
	movq %rax, %rdi
	jmp __resume

irq_exit_nested:
	fxrstor (%rsp)
	addq $X86_FXSAVE_SIZE, %rsp
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rdi
	popq %rdx
	popq %rcx
	popq %rsi
	popq %rax
	iretq

#define IRQ(nr) vector_ ## nr: pushq $(nr - IV_IRQS); jmp irq

IRQ( 33); IRQ( 34); IRQ( 35); IRQ( 36); IRQ( 37); IRQ( 38); IRQ( 39)
IRQ( 40); IRQ( 41); IRQ( 42); IRQ( 43); IRQ( 44); IRQ( 45); IRQ( 46); IRQ( 47)
IRQ( 48); IRQ( 49); IRQ( 50); IRQ( 51); IRQ( 52); IRQ( 53); IRQ( 54); IRQ( 55)
IRQ( 56); IRQ( 57); IRQ( 58); IRQ( 59); IRQ( 60); IRQ( 61); IRQ( 62); IRQ( 63)
IRQ( 64); IRQ( 65); IRQ( 66); IRQ( 67); IRQ( 68); IRQ( 69); IRQ( 70); IRQ( 71)
IRQ( 72); IRQ( 73); IRQ( 74); IRQ( 75); IRQ( 76); IRQ( 77); IRQ( 78); IRQ( 79)
IRQ( 80); IRQ( 81); IRQ( 82); IRQ( 83); IRQ( 84); IRQ( 85); IRQ( 86); IRQ( 87)
IRQ( 88); IRQ( 89); IRQ( 90); IRQ( 91); IRQ( 92); IRQ( 93); IRQ( 94); IRQ( 95)
IRQ( 96); IRQ( 97); IRQ( 98); IRQ( 99); IRQ(100); IRQ(101); IRQ(102); IRQ(103)
IRQ(104); IRQ(105); IRQ(106); IRQ(107); IRQ(108); IRQ(109); IRQ(110); IRQ(111)
IRQ(112); IRQ(113); IRQ(114); IRQ(115); IRQ(116); IRQ(117); IRQ(118); IRQ(119)
IRQ(120); IRQ(121); IRQ(122); IRQ(123); IRQ(124); IRQ(125); IRQ(126); IRQ(127)
IRQ(128); IRQ(129); IRQ(130); IRQ(131); IRQ(132); IRQ(133); IRQ(134); IRQ(135)
IRQ(136); IRQ(137); IRQ(138); IRQ(139); IRQ(140); IRQ(141); IRQ(142); IRQ(143)
IRQ(144); IRQ(145); IRQ(146); IRQ(147); IRQ(148); IRQ(149); IRQ(150); IRQ(151)
IRQ(152); IRQ(153); IRQ(154); IRQ(155); IRQ(156); IRQ(157); IRQ(158); IRQ(159)
IRQ(160); IRQ(161); IRQ(162); IRQ(163); IRQ(164); IRQ(165); IRQ(166); IRQ(167)
IRQ(168); IRQ(169); IRQ(170); IRQ(171); IRQ(172); IRQ(173); IRQ(174); IRQ(175)
IRQ(176); IRQ(177); IRQ(178); IRQ(179); IRQ(180); IRQ(181); IRQ(182); IRQ(183)
IRQ(184); IRQ(185); IRQ(186); IRQ(187); IRQ(188); IRQ(189); IRQ(190); IRQ(191)
IRQ(192); IRQ(193); IRQ(194); IRQ(195); IRQ(196); IRQ(197); IRQ(198); IRQ(199)
IRQ(200); IRQ(201); IRQ(202); IRQ(203); IRQ(204); IRQ(205); IRQ(206); IRQ(207)
IRQ(208); IRQ(209); IRQ(210); IRQ(211); IRQ(212); IRQ(213); IRQ(214); IRQ(215)
IRQ(216); IRQ(217); IRQ(218); IRQ(219); IRQ(220); IRQ(221); IRQ(222); IRQ(223)
IRQ(224); IRQ(225); IRQ(226); IRQ(227); IRQ(228); IRQ(229); IRQ(230); IRQ(231)
IRQ(232); IRQ(233); IRQ(234); IRQ(235); IRQ(236); IRQ(237); IRQ(238); IRQ(239)
IRQ(240); IRQ(241); IRQ(242); IRQ(243); IRQ(244); IRQ(245); IRQ(246); IRQ(247)
IRQ(248); IRQ(249); IRQ(250); IRQ(251); IRQ(252); IRQ(253); IRQ(254); IRQ(255)

.section .lorodata,"a"

/*
 * IDT.
 */

/* Descriptor type. Traps don't implicitly disable interrupts. User variants
 * can be invoked by software running in user mode (ring 3).
 *
 * For KPTI everything lands on the trampoline stack and we must get off of
 * it before re-enabling interrupts; use interrupt gates for everything.
 */
#define INTR		0x8e
#define USER_INTR	0xee
#ifdef CONFIG_X86_KPTI
#define TRAP		INTR
#define USER_TRAP	UINTR
#else
#define TRAP		0x8f
#define USER_TRAP	0xef
#endif

#define IDT(nr, type, ist) \
	.word vector_ ## nr, X86_KERNEL_CS; \
	.byte ist, type; \
	.word 0, 0, 0, 0, 0

/* Which IST entry in TSS to use for automatic stack switching, or 0 if
 * no automatic switch is to take place. Stack page must be present in
 * the current page tables, if KPTI is on only the trampoline stack and
 * the current user stack can be accessed.
 */
#ifdef CONFIG_X86_KPTI
/* Everything lands on ist2, which is set to the trampoline stack.
 * Interrupt/exception entry updates page tables and manually switches to
 * the irq/exception stacks stored in ist1/ist7
 */
#define	IRQ_STACK	2
#define EXC_STACK	2
#define BAD_STACK	2
#else
#define	IRQ_STACK	1
#define EXC_STACK	7
#define BAD_STACK	7 /* Horrible things: NMIs, double faults, MCEs */
#endif

.align 16
idt:
	IDT(  0, TRAP, EXC_STACK); IDT(  1, TRAP, EXC_STACK)
	IDT(  2, TRAP, BAD_STACK); IDT(  3, TRAP, EXC_STACK)
	IDT(  4, TRAP, EXC_STACK); IDT(  5, TRAP, EXC_STACK)
	IDT(  6, TRAP, EXC_STACK); IDT(  7, TRAP, EXC_STACK)
	IDT(  8, TRAP, BAD_STACK); IDT(  9, TRAP, EXC_STACK)
	IDT( 10, TRAP, EXC_STACK); IDT( 11, TRAP, EXC_STACK)
	IDT( 12, TRAP, EXC_STACK); IDT( 13, TRAP, EXC_STACK)
	IDT( 14, TRAP, EXC_STACK); IDT( 15, TRAP, EXC_STACK)
	IDT( 16, TRAP, EXC_STACK); IDT( 17, TRAP, EXC_STACK)
	IDT( 18, TRAP, BAD_STACK); IDT( 19, TRAP, EXC_STACK)
	IDT( 20, TRAP, EXC_STACK); IDT( 21, TRAP, EXC_STACK)
	IDT( 22, TRAP, EXC_STACK); IDT( 23, TRAP, EXC_STACK)
	IDT( 24, TRAP, EXC_STACK); IDT( 25, TRAP, EXC_STACK)
	IDT( 26, TRAP, EXC_STACK); IDT( 27, TRAP, EXC_STACK)
	IDT( 28, TRAP, EXC_STACK); IDT( 29, TRAP, EXC_STACK)
	IDT( 30, TRAP, EXC_STACK); IDT( 31, TRAP, EXC_STACK)

	/* Oops vector can be invoked from Ring 3 and runs on exception stack */
	IDT(Z_X86_OOPS_VECTOR, USER_INTR, EXC_STACK); IDT( 33, INTR, IRQ_STACK)
	IDT( 34, INTR, IRQ_STACK); IDT( 35, INTR, IRQ_STACK)
	IDT( 36, INTR, IRQ_STACK); IDT( 37, INTR, IRQ_STACK)
	IDT( 38, INTR, IRQ_STACK); IDT( 39, INTR, IRQ_STACK)
	IDT( 40, INTR, IRQ_STACK); IDT( 41, INTR, IRQ_STACK)
	IDT( 42, INTR, IRQ_STACK); IDT( 43, INTR, IRQ_STACK)
	IDT( 44, INTR, IRQ_STACK); IDT( 45, INTR, IRQ_STACK)
	IDT( 46, INTR, IRQ_STACK); IDT( 47, INTR, IRQ_STACK)
	IDT( 48, INTR, IRQ_STACK); IDT( 49, INTR, IRQ_STACK)
	IDT( 50, INTR, IRQ_STACK); IDT( 51, INTR, IRQ_STACK)
	IDT( 52, INTR, IRQ_STACK); IDT( 53, INTR, IRQ_STACK)
	IDT( 54, INTR, IRQ_STACK); IDT( 55, INTR, IRQ_STACK)
	IDT( 56, INTR, IRQ_STACK); IDT( 57, INTR, IRQ_STACK)
	IDT( 58, INTR, IRQ_STACK); IDT( 59, INTR, IRQ_STACK)
	IDT( 60, INTR, IRQ_STACK); IDT( 61, INTR, IRQ_STACK)
	IDT( 62, INTR, IRQ_STACK); IDT( 63, INTR, IRQ_STACK)
	IDT( 64, INTR, IRQ_STACK); IDT( 65, INTR, IRQ_STACK)
	IDT( 66, INTR, IRQ_STACK); IDT( 67, INTR, IRQ_STACK)
	IDT( 68, INTR, IRQ_STACK); IDT( 69, INTR, IRQ_STACK)
	IDT( 70, INTR, IRQ_STACK); IDT( 71, INTR, IRQ_STACK)
	IDT( 72, INTR, IRQ_STACK); IDT( 73, INTR, IRQ_STACK)
	IDT( 74, INTR, IRQ_STACK); IDT( 75, INTR, IRQ_STACK)
	IDT( 76, INTR, IRQ_STACK); IDT( 77, INTR, IRQ_STACK)
	IDT( 78, INTR, IRQ_STACK); IDT( 79, INTR, IRQ_STACK)
	IDT( 80, INTR, IRQ_STACK); IDT( 81, INTR, IRQ_STACK)
	IDT( 82, INTR, IRQ_STACK); IDT( 83, INTR, IRQ_STACK)
	IDT( 84, INTR, IRQ_STACK); IDT( 85, INTR, IRQ_STACK)
	IDT( 86, INTR, IRQ_STACK); IDT( 87, INTR, IRQ_STACK)
	IDT( 88, INTR, IRQ_STACK); IDT( 89, INTR, IRQ_STACK)
	IDT( 90, INTR, IRQ_STACK); IDT( 91, INTR, IRQ_STACK)
	IDT( 92, INTR, IRQ_STACK); IDT( 93, INTR, IRQ_STACK)
	IDT( 94, INTR, IRQ_STACK); IDT( 95, INTR, IRQ_STACK)
	IDT( 96, INTR, IRQ_STACK); IDT( 97, INTR, IRQ_STACK)
	IDT( 98, INTR, IRQ_STACK); IDT( 99, INTR, IRQ_STACK)
	IDT(100, INTR, IRQ_STACK); IDT(101, INTR, IRQ_STACK)
	IDT(102, INTR, IRQ_STACK); IDT(103, INTR, IRQ_STACK)
	IDT(104, INTR, IRQ_STACK); IDT(105, INTR, IRQ_STACK)
	IDT(106, INTR, IRQ_STACK); IDT(107, INTR, IRQ_STACK)
	IDT(108, INTR, IRQ_STACK); IDT(109, INTR, IRQ_STACK)
	IDT(110, INTR, IRQ_STACK); IDT(111, INTR, IRQ_STACK)
	IDT(112, INTR, IRQ_STACK); IDT(113, INTR, IRQ_STACK)
	IDT(114, INTR, IRQ_STACK); IDT(115, INTR, IRQ_STACK)
	IDT(116, INTR, IRQ_STACK); IDT(117, INTR, IRQ_STACK)
	IDT(118, INTR, IRQ_STACK); IDT(119, INTR, IRQ_STACK)
	IDT(120, INTR, IRQ_STACK); IDT(121, INTR, IRQ_STACK)
	IDT(122, INTR, IRQ_STACK); IDT(123, INTR, IRQ_STACK)
	IDT(124, INTR, IRQ_STACK); IDT(125, INTR, IRQ_STACK)
	IDT(126, INTR, IRQ_STACK); IDT(127, INTR, IRQ_STACK)
	IDT(128, INTR, IRQ_STACK); IDT(129, INTR, IRQ_STACK)
	IDT(130, INTR, IRQ_STACK); IDT(131, INTR, IRQ_STACK)
	IDT(132, INTR, IRQ_STACK); IDT(133, INTR, IRQ_STACK)
	IDT(134, INTR, IRQ_STACK); IDT(135, INTR, IRQ_STACK)
	IDT(136, INTR, IRQ_STACK); IDT(137, INTR, IRQ_STACK)
	IDT(138, INTR, IRQ_STACK); IDT(139, INTR, IRQ_STACK)
	IDT(140, INTR, IRQ_STACK); IDT(141, INTR, IRQ_STACK)
	IDT(142, INTR, IRQ_STACK); IDT(143, INTR, IRQ_STACK)
	IDT(144, INTR, IRQ_STACK); IDT(145, INTR, IRQ_STACK)
	IDT(146, INTR, IRQ_STACK); IDT(147, INTR, IRQ_STACK)
	IDT(148, INTR, IRQ_STACK); IDT(149, INTR, IRQ_STACK)
	IDT(150, INTR, IRQ_STACK); IDT(151, INTR, IRQ_STACK)
	IDT(152, INTR, IRQ_STACK); IDT(153, INTR, IRQ_STACK)
	IDT(154, INTR, IRQ_STACK); IDT(155, INTR, IRQ_STACK)
	IDT(156, INTR, IRQ_STACK); IDT(157, INTR, IRQ_STACK)
	IDT(158, INTR, IRQ_STACK); IDT(159, INTR, IRQ_STACK)
	IDT(160, INTR, IRQ_STACK); IDT(161, INTR, IRQ_STACK)
	IDT(162, INTR, IRQ_STACK); IDT(163, INTR, IRQ_STACK)
	IDT(164, INTR, IRQ_STACK); IDT(165, INTR, IRQ_STACK)
	IDT(166, INTR, IRQ_STACK); IDT(167, INTR, IRQ_STACK)
	IDT(168, INTR, IRQ_STACK); IDT(169, INTR, IRQ_STACK)
	IDT(170, INTR, IRQ_STACK); IDT(171, INTR, IRQ_STACK)
	IDT(172, INTR, IRQ_STACK); IDT(173, INTR, IRQ_STACK)
	IDT(174, INTR, IRQ_STACK); IDT(175, INTR, IRQ_STACK)
	IDT(176, INTR, IRQ_STACK); IDT(177, INTR, IRQ_STACK)
	IDT(178, INTR, IRQ_STACK); IDT(179, INTR, IRQ_STACK)
	IDT(180, INTR, IRQ_STACK); IDT(181, INTR, IRQ_STACK)
	IDT(182, INTR, IRQ_STACK); IDT(183, INTR, IRQ_STACK)
	IDT(184, INTR, IRQ_STACK); IDT(185, INTR, IRQ_STACK)
	IDT(186, INTR, IRQ_STACK); IDT(187, INTR, IRQ_STACK)
	IDT(188, INTR, IRQ_STACK); IDT(189, INTR, IRQ_STACK)
	IDT(190, INTR, IRQ_STACK); IDT(191, INTR, IRQ_STACK)
	IDT(192, INTR, IRQ_STACK); IDT(193, INTR, IRQ_STACK)
	IDT(194, INTR, IRQ_STACK); IDT(195, INTR, IRQ_STACK)
	IDT(196, INTR, IRQ_STACK); IDT(197, INTR, IRQ_STACK)
	IDT(198, INTR, IRQ_STACK); IDT(199, INTR, IRQ_STACK)
	IDT(200, INTR, IRQ_STACK); IDT(201, INTR, IRQ_STACK)
	IDT(202, INTR, IRQ_STACK); IDT(203, INTR, IRQ_STACK)
	IDT(204, INTR, IRQ_STACK); IDT(205, INTR, IRQ_STACK)
	IDT(206, INTR, IRQ_STACK); IDT(207, INTR, IRQ_STACK)
	IDT(208, INTR, IRQ_STACK); IDT(209, INTR, IRQ_STACK)
	IDT(210, INTR, IRQ_STACK); IDT(211, INTR, IRQ_STACK)
	IDT(212, INTR, IRQ_STACK); IDT(213, INTR, IRQ_STACK)
	IDT(214, INTR, IRQ_STACK); IDT(215, INTR, IRQ_STACK)
	IDT(216, INTR, IRQ_STACK); IDT(217, INTR, IRQ_STACK)
	IDT(218, INTR, IRQ_STACK); IDT(219, INTR, IRQ_STACK)
	IDT(220, INTR, IRQ_STACK); IDT(221, INTR, IRQ_STACK)
	IDT(222, INTR, IRQ_STACK); IDT(223, INTR, IRQ_STACK)
	IDT(224, INTR, IRQ_STACK); IDT(225, INTR, IRQ_STACK)
	IDT(226, INTR, IRQ_STACK); IDT(227, INTR, IRQ_STACK)
	IDT(228, INTR, IRQ_STACK); IDT(229, INTR, IRQ_STACK)
	IDT(230, INTR, IRQ_STACK); IDT(231, INTR, IRQ_STACK)
	IDT(232, INTR, IRQ_STACK); IDT(233, INTR, IRQ_STACK)
	IDT(234, INTR, IRQ_STACK); IDT(235, INTR, IRQ_STACK)
	IDT(236, INTR, IRQ_STACK); IDT(237, INTR, IRQ_STACK)
	IDT(238, INTR, IRQ_STACK); IDT(239, INTR, IRQ_STACK)
	IDT(240, INTR, IRQ_STACK); IDT(241, INTR, IRQ_STACK)
	IDT(242, INTR, IRQ_STACK); IDT(243, INTR, IRQ_STACK)
	IDT(244, INTR, IRQ_STACK); IDT(245, INTR, IRQ_STACK)
	IDT(246, INTR, IRQ_STACK); IDT(247, INTR, IRQ_STACK)
	IDT(248, INTR, IRQ_STACK); IDT(249, INTR, IRQ_STACK)
	IDT(250, INTR, IRQ_STACK); IDT(251, INTR, IRQ_STACK)
	IDT(252, INTR, IRQ_STACK); IDT(253, INTR, IRQ_STACK)
	IDT(254, INTR, IRQ_STACK); IDT(255, INTR, IRQ_STACK)
idt_end:

idt48:  /* LIDT descriptor for 32 bit mode */
	.word (idt_end - idt - 1)
	.long idt

idt80:  /* LIDT descriptor for 64 bit mode */
	.word (idt_end - idt - 1)
	.quad idt

.section .gdt,"ad"

/*
 * GDT - a single GDT is shared by all threads (and, eventually, all CPUs).
 * This layout must agree with the selectors in
 * include/arch/x86/intel64/thread.h.
 *
 * The 64-bit kernel code and data segment descriptors must be in sequence as
 * required by 'syscall'
 *
 * The 32-bit user code, 64-bit user code, and 64-bit user data segment
 * descriptors must be in sequence as required by 'sysret'
 */
.align 8

gdt:
	.word 0,      0, 0,      0	/* 0x00: null descriptor */
	.word 0xFFFF, 0, 0x9A00, 0x00CF	/* 0x08: 32-bit kernel code */
	.word 0xFFFF, 0, 0x9200, 0x00CF	/* 0x10: 32-bit kernel data */
	.word 0,      0, 0x9800, 0x0020	/* 0x18: 64-bit kernel code */
	.word 0,      0, 0x9200, 0x0000	/* 0x20: 64-bit kernel data */
	.word 0xFFFF, 0, 0xFA00, 0x00CF /* 0x28: 32-bit user code (unused) */
	.word 0,      0, 0xF200, 0x0000	/* 0x30: 64-bit user data */
	.word 0,      0, 0xF800, 0x0020	/* 0x38: 64-bit user code */

	/* Remaining entries are TSS for each enabled CPU */

	.word __X86_TSS64_SIZEOF-1	/* 0x40: 64-bit TSS (16-byte entry) */
	.word tss0
	.word 0x8900
	.word 0, 0, 0, 0, 0

#if CONFIG_MP_NUM_CPUS > 1
	.word __X86_TSS64_SIZEOF-1	/* 0x50: 64-bit TSS (16-byte entry) */
	.word tss1
	.word 0x8900
	.word 0, 0, 0, 0, 0
#endif

#if CONFIG_MP_NUM_CPUS > 2
	.word __X86_TSS64_SIZEOF-1	/* 0x60: 64-bit TSS (16-byte entry) */
	.word tss2
	.word 0x8900
	.word 0, 0, 0, 0, 0
#endif

#if CONFIG_MP_NUM_CPUS > 3
	.word __X86_TSS64_SIZEOF-1	/* 0x70: 64-bit TSS (16-byte entry) */
	.word tss3
	.word 0x8900
	.word 0, 0, 0, 0, 0
#endif

gdt_end:

gdt48:  /* LGDT descriptor for 32 bit mode */
	.word (gdt_end - gdt - 1)
	.long gdt

gdt80:  /* LGDT descriptor for long mode */
	.word (gdt_end - gdt - 1)
	.quad gdt
.section .lodata,"ad"

/*
 * Known-good stack for handling CPU exceptions.
 */

.global _exception_stack
.align 16
_exception_stack:
	.fill CONFIG_EXCEPTION_STACK_SIZE, 1, 0xAA

#if CONFIG_MP_NUM_CPUS > 1
.global _exception_stack1
.align 16
_exception_stack1:
	.fill CONFIG_EXCEPTION_STACK_SIZE, 1, 0xAA
#endif

#if CONFIG_MP_NUM_CPUS > 2
.global _exception_stack2
.align 16
_exception_stack2:
	.fill CONFIG_EXCEPTION_STACK_SIZE, 1, 0xAA
#endif

#if CONFIG_MP_NUM_CPUS > 3
.global _exception_stack3
.align 16
_exception_stack3:
	.fill CONFIG_EXCEPTION_STACK_SIZE, 1, 0xAA
#endif

#ifdef CONFIG_X86_KPTI
.section .trampolines,"ad"

.global z_x86_trampoline_stack
.align 16
z_x86_trampoline_stack:
	.fill Z_X86_TRAMPOLINE_STACK_SIZE, 1, 0xAA

#if CONFIG_MP_NUM_CPUS > 1
.global z_x86_trampoline_stack1
.align 16
z_x86_trampoline_stack1:
	.fill Z_X86_TRAMPOLINE_STACK_SIZE, 1, 0xAA
#endif

#if CONFIG_MP_NUM_CPUS > 2
.global z_x86_trampoline_stack2
.align 16
z_x86_trampoline_stack2:
	.fill Z_X86_TRAMPOLINE_STACK_SIZE, 1, 0xAA
#endif

#if CONFIG_MP_NUM_CPUS > 3
.global z_x86_trampoline_stack3
.align 16
z_x86_trampoline_stack3:
	.fill Z_X86_TRAMPOLINE_STACK_SIZE, 1, 0xAA
#endif
#endif /* CONFIG_X86_KPTI */
