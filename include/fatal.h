/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_FATAL_H
#define ZEPHYR_INCLUDE_FATAL_H

#include <arch/cpu.h>
#include <toolchain.h>

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
	K_ERR_KERNEL_PANIC

	/* TODO: add more codes for exception types that are common across
	 * architectures
	 */
};

/**
 * @brief Halt the system on a fatal error
 *
 * Invokes architecture-specific code to power off or halt the system in
 * a low power state. Lacking that, lock interupts and sit in an idle loop.
 *
 * @param reason Fatal exception reason code
 */
FUNC_NORETURN void k_fatal_halt(unsigned int reason);

/**
 * @brief Fatal error policy handler
 *
 * This function is not invoked by application code, but is declared as a
 * weak symbol so that applications may introduce their own policy.
 *
 * The default implementation of this function halts the system
 * unconditionally. Depending on architecture support, this may be
 * a simple infinite loop, power off the hardware, or exit an emulator.
 *
 * If this function returns, then the currently executing thread will be
 * aborted.
 *
 * A few notes for custom implementations:
 *
 * - If the error is determined to be unrecoverable, LOG_PANIC() should be
 *   invoked to flush any pending logging buffers.
 * - K_ERR_KERNEL_PANIC indicates a severe unrecoverable error in the kernel
 *   itself, and should not be considered recoverable. There is an assertion
 *   in z_fatal_error() to enforce this.
 * - Even outside of a kernel panic, unless the fault occurred in user mode,
 *   the kernel itself may be in an inconsistent state, with API calls to
 *   kernel objects possibly exhibiting undefined behavior or triggering
 *   another exception.
 *
 * @param reason The reason for the fatal error
 * @param esf Exception context, with details and partial or full register
 *            state when the error occurred. May in some cases be NULL.
 */
void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *esf);

/**
 * Called by architecture code upon a fatal error.
 *
 * This function dumps out architecture-agnostic information about the error
 * and then makes a policy decision on what to do by invoking
 * k_sys_fatal_error_handler().
 *
 * On architectures where k_thread_abort() never returns, this function
 * never returns either.
 *
 * @param reason The reason for the fatal error
 * @param esf Exception context, with details and partial or full register
 *            state when the error occurred. May in some cases be NULL.
 */
void z_fatal_error(unsigned int reason, const z_arch_esf_t *esf);

/**
 * Print messages related to an exception
 *
 * This ensures the following:
 *  - The log system will enter panic mode if it is not already
 *  - Messages will be sent to printk() or the log subsystem if it is enabled
 *
 * Log subsystem filtering is disabled.
 * To conform with log subsystem semantics, newlines are automatically
 * appended, invoke this once per line.
 *
 * @param fmt Format string
 * @param ... Optional list of format arguments
 *
 * FIXME: Implemented in C file to avoid #include loops, disentangle and
 * make this a macro
 */
#if defined(CONFIG_LOG) || defined(CONFIG_PRINTK)
__printf_like(1, 2) void z_fatal_print(const char *fmt, ...);
#else
#define z_fatal_print(...) do { } while (false)
#endif

#endif /* ZEPHYR_INCLUDE_FATAL_H */
