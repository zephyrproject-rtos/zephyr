/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arch/x86/multiboot.h>
#include <sys/util.h>
#include <arch/x86/msr.h>
#include <kernel_arch_data.h>
#include <offsets_short.h>
#include <drivers/interrupt_controller/loapic.h>
#include <arch/cpu.h>

.section .locore,"ax"
.code32

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
	movw %ax, %gs

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

	/*
	 * transition to long mode, reload the segment registers,
	 * and configure per-CPU stuff: GS, task register, stack.
	 */

go64:	movl %cr4, %eax			/* enable PAE and SSE */
	orl $(CR4_PAE | CR4_OSFXSR), %eax
	movl %eax, %cr4
	clts

#ifdef CONFIG_X86_MMU
	movl __x86_cpuboot_t_ptables_OFFSET(%ebp), %eax
#else
	movl $z_x86_flat_ptables, %eax
#endif
	movl %eax, %cr3

	movl $X86_EFER_MSR, %ecx	/* enable long mode and no-execute */
	rdmsr
	orl $(X86_EFER_MSR_LME | X86_EFER_MSR_NXE), %eax
	wrmsr

	movl %cr0, %eax			/* enable paging */
	orl $(CR0_PG | CR0_WP), %eax
	movl %eax, %cr0

	jmpl $X86_KERNEL_CS, $1f
.code64
1:	movl $X86_KERNEL_DS, %eax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movw %ax, %fs

	movw __x86_cpuboot_t_tr_OFFSET(%rbp), %ax
	ltr %ax
	movw __x86_cpuboot_t_gs_OFFSET(%rbp), %ax
	movw %ax, %gs

	movq __x86_cpuboot_t_sp_OFFSET(%rbp), %rsp
	movq %rsp, %gs:__x86_tss64_t_ist1_OFFSET

	/*
	 * finally, complete environment for the C runtime and go.
	 */

	cld	/* GCC presumes a clear direction flag */

#ifdef CONFIG_INIT_STACKS
	movq $0xAAAAAAAAAAAAAAAA, %rax
	movq %rsp, %rdi
	subq $CONFIG_ISR_STACK_SIZE, %rdi
	movq $(CONFIG_ISR_STACK_SIZE >> 3), %rcx
	rep stosq
#endif

	xorl %edi, %edi
	call x86_sse_init

#ifdef CONFIG_LOAPIC
	call z_loapic_enable
#endif

	/* don't replace CALL with JMP; honor the ABI stack alignment! */

	incl __x86_cpuboot_t_ready_OFFSET(%rbp)
	movq __x86_cpuboot_t_arg_OFFSET(%rbp), %rsi
	call *__x86_cpuboot_t_fn_OFFSET(%rbp) /* enter kernel; never return */

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
	movq %rsp, _thread_offset_to_rsp(%rsi)
	movq %rbx, _thread_offset_to_rbx(%rsi)
	movq %rbp, _thread_offset_to_rbp(%rsi)
	movq %r12, _thread_offset_to_r12(%rsi)
	movq %r13, _thread_offset_to_r13(%rsi)
	movq %r14, _thread_offset_to_r14(%rsi)
	movq %r15, _thread_offset_to_r15(%rsi)

	movq %gs:__x86_tss64_t_ist1_OFFSET, %rsp

	/* fall through to __resume */

/*
 * Entry:
 *   RSP = top of CPU interrupt stack
 *   RDI = (struct k_thread *) thread to resume
 */

__resume:
	pushq $X86_KERNEL_DS			/* SS */
	pushq _thread_offset_to_rsp(%rdi)	/* RSP */
	pushq _thread_offset_to_rflags(%rdi)	/* RFLAGS */
	pushq $X86_KERNEL_CS			/* CS */
	pushq _thread_offset_to_rip(%rdi)	/* RIP */

	movq _thread_offset_to_rbx(%rdi), %rbx
	movq _thread_offset_to_rbp(%rdi), %rbp
	movq _thread_offset_to_r12(%rdi), %r12
	movq _thread_offset_to_r13(%rdi), %r13
	movq _thread_offset_to_r14(%rdi), %r14
	movq _thread_offset_to_r15(%rdi), %r15

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

