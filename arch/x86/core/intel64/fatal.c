/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <kernel_structs.h>
#include <kernel_internal.h>

void z_x86_fatal_error(unsigned int reason, const z_arch_esf_t *esf)
{
	if (esf != NULL) {
		z_fatal_print("RIP = %lx RSP = %lx RFLAGS = %lx\n",
			esf->rip, esf->rsp, esf->rflags);

		z_fatal_print("RAX = %lx, RBX = %lx, RCX = %lx, RDX = %lx\n",
			esf->rax, esf->rbx, esf->rcx, esf->rdx);

		z_fatal_print("RSI = %lx, RDI = %lx, RBP = %lx, RSP = %lx\n",
			esf->rsi, esf->rdi, esf->rbp, esf->rsp);

		z_fatal_print("R8 = %lx, R9 = %lx, R10 = %lx, R11 = %lx\n",
			esf->r8, esf->r9, esf->r10, esf->r11);

		z_fatal_print("R12 = %lx, R13 = %lx, R14 = %lx, R15 = %lx\n",
			esf->r12, esf->r13, esf->r14, esf->r15);
	}

	z_fatal_error(reason, esf);
}

void z_x86_exception(const z_arch_esf_t *esf)
{
	z_fatal_print("** CPU Exception %ld (code %ld/%lx) **\n",
		esf->vector, esf->code, esf->code);

	z_x86_fatal_error(K_ERR_CPU_EXCEPTION, esf);
}
