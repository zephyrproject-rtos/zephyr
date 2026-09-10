/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RX arch public error handling
 *
 * Renesas RX-specific kernel error handling interface. Included by
 * rx/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RX_ERROR_H_
#define ZEPHYR_INCLUDE_ARCH_RX_ERROR_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARCH_EXCEPT(reason_p)                           \
	do {                                                \
		arch_irq_unlock(0);                             \
		__asm__ volatile("mov %[_reason], r1\n\t"       \
				 "int #2\n\t" ::[_reason] "r"(reason_p) \
				 : "r1", "memory");                     \
	} while (false)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_RX_ERROR_H_ */