1:	iretq




#define EXCEPT_CODE(nr) vector_ ## nr: pushq $nr; jmp except
#define EXCEPT(nr)	vector_ ## nr: pushq $0; pushq $nr; jmp except

except: /*
	 * finish struct NANO_ESF on stack. 'vector' .. 'ss' are
	 * already there from hardware trap and EXCEPT_*() stub.
	 */

	pushq %r15
	pushq %r14
	pushq %r13
	pushq %r12
	pushq %r11
	pushq %r10
	pushq %r9
	pushq %r8
	pushq %rdi
	pushq %rsi
	pushq %rbp
	pushq %rdx
	pushq %rcx
	pushq %rbx
	pushq %rax

	movq %rsp, %rdi
	call z_x86_exception

	hlt /* should not return, but just in case .. */

EXCEPT      ( 0); EXCEPT      ( 1); EXCEPT      ( 2); EXCEPT      ( 3)
EXCEPT      ( 4); EXCEPT      ( 5); EXCEPT      ( 6); EXCEPT      ( 7)
EXCEPT_CODE ( 8); EXCEPT      ( 9); EXCEPT_CODE (10); EXCEPT_CODE (11)
EXCEPT_CODE (12); EXCEPT_CODE (13); EXCEPT_CODE (14); EXCEPT      (15)
EXCEPT      (16); EXCEPT_CODE (17); EXCEPT      (18); EXCEPT      (19)
EXCEPT      (20); EXCEPT      (21); EXCEPT      (22); EXCEPT      (23)
EXCEPT      (24); EXCEPT      (25); EXCEPT      (26); EXCEPT      (27)
EXCEPT      (28); EXCEPT      (29); EXCEPT      (30); EXCEPT      (31)

/*
 * When we arrive at 'irq' from one of the IRQ(X) stubs,
 * we're on the "freshest" IRQ stack and it contains:
 *
 *   SS
 *   RSP
 *   RFLAGS
 *   CS
 *   RIP
 *   (vector number - IV_IRQS) <-- RSP points here
 *   RSI <-- we push this on entry
 */

.globl x86_irq_funcs	/* see irq_manage.c .. */
.globl x86_irq_args	/* .. for these definitions */

irq:
	pushq %rsi
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
	popq %rax /* CS: discard */
	popq %rax /* RFLAGS */
	movq %rax, _thread_offset_to_rflags(%rsi)
	popq %rax /* RSP */
	movq %rax, _thread_offset_to_rsp(%rsi)
	popq %rax /* SS: discard */

irq_dispatch:
	movq x86_irq_funcs(,%rcx,8), %rbx
	movq x86_irq_args(,%rcx,8), %rdi
	call *%rbx

	xorl %eax, %eax
#ifdef CONFIG_X2APIC
	xorl %edx, %edx
	movl $(X86_X2APIC_BASE_MSR + (LOAPIC_EOI >> 4)), %ecx
	wrmsr
#else /* xAPIC */
	movl %eax, (CONFIG_LOAPIC_BASE_ADDRESS + LOAPIC_EOI)
#endif

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

IRQ( 32); IRQ( 33); IRQ( 34); IRQ( 35); IRQ( 36); IRQ( 37); IRQ( 38); IRQ( 39)
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

#define TRAP 0x8f
#define INTR 0x8e

#define IDT(nr, type, ist) \
	.word vector_ ## nr, X86_KERNEL_CS; \
	.byte ist, type; \
	.word 0, 0, 0, 0, 0

