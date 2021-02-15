/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARCv2 public error handling
 *
 * ARC-specific kernel error handling interface. Included by arc/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_ERROR_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_ERROR_H_

#include <arch/arc/syscall.h>
#include <arch/arc/v2/exc.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * use trap_s to raise a SW exception
 */
#define ARCH_EXCEPT(reason_p)	do { \
		__asm__ volatile ( \
		"mov %%r0, %[reason]\n\t" \
		"trap_s %[id]\n\t" \
		: \
		: [reason] "i" (reason_p), \
		[id] "i" (_TRAP_S_CALL_RUNTIME_EXCEPT) \
		: "memory"); \
		CODE_UNREACHABLE; /* LCOV_EXCL_LINE */ \
	} while (false)

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_ERROR_H_ */
