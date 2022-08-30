/*
 * Copyright (c) 2020 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM AArch64 public error handling
 *
 * ARM AArch64-specific kernel error handling interface. Included by arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_ERROR_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_ERROR_H_

#include <zephyr/arch/arm64/syscall.h>
#include <zephyr/arch/arm64/exc.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARCH_EXCEPT(reason_p)						\
do {									\
	register uint64_t x8 __asm__("x8") = reason_p;			\
									\
	__asm__ volatile("svc %[id]\n"					\
			 :						\
			 : [id] "i" (_SVC_CALL_RUNTIME_EXCEPT),		\
				"r" (x8)				\
			 : "memory");					\
} while (false)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_ERROR_H_ */
