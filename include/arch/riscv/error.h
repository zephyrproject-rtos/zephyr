/*
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISCV public error handling
 *
 * RISCV-specific kernel error handling interface. Included by riscv/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_ERROR_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_ERROR_H_

#include <arch/riscv/syscall.h>
#include <arch/riscv/exp.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_USERSPACE

/*
 * Kernel features like canary (software stack guard) are built
 * with an argument to bypass the test before syscall (test if CPU
 * is running in user or kernel) and directly execute the function.
 * Then if this kind of code wishes to trigger a CPU exception,
 * the implemented syscall is useless because  the function is directly
 * called even if the CPU is running in user (which happens during
 * sanity check). To fix that, I bypass the generated test code by writing
 * the test myself to remove the bypass ability.
 */

#define ARCH_EXCEPT(reason_p)	do {			\
		if (_is_user_context()) {		\
			arch_syscall_invoke1(reason_p,	\
				K_SYSCALL_USER_FAULT);	\
		} else {				\
			compiler_barrier();		\
			z_impl_user_fault(reason_p);	\
		}					\
		CODE_UNREACHABLE; /* LCOV_EXCL_LINE */	\
	} while (false)
#else
#define ARCH_EXCEPT(reason_p)	do {			\
		z_impl_user_fault(reason_p);		\
	} while (false)
#endif

__syscall void user_fault(unsigned int reason);

#include <syscalls/error.h>

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_ERROR_H_ */