.align 16
idt:
    IDT(  0, TRAP, 7); IDT(  1, TRAP, 7); IDT(  2, TRAP, 7); IDT(  3, TRAP, 7)
    IDT(  4, TRAP, 7); IDT(  5, TRAP, 7); IDT(  6, TRAP, 7); IDT(  7, TRAP, 7)
    IDT(  8, TRAP, 7); IDT(  9, TRAP, 7); IDT( 10, TRAP, 7); IDT( 11, TRAP, 7)
    IDT( 12, TRAP, 7); IDT( 13, TRAP, 7); IDT( 14, TRAP, 7); IDT( 15, TRAP, 7)
    IDT( 16, TRAP, 7); IDT( 17, TRAP, 7); IDT( 18, TRAP, 7); IDT( 19, TRAP, 7)
    IDT( 20, TRAP, 7); IDT( 21, TRAP, 7); IDT( 22, TRAP, 7); IDT( 23, TRAP, 7)
    IDT( 24, TRAP, 7); IDT( 25, TRAP, 7); IDT( 26, TRAP, 7); IDT( 27, TRAP, 7)
    IDT( 28, TRAP, 7); IDT( 29, TRAP, 7); IDT( 30, TRAP, 7); IDT( 31, TRAP, 7)

    IDT( 32, INTR, 1); IDT( 33, INTR, 1); IDT( 34, INTR, 1); IDT( 35, INTR, 1)
    IDT( 36, INTR, 1); IDT( 37, INTR, 1); IDT( 38, INTR, 1); IDT( 39, INTR, 1)
    IDT( 40, INTR, 1); IDT( 41, INTR, 1); IDT( 42, INTR, 1); IDT( 43, INTR, 1)
    IDT( 44, INTR, 1); IDT( 45, INTR, 1); IDT( 46, INTR, 1); IDT( 47, INTR, 1)
    IDT( 48, INTR, 1); IDT( 49, INTR, 1); IDT( 50, INTR, 1); IDT( 51, INTR, 1)
    IDT( 52, INTR, 1); IDT( 53, INTR, 1); IDT( 54, INTR, 1); IDT( 55, INTR, 1)
    IDT( 56, INTR, 1); IDT( 57, INTR, 1); IDT( 58, INTR, 1); IDT( 59, INTR, 1)
    IDT( 60, INTR, 1); IDT( 61, INTR, 1); IDT( 62, INTR, 1); IDT( 63, INTR, 1)
    IDT( 64, INTR, 1); IDT( 65, INTR, 1); IDT( 66, INTR, 1); IDT( 67, INTR, 1)
    IDT( 68, INTR, 1); IDT( 69, INTR, 1); IDT( 70, INTR, 1); IDT( 71, INTR, 1)
    IDT( 72, INTR, 1); IDT( 73, INTR, 1); IDT( 74, INTR, 1); IDT( 75, INTR, 1)
    IDT( 76, INTR, 1); IDT( 77, INTR, 1); IDT( 78, INTR, 1); IDT( 79, INTR, 1)
    IDT( 80, INTR, 1); IDT( 81, INTR, 1); IDT( 82, INTR, 1); IDT( 83, INTR, 1)
    IDT( 84, INTR, 1); IDT( 85, INTR, 1); IDT( 86, INTR, 1); IDT( 87, INTR, 1)
    IDT( 88, INTR, 1); IDT( 89, INTR, 1); IDT( 90, INTR, 1); IDT( 91, INTR, 1)
    IDT( 92, INTR, 1); IDT( 93, INTR, 1); IDT( 94, INTR, 1); IDT( 95, INTR, 1)
    IDT( 96, INTR, 1); IDT( 97, INTR, 1); IDT( 98, INTR, 1); IDT( 99, INTR, 1)
    IDT(100, INTR, 1); IDT(101, INTR, 1); IDT(102, INTR, 1); IDT(103, INTR, 1)
    IDT(104, INTR, 1); IDT(105, INTR, 1); IDT(106, INTR, 1); IDT(107, INTR, 1)
    IDT(108, INTR, 1); IDT(109, INTR, 1); IDT(110, INTR, 1); IDT(111, INTR, 1)
    IDT(112, INTR, 1); IDT(113, INTR, 1); IDT(114, INTR, 1); IDT(115, INTR, 1)
    IDT(116, INTR, 1); IDT(117, INTR, 1); IDT(118, INTR, 1); IDT(119, INTR, 1)
    IDT(120, INTR, 1); IDT(121, INTR, 1); IDT(122, INTR, 1); IDT(123, INTR, 1)
    IDT(124, INTR, 1); IDT(125, INTR, 1); IDT(126, INTR, 1); IDT(127, INTR, 1)
    IDT(128, INTR, 1); IDT(129, INTR, 1); IDT(130, INTR, 1); IDT(131, INTR, 1)
    IDT(132, INTR, 1); IDT(133, INTR, 1); IDT(134, INTR, 1); IDT(135, INTR, 1)
    IDT(136, INTR, 1); IDT(137, INTR, 1); IDT(138, INTR, 1); IDT(139, INTR, 1)
    IDT(140, INTR, 1); IDT(141, INTR, 1); IDT(142, INTR, 1); IDT(143, INTR, 1)
    IDT(144, INTR, 1); IDT(145, INTR, 1); IDT(146, INTR, 1); IDT(147, INTR, 1)
    IDT(148, INTR, 1); IDT(149, INTR, 1); IDT(150, INTR, 1); IDT(151, INTR, 1)
    IDT(152, INTR, 1); IDT(153, INTR, 1); IDT(154, INTR, 1); IDT(155, INTR, 1)
    IDT(156, INTR, 1); IDT(157, INTR, 1); IDT(158, INTR, 1); IDT(159, INTR, 1)
    IDT(160, INTR, 1); IDT(161, INTR, 1); IDT(162, INTR, 1); IDT(163, INTR, 1)
    IDT(164, INTR, 1); IDT(165, INTR, 1); IDT(166, INTR, 1); IDT(167, INTR, 1)
    IDT(168, INTR, 1); IDT(169, INTR, 1); IDT(170, INTR, 1); IDT(171, INTR, 1)
    IDT(172, INTR, 1); IDT(173, INTR, 1); IDT(174, INTR, 1); IDT(175, INTR, 1)
    IDT(176, INTR, 1); IDT(177, INTR, 1); IDT(178, INTR, 1); IDT(179, INTR, 1)
    IDT(180, INTR, 1); IDT(181, INTR, 1); IDT(182, INTR, 1); IDT(183, INTR, 1)
    IDT(184, INTR, 1); IDT(185, INTR, 1); IDT(186, INTR, 1); IDT(187, INTR, 1)
    IDT(188, INTR, 1); IDT(189, INTR, 1); IDT(190, INTR, 1); IDT(191, INTR, 1)
    IDT(192, INTR, 1); IDT(193, INTR, 1); IDT(194, INTR, 1); IDT(195, INTR, 1)
    IDT(196, INTR, 1); IDT(197, INTR, 1); IDT(198, INTR, 1); IDT(199, INTR, 1)
    IDT(200, INTR, 1); IDT(201, INTR, 1); IDT(202, INTR, 1); IDT(203, INTR, 1)
    IDT(204, INTR, 1); IDT(205, INTR, 1); IDT(206, INTR, 1); IDT(207, INTR, 1)
    IDT(208, INTR, 1); IDT(209, INTR, 1); IDT(210, INTR, 1); IDT(211, INTR, 1)
    IDT(212, INTR, 1); IDT(213, INTR, 1); IDT(214, INTR, 1); IDT(215, INTR, 1)
    IDT(216, INTR, 1); IDT(217, INTR, 1); IDT(218, INTR, 1); IDT(219, INTR, 1)
    IDT(220, INTR, 1); IDT(221, INTR, 1); IDT(222, INTR, 1); IDT(223, INTR, 1)
    IDT(224, INTR, 1); IDT(225, INTR, 1); IDT(226, INTR, 1); IDT(227, INTR, 1)
    IDT(228, INTR, 1); IDT(229, INTR, 1); IDT(230, INTR, 1); IDT(231, INTR, 1)
    IDT(232, INTR, 1); IDT(233, INTR, 1); IDT(234, INTR, 1); IDT(235, INTR, 1)
    IDT(236, INTR, 1); IDT(237, INTR, 1); IDT(238, INTR, 1); IDT(239, INTR, 1)
    IDT(240, INTR, 1); IDT(241, INTR, 1); IDT(242, INTR, 1); IDT(243, INTR, 1)
    IDT(244, INTR, 1); IDT(245, INTR, 1); IDT(246, INTR, 1); IDT(247, INTR, 1)
    IDT(248, INTR, 1); IDT(249, INTR, 1); IDT(250, INTR, 1); IDT(251, INTR, 1)
    IDT(252, INTR, 1); IDT(253, INTR, 1); IDT(254, INTR, 1); IDT(255, INTR, 1)

