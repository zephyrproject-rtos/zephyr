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
#include <zephyr/arch/exception.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_RISCV_S_MODE
/*
 * In S-mode, ecall (cause=9) is kept in M-mode for SBI so it never reaches
 * the S-mode exception handler.  Kernel-context panics use ebreak (cause=3,
 * breakpoint) which IS delegated to S-mode.  isr.S treats ebreak with
 * t0==RV_ECALL_RUNTIME_EXCEPT (0) as ARCH_EXCEPT and reads the reason from
 * saved a0 — mirroring exactly how M-mode handles ecall with RV_ECALL_RUNTIME_EXCEPT.
 */
#define ARCH_EXCEPT(reason_p)	do {					\
		if (k_is_user_context()) {				\
			arch_syscall_invoke1(reason_p,			\
				K_SYSCALL_USER_FAULT);			\
		} else {						\
			register unsigned long _r __asm__("a0") =	\
				(unsigned long)(reason_p);		\
			register unsigned long _t __asm__("t0") = 0;	\
			__asm__ volatile("ebreak"			\
					 : : "r"(_r), "r"(_t)		\
					 : "memory");			\
		}							\
		CODE_UNREACHABLE; /* LCOV_EXCL_LINE */			\
	} while (false)

#elif defined(CONFIG_USERSPACE)

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

#include <zephyr/syscalls/error.h>

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_ERROR_H_ */
