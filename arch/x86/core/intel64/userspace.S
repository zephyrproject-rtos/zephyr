/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/arch/cpu.h>
#include <offsets_short.h>
#include <zephyr/syscall.h>
#include <zephyr/sys/mem_manage.h>

#ifdef CONFIG_X86_KPTI
/* Copy interrupt return stack context to the trampoline stack, switch back
 * to the user page table, and only then 'iret'. We jump to this instead
 * of calling 'iret' if KPTI is turned on. This must be invoked with interrupts
 * locked.
 *
 * Stack layout is expected to be what 'iretq' expects, which is as follows:
 *
 * 32 SS
 * 24 RSP
 * 16 RFLAGS
 * 8  CS
 * 0  RIP
 */
.global z_x86_trampoline_to_user
z_x86_trampoline_to_user:
	/* Stash EDI, need a free register */
	pushq	%rdi

	/* Store old stack pointer and switch to trampoline stack */
	movq	%rsp, %rdi
	movq	%gs:__x86_tss64_t_ist2_OFFSET, %rsp

	/* Copy context */
	pushq	40(%rdi)	/* SS */
	pushq	32(%rdi)	/* RSP */
	pushq	24(%rdi)	/* RFLAGS */
	pushq	16(%rdi)	/* CS */
	pushq   8(%rdi)		/* RIP */
	xchgq	%rdi, (%rdi)	/* Exchange old rdi to restore it and put
				   trampoline stack address in its old storage
				   area */

	/* Switch to thread's page table */
	pushq	%rax
	movq	%gs:__x86_tss64_t_cpu_OFFSET, %rax
	movq	___cpu_t_current_OFFSET(%rax), %rax
	movq	_thread_offset_to_ptables(%rax), %rax
	movq	%rax, %cr3
	popq	%rax
	movq	$0, -8(%rsp)	/* Delete stashed RAX data */

	/* Trampoline stack should have nothing sensitive in it at this point */
	swapgs
	iretq
#endif /* CONFIG_X86_KPTI */


/* Landing site for 'syscall' instruction
 *
 * Call id is in RAX
 * Arguments are in RDI, RSI, RDX, R10, R8, R9
 * Return address stored by CPU in RCX
 * User RFLAGS store by CPU in R11
 * Current RFLAGS has been masked with ~X86_FMASK_MSR
 */
.global z_x86_syscall_entry_stub
z_x86_syscall_entry_stub:
	swapgs

	/* Save original stack pointer from user mode in memory, at the
	 * moment we have no free registers or stack to save it to. This
	 * eventually gets put on the stack before we re-enable interrupts
	 * as this is a per-cpu and not per-thread area.
	 */
	movq	%rsp, %gs:__x86_tss64_t_usp_OFFSET

#ifdef CONFIG_X86_KPTI
	/* We need to switch to the trampoline stack so that we can
	 * switch to the kernel's page table
	 */
	movq	%gs:__x86_tss64_t_ist2_OFFSET, %rsp

	/* Load kernel's page table */
	pushq	%rax

	/* NOTE: Presumes phys=virt */
	movq	$Z_MEM_PHYS_ADDR(z_x86_kernel_ptables), %rax
	movq	%rax, %cr3
	popq	%rax
	movq	$0, -8(%rsp)	/* Delete stashed RAX data */
#endif /* CONFIG_X86_KPTI */

	/* Switch to the privilege mode stack pointer stored in
	 * x86_tss64.psp
	 */
	movq	%gs:__x86_tss64_t_psp_OFFSET, %rsp

	/* We're now on the privilege mode stack; push the old user stack
	 * pointer onto it
	 */
	pushq	%gs:__x86_tss64_t_usp_OFFSET
#ifdef CONFIG_X86_KPTI
	movq	$0, %gs:__x86_tss64_t_usp_OFFSET