idt48:
	.word (idt48 - idt - 1)
	.long idt

/*
 * Page tables. Long mode requires them, but we don't implement any memory
 * protection yet, so these simply identity-map the first 4GB w/ 1GB pages.
 */

.align 4096

.globl z_x86_flat_ptables
z_x86_flat_ptables:
	.long pdp + 0x03		/* 0x03 = R/W, P */
	.long 0
	.fill 4088, 1, 0

pdp:	.long 0x00000083		/* 0x83 = 1GB, R/W, P */
	.long 0
	.long 0x40000083
	.long 0
	.long 0x80000083
	.long 0
	.long 0xC0000083
	.long 0
	.fill 4064, 1, 0

.section .lodata,"ad"

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
	.word 0, 0, 0x9200, 0x0000	/* 0x20: 64-bit kernel data */

	.word 0, 0, 0, 0		/* 0x28: unused */

	/*
	 * CPU 0 task state segment descriptors
	 */

	.word 0				/* 0x30: 64-bit TSS data (for GS) */
	.word tss0
	.word 0x9200
	.word 0

	.word 0, 0, 0, 0		/* 0x38: unused */

	.word __X86_TSS64_SIZEOF-1	/* 0x40: 64-bit TSS (16-byte entry) */
	.word tss0
	.word 0x8900
	.word 0, 0, 0, 0, 0

