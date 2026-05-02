/*
 * Copyright (c) 2023 CSIRO.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief Fatal base type definitions
 */

#ifndef ZEPHYR_INCLUDE_FATAL_TYPES_H
#define ZEPHYR_INCLUDE_FATAL_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup fatal_types Fatal error base types
 * @ingroup fatal_apis
 * @{
 */

enum k_fatal_error_reason {
	/** Generic CPU exception, not covered by other codes */
	K_ERR_CPU_EXCEPTION,

	/** Unhandled hardware interrupt */
	K_ERR_SPURIOUS_IRQ,

	/** Faulting context overflowed its stack buffer */
	K_ERR_STACK_CHK_FAIL,

	/** Moderate severity software error */
	K_ERR_KERNEL_OOPS,

	/** High severity software error */
	K_ERR_KERNEL_PANIC,

	/** Arch specific fatal errors */
	K_ERR_ARCH_START = 16
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FATAL_TYPES_H */
