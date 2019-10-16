/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fatal fault handling
 *
 * This module implements the routines necessary for handling fatal faults on
 * ARCv2 CPUs.
 */

#include <kernel_structs.h>
#include <offsets_short.h>
#include <toolchain.h>
#include <arch/cpu.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(os);

void z_arc_fatal_error(unsigned int reason, const z_arch_esf_t *esf)
{
	if (reason == K_ERR_CPU_EXCEPTION) {
		LOG_ERR("Faulting instruction address = 0x%lx",
			z_arc_v2_aux_reg_read(_ARC_V2_ERET));
	}

	z_fatal_error(reason, esf);
}

FUNC_NORETURN void z_arch_syscall_oops(void *ssf_ptr)
{
	z_arc_fatal_error(K_ERR_KERNEL_OOPS, ssf_ptr);
	CODE_UNREACHABLE;
}

FUNC_NORETURN void z_arch_system_halt(unsigned int reason)
{
	ARG_UNUSED(reason);

	__asm__("brk");

	CODE_UNREACHABLE;
}
