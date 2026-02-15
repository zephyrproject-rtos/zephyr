/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>

void arch_sig_pre(struct k_thread *thread, void *arg)
{
	uintptr_t user_sp;
	struct _x86_syscall_stack_frame *const sf = arg;
	struct arch_sig_ctx *const backup = &thread->base.sig.user;

	/* Backup the original EAX, EFLAGS, ESP, and EIP so we can restore after signal */
	backup->eax = thread->arch.sig_eax;
	backup->eflags = sf->eflags;
	backup->esp = sf->esp;
	backup->eip = sf->eip;

	/* Additional stack frame to facilitate return to original syscall epilogue */
	/* entered like a called function: ESP + 4 is 16-byte aligned */
	user_sp = ROUND_DOWN((uintptr_t)sf->esp, 16) - sizeof(uintptr_t);
	/* write original EIP at new top of user stack */
	K_OOPS(K_SYSCALL_MEMORY_WRITE(user_sp, sizeof(uintptr_t)));
	*(uintptr_t *)user_sp = (uintptr_t)backup->eip;
	sf->esp = (unsigned long)user_sp;

	/* Overwrite EIP (return address) to resume execution in arch_sig_trampoline() */
	sf->eip = (unsigned long)arch_sig_trampoline;
}

void arch_sig_post(struct k_thread *thread, void *arg)
{
	struct _x86_syscall_stack_frame *const sf = arg;
	struct arch_sig_ctx *const backup = &thread->base.sig.user;

	/* Restore the original EAX, EFLAGS, ESP, and EIP */
	thread->arch.sig_eax = backup->eax;
	sf->eflags = backup->eflags;
	sf->esp = backup->esp;
	sf->eip = backup->eip;
}
