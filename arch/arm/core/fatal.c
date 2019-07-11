/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel fatal error handler for ARM Cortex-M
 *
 * This module provides the z_NanoFatalErrorHandler() routine for ARM Cortex-M.
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <inttypes.h>

#include <kernel.h>
#include <kernel_structs.h>
#include <sys/printk.h>
#include <logging/log_ctrl.h>

void z_arm_fatal_error(unsigned int reason, const NANO_ESF *esf)
{
	printk("Faulting instruction address = 0x%x\n",
	       esf->basic.pc);
	z_fatal_error(reason, esf);
}

void z_do_kernel_oops(const NANO_ESF *esf)
{
	z_arm_fatal_error(esf->basic.r0, esf);
}

FUNC_NORETURN void z_arch_syscall_oops(void *ssf_ptr)
{
	u32_t *ssf_contents = ssf_ptr;
	NANO_ESF oops_esf = { 0 };

	oops_esf.basic.pc = ssf_contents[3];

	z_do_kernel_oops(&oops_esf);
	CODE_UNREACHABLE;
}
