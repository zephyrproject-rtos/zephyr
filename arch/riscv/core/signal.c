/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

void arch_sig_pre(struct k_thread *thread, void *arg)
{
	struct arch_esf *const esf = arg;
	struct arch_sig_ctx *const sig = &thread->base.sig.user;

	/* Backup the original RA, SP, A0, S0, and MEPC into storage */
	sig->ra = esf->ra;
	sig->sp = esf->sp;
	sig->a0 = esf->a0;
	sig->s0 = esf->s0;
	sig->mepc = esf->mepc;

	/* Overwrite MEPC to resume execution in arch_sig_trampoline() */
	esf->mepc = (unsigned long)arch_sig_trampoline;
}

void arch_sig_post(struct k_thread *thread, void *arg)
{
	struct arch_esf *const esf = arg;
	struct arch_sig_ctx *const sig = &thread->base.sig.user;

	/* Restore the original RA, SP, A0, S0, and MEPC into esf */
	esf->ra = sig->ra;
	esf->sp = sig->sp;
	esf->a0 = sig->a0;
	esf->s0 = sig->s0;
	esf->mepc = sig->mepc;
}
