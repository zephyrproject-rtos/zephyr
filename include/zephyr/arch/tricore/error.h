/*
 * Copyright (c) 2025 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TriCore public error handling
 *
 * TriCore-specific kernel error handling interface. Included by
 * tricore/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_TRICORE_ERROR_H_
#define ZEPHYR_INCLUDE_ARCH_TRICORE_ERROR_H_
#include <stdbool.h>
#include <zephyr/arch/tricore/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

#define TRICORE_SYSCALL_EXCEPT_STR STRINGIFY(TRICORE_SYSCALL_EXCEPT)

#define ARCH_EXCEPT(reason_p)                                                                      \
	do {                                                                                       \
		__asm__ volatile("mov %%d4, %[_reason]\n"                                          \
				 "syscall " TRICORE_SYSCALL_EXCEPT_STR                             \
				 "\n" ::[_reason] "r"(reason_p)                                    \
				 : "d4", "memory");                                                \
	} while (false)

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_TRICORE_ERROR_H_ */