#endif

	sti			/* re-enable interrupts */

	/* call_id is in RAX. bounds-check it, must be less than
	 * K_SYSCALL_LIMIT.
	 */
	cmp	$K_SYSCALL_LIMIT, %rax
	jae	_bad_syscall

_id_ok:
#ifdef CONFIG_X86_BOUNDS_CHECK_BYPASS_MITIGATION
	/* Prevent speculation with bogus system call IDs */
	lfence
#endif

	/* Remaining registers not involved in the syscall operation are
	 * RBX, RBP, R12-R15, plus floating point / SIMD registers.
	 *
	 * We save caller-saved registers so we can restore to original values
	 * when we call 'sysretq' at the end.
	 */
	pushq	%rdi
	subq	$X86_FXSAVE_SIZE, %rsp
	fxsave	(%rsp)
	pushq	%rsi
	pushq	%rdx
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11	/* RFLAGS */
	pushq	%rcx	/* Return address stored by 'syscall' */
	pushq	%rsp	/* SSF parameter */

	/* All other args are in the right registers, except arg4 which
	 * we had to put in r10 instead of RCX
	 */
	movq	%r10, %rcx

	/* from the call ID in RAX, load R10 with the actual function pointer
	 * to call by looking it up in the system call dispatch table
	 */
	xorq	%r11, %r11
	movq	_k_syscall_table(%r11, %rax, 8), %r10

	/* Run the marshal function, which is some entry in _k_syscall_table */
	call	*%r10

	/* RAX now contains the return value
	 *
	 * Callee-saved registers are un-touched from original values per C
	 * calling convention, but sensitive data may lurk in caller-saved regs
	 * RDI, RSI, RDX, R8, R9, R10, XMM* after we have serviced the system
	 * call.  We saved them earlier, restore their original values when
	 * the syscall was made. This also preserves these registers if they
	 * were not used as arguments.
	 *
	 * We also can't have RCX and R11 clobbered as we need the original
	 * values to successfully 'sysretq'.
	 */
	addq	$8, %rsp	/* Discard ssf */
	popq	%rcx	/* Restore return address for 'sysretq' */
	popq	%r11	/* Restore RFLAGS for 'sysretq' */
	popq	%r10
	popq	%r9
	popq	%r8
	popq	%rdx
	popq	%rsi
	fxrstor	(%rsp)
	addq	$X86_FXSAVE_SIZE, %rsp
	popq	%rdi

#ifdef CONFIG_X86_KPTI
	/* Lock IRQs as we are using per-cpu memory areas and the
	 * trampoline stack
	 */
	cli

	/* Stash user stack pointer and switch to trampoline stack */
	popq	%gs:__x86_tss64_t_usp_OFFSET
	movq	%gs:__x86_tss64_t_ist2_OFFSET, %rsp

	/* Switch to thread's page table */
	pushq	%rax
	movq	%gs:__x86_tss64_t_cpu_OFFSET, %rax
	movq	___cpu_t_current_OFFSET(%rax), %rax
	movq	_thread_offset_to_ptables(%rax), %rax
	movq	%rax, %cr3
	popq	%rax
	movq	$0, -8(%rsp)	/* Delete stashed RAX data */

	/* Restore saved user stack pointer */
	movq	%gs:__x86_tss64_t_usp_OFFSET, %rsp
	movq	$0, %gs:__x86_tss64_t_usp_OFFSET
#else
	/* Restore user stack pointer */
	popq	%rsp

	/* Return to user mode, locking interrupts as the normal interrupt
	 * handling path will get very confused if it occurs between
	 * 'swapgs' and 'sysretq'
	 */
	cli
#endif /* CONFIG_X86_KPTI */

	swapgs
	sysretq

_bad_syscall:
	/* RAX had a bogus syscall value in it, replace with the bad syscall
	 * handler's ID, and put the bad ID as its first argument.
	 *
	 * TODO: On this and all other arches, simply immediately return
	 * with -ENOSYS, once all syscalls have a return value
	 */
	movq	%rax, %rdi
	movq	$K_SYSCALL_BAD, %rax
	jmp	_id_ok

