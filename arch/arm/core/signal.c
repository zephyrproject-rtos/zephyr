/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

void arch_sig_pre(struct k_thread *thread, void *arg)
{
	struct arch_sig_ctx *const psf = arg;
	struct arch_sig_ctx *const backup = &thread->base.sig.user;

	/*
	 * Backup the original SP, PC (return address), LR, and R0 from PSF. The PSF is a
	 * four-word stack frame: it does not carry the callee-saved registers, which are
	 * captured separately in assembly at redirection.
	 */
	backup->sp = psf->sp;
	backup->lr = psf->lr;
	backup->r0 = psf->r0;
	backup->pc = psf->pc;

	/* Overwrite PC (return address) to resume execution in arch_sig_trampoline() */
	psf->pc = (unsigned long)arch_sig_trampoline;
}

void arch_sig_post(struct k_thread *thread, void *arg)
{
	struct arch_sig_ctx *const psf = arg;
	struct arch_sig_ctx *const backup = &thread->base.sig.user;

	/* Restore the original SP, PC (return address), LR, and R0 into PSF */
	psf->sp = backup->sp;
	psf->lr = backup->lr;
	psf->r0 = backup->r0;
	psf->pc = backup->pc;
}
