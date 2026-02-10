/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

extern void z_sig_trampoline(void);

void arch_sig_pre(struct k_thread *thread, void *arg)
{
	struct arch_esf *const esf = arg;
	struct arch_sig_ctx *const backup = &thread->base.sig.user;

	/* Backup the original FP, SP, ELR (return address), and X0 (syscall return value) from PSF */
#ifdef CONFIG_FRAME_POINTER
	backup->fp = esf->fp;
#endif
#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
	backup->sp = esf->sp;
#endif
	backup->lr = esf->lr;
	backup->elr = esf->elr;
	backup->x0 = esf->x0;

	/* Overwrite PC (return address) to resume execution in arch_sig_trampoline() */
	esf->elr = (unsigned long)arch_sig_trampoline;
}

void arch_sig_post(struct k_thread *thread, void *arg)
{
	struct arch_esf *const esf = arg;
	struct arch_sig_ctx *const backup = &thread->base.sig.user;

	/* Backup the original FP, SP, ELR (return address), and X0 (syscall return value) from PSF */
#ifdef CONFIG_FRAME_POINTER
	esf->fp = backup->fp;
#endif
#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
	esf->sp = backup->sp;
#endif
	esf->lr = backup->lr;
	esf->elr = backup->elr;
	esf->x0 = backup->x0;
}
