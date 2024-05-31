/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/* NMI handlers should override weak implementation
 * return true if NMI is handled, false otherwise
 */
__weak bool z_x86_do_kernel_nmi(const z_arch_esf_t *esf)
{
	ARG_UNUSED(esf);

	return false;
}

void z_x86_exception(z_arch_esf_t *esf)
{
	switch (esf->vector) {
	case Z_X86_OOPS_VECTOR:
		z_x86_do_kernel_oops(esf);
		break;
	case IV_PAGE_FAULT:
		z_x86_page_fault_handler(esf);
		break;
	case IV_NON_MASKABLE_INTERRUPT:
		if (!z_x86_do_kernel_nmi(esf)) {
			z_x86_unhandled_cpu_exception(esf->vector, esf);
			CODE_UNREACHABLE;
		}
		break;
	default:
		z_x86_unhandled_cpu_exception(esf->vector, esf);
		CODE_UNREACHABLE;
	}
}

#ifdef CONFIG_USERSPACE
void arch_syscall_oops(void *ssf_ptr)
{
	struct x86_ssf *ssf = ssf_ptr;

	LOG_ERR("Bad system call from RIP 0x%lx", ssf->rip);

	z_x86_fatal_error(K_ERR_KERNEL_OOPS, NULL);
}
#endif /* CONFIG_USERSPACE */
