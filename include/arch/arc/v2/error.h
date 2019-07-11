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
 * the exception caused by kernel will be handled in interrupt context
 * when the processor is already in interrupt context, no need to raise
 * a new exception; when the processor is in thread context, the exception
 * will be raised
 */
#define Z_ARCH_EXCEPT(reason_p)	do { \
	if (z_arc_v2_irq_unit_is_in_isr()) { \
		printk("@ %s:%d:\n", __FILE__,  __LINE__); \
		z_fatal_error(reason_p, 0); \
	} else {\
		__asm__ volatile ( \
		"mov r0, %[reason]\n\t" \
		"trap_s %[id]\n\t" \
		: \
		: [reason] "i" (reason_p), \
		[id] "i" (_TRAP_S_CALL_RUNTIME_EXCEPT) \
		: "memory"); \
		CODE_UNREACHABLE; \
	} \
	} while (false)

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_ERROR_H_ */
