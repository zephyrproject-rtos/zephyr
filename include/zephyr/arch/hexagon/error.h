/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Hexagon fatal error handling
 */

#ifndef ZEPHYR_INCLUDE_ARCH_HEXAGON_ERROR_H_
#define ZEPHYR_INCLUDE_ARCH_HEXAGON_ERROR_H_

#include <zephyr/arch/hexagon/syscall.h>
#include <zephyr/arch/exception.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern FUNC_NORETURN void z_hexagon_fatal_error(unsigned int reason);

/**
 * @brief Generate a software-induced fatal error.
 *
 * @param reason_p K_ERR_ scoped reason code for the fatal error.
 */
#ifdef CONFIG_USERSPACE
/* User mode cannot read the guest registers z_hexagon_fatal_error() uses. */
#define ARCH_EXCEPT(reason_p)						\
	do {								\
		if (k_is_user_context()) {				\
			arch_syscall_invoke1(reason_p,			\
					     K_SYSCALL_HEXAGON_USER_FAULT);	\
		} else {						\
			z_hexagon_fatal_error(reason_p);		\
		}							\
		CODE_UNREACHABLE;					\
	} while (false)

__syscall void hexagon_user_fault(unsigned int reason);

#include <zephyr/syscalls/error.h>
#else
#define ARCH_EXCEPT(reason_p)			\
	do {					\
		z_hexagon_fatal_error(reason_p);	\
		CODE_UNREACHABLE;		\
	} while (false)
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_HEXAGON_ERROR_H_ */
