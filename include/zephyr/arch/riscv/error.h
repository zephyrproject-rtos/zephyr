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

#include <zephyr/arch/riscv/syscall.h>
#include <zephyr/arch/riscv/exception.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_USERSPACE

#define ARCH_EXCEPT(reason_p)	do {			\
		if (k_is_user_context()) {		\
			arch_syscall_invoke1(reason_p,	\
				K_SYSCALL_USER_FAULT);	\
		} else {				\
			compiler_barrier();		\
			arch_syscall_invoke1(reason_p,	\
				RV_ECALL_RUNTIME_EXCEPT);\
		}					\
		CODE_UNREACHABLE; /* LCOV_EXCL_LINE */	\
	} while (false)
#else
#define ARCH_EXCEPT(reason_p) \
	arch_syscall_invoke1(reason_p, RV_ECALL_RUNTIME_EXCEPT)
#endif

__syscall void user_fault(unsigned int reason);

#include <syscalls/error.h>

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_ERROR_H_ */
