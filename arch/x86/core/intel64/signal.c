/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

void arch_sig_pre(struct k_thread *thread, void *arg)
{
	uintptr_t user_sp;
	struct x86_ssf *const sf = arg;
	struct arch_sig_ctx *const backup = &thread->base.sig.user;

	/* Backup the original RAX, RFLAGS, RSP, and RIP */
	backup->rax = sf->rdi;
	backup->rflags = sf->rflags;
	backup->rsp = sf->rsp;
	backup->rip = sf->rip;

	/* Additional stack frame to facilitate return to original syscall epilogue */
	/* entered like a called function: RSP + 8 is 16-byte aligned */
	user_sp = ROUND_DOWN((uintptr_t)sf->rsp, 16) - sizeof(uintptr_t);
	/* write original RIP at new top of user stack */
	*(uintptr_t *)user_sp = (uintptr_t)backup->rip;
	sf->rsp = (unsigned long)user_sp;

	/* Overwrite RIP (return address) to resume execution in arch_sig_trampoline() */
	sf->rip = (unsigned long)arch_sig_trampoline;
}

void arch_sig_post(struct k_thread *thread, void *arg)
{
	struct x86_ssf *const sf = arg;
	struct arch_sig_ctx *const backup = &thread->base.sig.user;

	/* Restore the original RAX, RFLAGS, RIP */
	sf->rdi = backup->rax;
	sf->rflags = backup->rflags;
	sf->rsp = backup->rsp;
	sf->rip = backup->rip;
}
