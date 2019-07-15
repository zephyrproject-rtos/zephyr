/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <kernel_structs.h>
#include <kernel_internal.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(os);

void z_x86_fatal_error(unsigned int reason, const z_arch_esf_t *esf)
{
	if (esf != NULL) {
		LOG_ERR("RIP=%016lx RSP=%016lx RFLAGS=%016lx\n",
			esf->rip, esf->rsp, esf->rflags);

		LOG_ERR("RAX=%016lx RBX=%016lx RCX=%016lx RDX=%016lx\n",
			esf->rax, esf->rbx, esf->rcx, esf->rdx);

		LOG_ERR("RSI=%016lx RDI=%016lx RBP=%016lx RSP=%016lx\n",
			esf->rsi, esf->rdi, esf->rbp, esf->rsp);

		LOG_ERR("R8=%016lx R9=%016lx R10=%016lx R11=%016lx\n",
			esf->r8, esf->r9, esf->r10, esf->r11);

		LOG_ERR("R12=%016lx R13=%016lx R14=%016lx R15=%016lx\n",
			esf->r12, esf->r13, esf->r14, esf->r15);
	}

	z_fatal_error(reason, esf);
}

void z_x86_exception(const z_arch_esf_t *esf)
{
	LOG_ERR("** CPU Exception %ld (code %ld/0x%lx) **\n",
		esf->vector, esf->code, esf->code);

	z_x86_fatal_error(K_ERR_CPU_EXCEPTION, esf);
}
