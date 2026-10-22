/* printk.h - low-level debug output */

/*
 * Copyright (c) 2010-2012, 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_SYS_PRINTK_H_
#define ZEPHYR_INCLUDE_SYS_PRINTK_H_

#include <zephyr/toolchain.h>
#include <stddef.h>
#include <stdarg.h>
#include <inttypes.h>

#ifdef CONFIG_LOG_PRINTK_STATIC
#include <zephyr/logging/log_core.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * @brief Print kernel debugging message.
 *
 * This routine prints a kernel debugging message to the system console.
 * Output is send immediately, without any mutual exclusion or buffering.
 *
 * A basic set of conversion specifier characters are supported:
 *   - signed decimal: \%d, \%i
 *   - unsigned decimal: \%u
 *   - unsigned hexadecimal: \%x (\%X is treated as \%x)
 *   - pointer: \%p
 *   - string: \%s
 *   - character: \%c
 *   - percent: \%\%
 *
 * Field width (with or without leading zeroes) is supported.
 * Length attributes h, hh, l, ll and z are supported. However, integral
 * values with %lld and %lli are only printed if they fit in a long
 * otherwise 'ERR' is printed. Full 64-bit values may be printed with %llx.
 * Flags and precision attributes are not supported.
 *
 * @param fmt Format string.
 * @param ... Optional list of format arguments.
 */
#if defined(CONFIG_PRINTK) || defined(__DOXYGEN__)

#ifdef CONFIG_LOG_PRINTK_STATIC
/* If printk is redirected to the logging use the macro which allow build time
 * logging message creation which is faster and allows format string stripping in
 * case of dictionary based logging.
 */
#define printk(...) Z_LOG_PRINTK(0, __VA_ARGS__)
#else
__printf_like(1, 2) void printk(const char *fmt, ...);
#endif /* CONFIG_LOG_PRINTK_STATIC */

/**
 * @brief Print kernel debugging message, va_list version
 *
 * See printk() for the output format.
 *
 * @param fmt Format string.
 * @param ap Format arguments.
 */
__printf_like(1, 0) void vprintk(const char *fmt, va_list ap);

/**
 * @brief Output a string without taking the printk lock
 *
 * Same output as printk(), but the spinlock is never taken and both the
 * logging subsystem and the user mode buffer are bypassed: text goes
 * straight to the platform's character output hook.
 *
 * For callers that cannot take the lock: a crash whose lock holder is
 * dead, output re-entered from inside printk() itself, or early boot
 * before the atomics a spinlock needs are usable. Output may interleave
 * with a concurrent printk(), which beats hanging or losing it.
 *
 * @note Kernel context only, and only as lock free as the output hook.
 *
 * @param fmt Format string.
 * @param ... Optional list of format arguments.
 */
__printf_like(1, 2) void printk_unlocked(const char *fmt, ...);

/**
 * @brief Output a string without any locking, va_list version
 *
 * See printk_unlocked() for when this is appropriate and what it gives up.
 *
 * @param fmt Format string.
 * @param ap Format arguments.
 */
__printf_like(1, 0) void vprintk_unlocked(const char *fmt, va_list ap);

/**
 * @brief Stop printk() taking its lock, for crash reporting
 *
 * After this, printk() behaves as if every caller had used
 * printk_unlocked(). Once a fatal error is being handled the lock cannot
 * be trusted: the faulting context may have died holding it, or a CPU
 * that will never run again may own it, and any later printk() would
 * block for ever. Individual call sites cannot fix that, because code
 * reached after the failure does not know it is in a crash.
 *
 * Called by the fatal error path. One way, for the rest of the system's
 * life.
 */
void printk_panic(void);

#else
static inline __printf_like(1, 2) void printk(const char *fmt, ...)
{
	ARG_UNUSED(fmt);
}

static inline __printf_like(1, 0) void vprintk(const char *fmt, va_list ap)
{
	ARG_UNUSED(fmt);
	ARG_UNUSED(ap);
}

static inline __printf_like(1, 2) void printk_unlocked(const char *fmt, ...)
{
	ARG_UNUSED(fmt);
}

static inline __printf_like(1, 0) void vprintk_unlocked(const char *fmt, va_list ap)
{
	ARG_UNUSED(fmt);
	ARG_UNUSED(ap);
}

static inline void printk_panic(void)
{
}
#endif /* defined(CONFIG_PRINTK) || defined(__DOXYGEN__) */

#ifdef CONFIG_PICOLIBC

#include <stdio.h>

#define snprintk(...) snprintf(__VA_ARGS__)
#define vsnprintk(str, size, fmt, ap) vsnprintf(str, size, fmt, ap)

#else

/**
 * @brief Print a kernel debugging message to a buffer.
 *
 * Formats as printk() does, but writes to @p str instead of the console.
 * The output is truncated if it does not fit, and is NUL terminated as
 * long as @p size is not zero.
 *
 * @param str Buffer to write to.
 * @param size Size of the buffer, in bytes.
 * @param fmt Format string.
 * @param ... Optional list of format arguments.
 *
 * @return Number of characters that would have been written, not counting
 * the terminating NUL. A value of @p size or more means the output was
 * truncated.
 */
__printf_like(3, 4) int snprintk(char *str, size_t size,
					const char *fmt, ...);

/**
 * @brief Print a kernel debugging message to a buffer, va_list version
 *
 * See snprintk() for the formatting and the buffer handling.
 *
 * @param str Buffer to write to.
 * @param size Size of the buffer, in bytes.
 * @param fmt Format string.
 * @param ap Format arguments.
 *
 * @return Number of characters that would have been written, not counting
 * the terminating NUL.
 */
__printf_like(3, 0) int vsnprintk(char *str, size_t size,
					  const char *fmt, va_list ap);

#endif

#ifdef __cplusplus
}
#endif

#endif
