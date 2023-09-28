/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel fatal error handler for ARM Cortex-M and Cortex-R
 *
 * This module provides the z_arm_fatal_error() routine for ARM Cortex-M
 * and Cortex-R CPUs.
 */

#include <zephyr/kernel.h>
#include <kernel_arch_data.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

static void esf_dump(const z_arch_esf_t *esf)
{
	LOG_ERR("r0/a1:  0x%08x  r1/a2:  0x%08x  r2/a3:  0x%08x",
		esf->basic.a1, esf->basic.a2, esf->basic.a3);
	LOG_ERR("r3/a4:  0x%08x r12/ip:  0x%08x r14/lr:  0x%08x",
		esf->basic.a4, esf->basic.ip, esf->basic.lr);
	LOG_ERR(" xpsr:  0x%08x", esf->basic.xpsr);
#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	for (int i = 0; i < ARRAY_SIZE(esf->fpu.s); i += 4) {
		LOG_ERR("s[%2d]:  0x%08x  s[%2d]:  0x%08x"
			"  s[%2d]:  0x%08x  s[%2d]:  0x%08x",
			i, (uint32_t)esf->fpu.s[i],
			i + 1, (uint32_t)esf->fpu.s[i + 1],
			i + 2, (uint32_t)esf->fpu.s[i + 2],
			i + 3, (uint32_t)esf->fpu.s[i + 3]);
	}
#ifdef CONFIG_VFP_FEATURE_REGS_S64_D32
	for (int i = 0; i < ARRAY_SIZE(esf->fpu.d); i += 4) {
		LOG_ERR("d[%2d]:  0x%16llx  d[%2d]:  0x%16llx"
			"  d[%2d]:  0x%16llx  d[%2d]:  0x%16llx",
			i, (uint64_t)esf->fpu.d[i],
			i + 1, (uint64_t)esf->fpu.d[i + 1],
			i + 2, (uint64_t)esf->fpu.d[i + 2],
			i + 3, (uint64_t)esf->fpu.d[i + 3]);
	}
#endif
	LOG_ERR("fpscr:  0x%08x", esf->fpu.fpscr);
#endif
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	const struct _callee_saved *callee = esf->extra_info.callee;

	if (callee != NULL) {
		LOG_ERR("r4/v1:  0x%08x  r5/v2:  0x%08x  r6/v3:  0x%08x",
			callee->v1, callee->v2, callee->v3);
		LOG_ERR("r7/v4:  0x%08x  r8/v5:  0x%08x  r9/v6:  0x%08x",
			callee->v4, callee->v5, callee->v6);
		LOG_ERR("r10/v7: 0x%08x  r11/v8: 0x%08x    psp:  0x%08x",
			callee->v7, callee->v8, callee->psp);
	}

	LOG_ERR("EXC_RETURN: 0x%0x", esf->extra_info.exc_return);

#endif /* CONFIG_EXTRA_EXCEPTION_INFO */
	LOG_ERR("Faulting instruction address (r15/pc): 0x%08x",
		esf->basic.pc);
}

void z_arm_fatal_error(unsigned int reason, const z_arch_esf_t *esf)
{

	if (esf != NULL) {
		esf_dump(esf);
	}
	z_fatal_error(reason, esf);
}

/**
 * @brief Handle a software-generated fatal exception
 * (e.g. kernel oops, panic, etc.).
 *
 * Notes:
 * - the function is invoked in SVC Handler
 * - if triggered from nPRIV mode, only oops and stack fail error reasons
 *   may be propagated to the fault handling process.
 * - We expect the supplied exception stack frame to always be a valid
 *   frame. That is because, if the ESF cannot be stacked during an SVC,
 *   a processor fault (e.g. stacking error) will be generated, and the
 *   fault handler will executed instead of the SVC.
 *
 * @param esf exception frame
 * @param callee_regs Callee-saved registers (R4-R11)
 */
void z_do_kernel_oops(const z_arch_esf_t *esf, _callee_saved_t *callee_regs)
{
#if !(defined(CONFIG_EXTRA_EXCEPTION_INFO) && defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE))
	ARG_UNUSED(callee_regs);
#endif
	/* Stacked R0 holds the exception reason. */
	unsigned int reason = esf->basic.r0;

#if defined(CONFIG_USERSPACE)
	if (z_arm_preempted_thread_in_user_mode(esf)) {
		/*
		 * Exception triggered from user mode.
		 *
		 * User mode is only allowed to induce oopses and stack check
		 * failures via software-triggered system fatal exceptions.
		 */
		if (!((esf->basic.r0 == K_ERR_KERNEL_OOPS) ||
			(esf->basic.r0 == K_ERR_STACK_CHK_FAIL))) {

			reason = K_ERR_KERNEL_OOPS;
		}
	}

#endif /* CONFIG_USERSPACE */

#if !defined(CONFIG_EXTRA_EXCEPTION_INFO)
	z_arm_fatal_error(reason, esf);
#else
	z_arch_esf_t esf_copy;

	memcpy(&esf_copy, esf, offsetof(z_arch_esf_t, extra_info));
#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	/* extra exception info is collected in callee_reg param
	 * on CONFIG_ARMV7_M_ARMV8_M_MAINLINE
	 */

	esf_copy.extra_info = (struct __extra_esf_info) {
		.callee = callee_regs,
	};
#else
	/* extra exception info is not collected for kernel oops
	 * path today so we make a copy of the ESF and zero out
	 * that information
	 */
	esf_copy.extra_info = (struct __extra_esf_info) { 0 };
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE */

	z_arm_fatal_error(reason, &esf_copy);
#endif /* CONFIG_EXTRA_EXCEPTION_INFO */
}

FUNC_NORETURN void arch_syscall_oops(void *ssf_ptr)
{
	uint32_t *ssf_contents = ssf_ptr;
	z_arch_esf_t oops_esf = { 0 };

	/* TODO: Copy the rest of the register set out of ssf_ptr */
	oops_esf.basic.pc = ssf_contents[3];

	z_arm_fatal_error(K_ERR_KERNEL_OOPS, &oops_esf);
	CODE_UNREACHABLE;
}
