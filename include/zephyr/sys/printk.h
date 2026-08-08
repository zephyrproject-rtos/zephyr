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
#ifdef CONFIG_PRINTK

#ifdef CONFIG_LOG_PRINTK_STATIC
/* If printk is redirected to the logging use the macro which allow build time
 * logging message creation which is faster and allows format string stripping in
 * case of dictionary based logging.
 */
#define printk(...) Z_LOG_PRINTK(0, __VA_ARGS__)
#else
__printf_like(1, 2) void printk(const char *fmt, ...);
#endif /* CONFIG_LOG_PRINTK_STATIC */

__printf_like(1, 0) void vprintk(const char *fmt, va_list ap);

/**
 * @brief Output a string without any locking
 *
 * Same output as printk() but the internal spinlock is never taken, and
 * the logging subsystem and the user mode output buffer are both bypassed:
 * the text goes straight to the character output hook installed by the
 * platform.
 *
 * This is meant for contexts where taking a lock is impossible or unwise:
 *
 * - Fatal error and assertion paths. The lock may be held by the very
 *   context that failed, or by a CPU that will never release it, in which
 *   case a locking printk() would hang instead of reporting.
 * - Early boot, before locking primitives are usable. On some
 *   architectures the atomic instructions a spinlock relies on require the
 *   MMU to be enabled first.
 * - Re-entrant output. An assertion firing while a message is being
 *   printed would otherwise deadlock on a non-recursive spinlock.
 *
 * Output may therefore interleave with a concurrent printk(). In the
 * situations above that is preferable to deadlocking or losing the
 * message entirely.
 *
 * @note Only usable from kernel context, and only as lock free as the
 * character output hook behind it: a console driver that takes its own
 * lock reintroduces the problem.
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
 * @brief Switch printk() to its most robust output mode
 *
 * Tells printk() that the system is crashing, after which it stops taking
 * its internal spinlock, exactly as if every caller had used
 * printk_unlocked().
 *
 * Once a fatal error is being handled, that lock cannot be relied upon:
 * the faulting context may have died while holding it, or a CPU that will
 * never run again may own it. Any subsequent printk() would then block
 * forever, which loses not only the crash report but everything after it.
 * Converting individual callers is not enough, because code reached after
 * the failure has no way of knowing it is now running in a crash.
 *
 * Called by the fatal error path. There is no way back: the switch stays
 * in effect for the remaining life of the system, on the assumption that
 * whatever comes after a fatal error matters less than being able to
 * report it.
 */
void printk_panic(void);

#else
/** @cond INTERNAL_HIDDEN */
/* Stubs for CONFIG_PRINTK=n. The API is documented above; these carry no
 * documentation of their own so that Doxygen describes each function once.
 */
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
/** @endcond */
#endif

#ifdef CONFIG_PICOLIBC

#include <stdio.h>

#define snprintk(...) snprintf(__VA_ARGS__)
#define vsnprintk(str, size, fmt, ap) vsnprintf(str, size, fmt, ap)

#else

__printf_like(3, 4) int snprintk(char *str, size_t size,
					const char *fmt, ...);
__printf_like(3, 0) int vsnprintk(char *str, size_t size,
					  const char *fmt, va_list ap);

#endif

#ifdef __cplusplus
}
#endif

#endif
