/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_OOPS_H
#define ZEPHYR_OOPS_H

#include <arch/cpu.h>
#include <misc/printk.h>

#ifdef _ARCH_EXCEPT
/* This archtecture has direct support for triggering a CPU exception */
#define _k_except_reason(reason)	_ARCH_EXCEPT(reason)
#else

/* NOTE: This is the implementation for arches that do not implement
 * _ARCH_EXCEPT() to generate a real CPU exception.
 *
 * We won't have a real exception frame to determine the PC value when
 * the oops occurred, so print file and line number before we jump into
 * the fatal error handler.
 */
#define _k_except_reason(reason) do { \
		printk("@ %s:%d:\n", __FILE__,  __LINE__); \
		_NanoFatalErrorHandler(reason, &_default_esf); \
		CODE_UNREACHABLE; \
	} while (false)

#endif /* _ARCH__EXCEPT */

/**
 * @brief Fatally terminate a thread
 *
 * This should be called when a thread has encountered an unrecoverable
 * runtime condition and needs to terminate. What this ultimately
 * means is determined by the _fatal_error_handler() implementation, which
 * will be called will reason code _NANO_ERR_KERNEL_OOPS.
 *
 * If this is called from ISR context, the default system fatal error handler
 * will treat it as an unrecoverable system error, just like k_panic().
 * @req K-MISC-003
 */
#define k_oops()	_k_except_reason(_NANO_ERR_KERNEL_OOPS)

/**
 * @brief Fatally terminate the system
 *
 * This should be called when the Zephyr kernel has encountered an
 * unrecoverable runtime condition and needs to terminate. What this ultimately
 * means is determined by the _fatal_error_handler() implementation, which
 * will be called will reason code _NANO_ERR_KERNEL_PANIC.
 * @req K-MISC-004
 */
#define k_panic()	_k_except_reason(_NANO_ERR_KERNEL_PANIC)

#endif /* ZEPHYR_OOPS_H */