#if CONFIG_MP_NUM_CPUS > 1
	/*
	 * CPU 1 task state segment descriptors
	 */

	.word 0				/* 0x50: 64-bit TSS data (for GS) */
	.word tss1
	.word 0x9200
	.word 0

	.word 0, 0, 0, 0		/* 0x58: unused */

	.word __X86_TSS64_SIZEOF-1	/* 0x60: 64-bit TSS (16-byte entry) */
	.word tss1
	.word 0x8900
	.word 0, 0, 0, 0, 0
#endif

#if CONFIG_MP_NUM_CPUS > 2
	/*
	 * CPU 2 task state segment descriptors
	 */

	.word 0				/* 0x70: 64-bit TSS data (for GS) */
	.word tss2
	.word 0x9200
	.word 0

	.word 0, 0, 0, 0		/* 0x78: unused */

	.word __X86_TSS64_SIZEOF-1	/* 0x80: 64-bit TSS (16-byte entry) */
	.word tss2
	.word 0x8900
	.word 0, 0, 0, 0, 0
#endif

#if CONFIG_MP_NUM_CPUS > 3
	/*
	 * CPU 3 task state segment descriptors
	 */

	.word 0				/* 0x90: 64-bit TSS data (for GS) */
	.word tss3
	.word 0x9200
	.word 0

	.word 0, 0, 0, 0		/* 0x98: unused */

	.word __X86_TSS64_SIZEOF-1	/* 0xA0: 64-bit TSS (16-byte entry) */
	.word tss3
	.word 0x8900
	.word 0, 0, 0, 0, 0
#endif

gdt48:
	.word (gdt48 - gdt - 1)
	.long gdt

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