/*
 * size_t arch_user_string_nlen(const char *s, size_t maxsize, int *err_arg)
 *                              ^ RDI          ^ RSI           ^ RDX
 */
.global arch_user_string_nlen
arch_user_string_nlen:
	/* Initial error value, strlen_done adjusts this if we succeed */
	movl	$-1, %r8d

	/* use RAX as our length count (this function's return value) */
	xor	%rax, %rax

	/* This code might page fault */
strlen_loop:
.global z_x86_user_string_nlen_fault_start
z_x86_user_string_nlen_fault_start:
	cmpb	$0x0, (%rdi, %rax, 1)	/* *(RDI + RAX) == 0? Could fault. */

.global z_x86_user_string_nlen_fault_end
z_x86_user_string_nlen_fault_end:
	je	strlen_done
	cmp	%rsi, %rax		/* Max length reached? */
	je	strlen_done
	inc	%rax			/* EAX++ and loop again */
	jmp	strlen_loop

strlen_done:
	/* Set error value to 0 since we succeeded */
	xorl	%r8d, %r8d

.global z_x86_user_string_nlen_fixup
z_x86_user_string_nlen_fixup:
	/* Write error value to 32-bit integer err pointer parameter */
	movl	%r8d, (%rdx)
	retq

/*
 * Trampoline function to put the p3 parameter in the register expected
 * by the calling convention, we couldn't use RCX when we called 'sysret'
 */
z_x86_userspace_landing_site:
	/* Place argument 4 in the correct position */
	movq	%r10, %rcx
	call	z_thread_entry

/* FUNC_NORETURN void z_x86_userspace_enter(
 *		k_thread_entry_t user_entry,	<- RDI
 *		void *p1, void *p2, void *p3,	<- RSI, RDX, RCX
 *		uintptr_t stack_end,		<- R8
 *		uintptr_t stack_start)		<- R9
 *
 * A one-way trip to userspace.
 */
.global z_x86_userspace_enter
z_x86_userspace_enter:
	/* RCX is sysret return address, pass along p3 in r10,
	 * z_x86_userspace_landing_site will fix this up
	 */
	movq	%rcx, %r10

	/* switch to privilege mode stack so we can erase thread stack buffer,
	 * the buffer is the page immediately before the thread stack
	 */
	movq	%r9, %rsp

	/* Push callee-saved regs and go back into C code to erase the stack
	 * buffer and set US bit in page tables for it
	 */
	pushq	%rdx
	pushq	%rsi
	pushq	%rdi
	pushq	%r8
	pushq	%r10
	callq	z_x86_current_stack_perms
	popq	%r10
	popq	%r8
	popq	%rdi
	popq	%rsi
	popq	%rdx

	/* Reset to the beginning of the user stack */
	movq	%r8, %rsp

	/* set sysret entry point */
	movq	$z_x86_userspace_landing_site, %rcx

	/* Copy RFLAGS into r11, required by sysret */
	pushfq
	movq	(%rsp), %r11
	movq	$0, (%rsp)	/* Now a debugger-friendly return address */

	/* cleanse other registers */
	xorq	%rbx, %rbx
	xorq	%rbp, %rbp
	xorq	%r12, %r12
	xorq	%r13, %r13
	xorq	%r14, %r14
	xorq	%r15, %r15

	cli

#ifdef CONFIG_X86_KPTI
	/* Switch to thread's page table. We have free registers so no need
	 * to involve the trampoline stack.
	 */
	movq	%gs:__x86_tss64_t_cpu_OFFSET, %rax
	movq	___cpu_t_current_OFFSET(%rax), %rax
	movq	_thread_offset_to_ptables(%rax), %rax
	movq	%rax, %cr3
#endif
	swapgs
	sysretq
