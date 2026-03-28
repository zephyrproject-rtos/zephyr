/* exception.h - automatically selects the correct exception.h file to include */

/*
 * Copyright (c) 2024 Meta Platforms
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_EXCEPTION_H_

#if defined(CONFIG_EXCEPTION_DUMP_HOOK)

#include <stdbool.h>
#include <stdarg.h>

/**
 * @typedef exception_dump_hook_t
 * @brief Exception dump output callback.
 *
 * Called for each exception dump print with printf-style format and
 * argument list.
 *
 * @param format Printf-style format string.
 * @param args Variable argument list for @p format.
 */
typedef void (*arch_exception_dump_hook_t)(const char *format, va_list args);

/**
 * @typedef exception_drain_hook_t
 * @brief Exception dump flush callback.
 *
 * Called when exception dump output should be drained or reset.
 *
 * @param flush True to flush/reset buffered state, false to emit pending output.
 */
typedef void (*arch_exception_drain_hook_t)(bool flush);

extern arch_exception_dump_hook_t arch_exception_dump_hook;
extern arch_exception_drain_hook_t arch_exception_drain_hook;

/**
 * @brief Set exception dump callbacks.
 *
 * Registers callback handlers used by exception dump helpers.
 *
 * @param dump Callback for formatted exception dump output.
 * @param drain Callback to flush or reset buffered output state.
 */
static inline void arch_exception_set_dump_hook(arch_exception_dump_hook_t dump,
						arch_exception_drain_hook_t drain)
{
	arch_exception_dump_hook = dump;
	arch_exception_drain_hook = drain;
}

/**
 * @brief Call the registered exception drain callback.
 *
 * Calls the drain callback if one is registered.
 *
 * @param flush True to flush/reset buffered state, false to emit pending output.
 */
static inline void arch_exception_call_drain_hook(bool flush)
{
	if (arch_exception_drain_hook) {
		arch_exception_drain_hook(flush);
	}
}

/**
 * @brief Call the registered exception dump callback.
 *
 * Forwards a printf-style format string and arguments to the registered
 * exception dump callback, if present.
 *
 * @param format Printf-style format string.
 * @param ... Arguments for @p format.
 */
static inline void arch_exception_call_dump_hook(const char *format, ...)
{
	va_list args;

	if (arch_exception_dump_hook) {
		va_start(args, format);
		arch_exception_dump_hook(format, args);
		va_end(args);
	}
}

#if defined(CONFIG_EXCEPTION_DUMP_HOOK_ONLY)
#define EXCEPTION_DUMP(format, ...) arch_exception_call_dump_hook(format "\n",  ##__VA_ARGS__)
#elif defined(CONFIG_LOG)
#define EXCEPTION_DUMP(format, ...) arch_exception_call_dump_hook(format "\n",  ##__VA_ARGS__); \
	LOG_ERR(format, ##__VA_ARGS__)
#else
#define EXCEPTION_DUMP(format, ...) arch_exception_call_dump_hook(format "\n",  ##__VA_ARGS__); \
	printk(format "\n", ##__VA_ARGS__)
#endif

#else

#if defined(CONFIG_LOG)
#define EXCEPTION_DUMP(...) LOG_ERR(__VA_ARGS__)
#else
#define EXCEPTION_DUMP(format, ...) printk(format "\n", ##__VA_ARGS__)
#endif
#endif

#if defined(CONFIG_X86_64)
#include <zephyr/arch/x86/intel64/exception.h>
#elif defined(CONFIG_X86)
#include <zephyr/arch/x86/ia32/exception.h>
#elif defined(CONFIG_ARM64)
#include <zephyr/arch/arm64/exception.h>
#elif defined(CONFIG_ARM)
#include <zephyr/arch/arm/exception.h>
#elif defined(CONFIG_ARC)
#include <zephyr/arch/arc/v2/exception.h>
#elif defined(CONFIG_RISCV)
#include <zephyr/arch/riscv/exception.h>
#elif defined(CONFIG_XTENSA)
#include <zephyr/arch/xtensa/exception.h>
#elif defined(CONFIG_MIPS)
#include <zephyr/arch/mips/exception.h>
#elif defined(CONFIG_ARCH_POSIX)
#include <zephyr/arch/posix/exception.h>
#elif defined(CONFIG_SPARC)
#include <zephyr/arch/sparc/exception.h>
#elif defined(CONFIG_RX)
#include <zephyr/arch/rx/exception.h>
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_EXCEPTION_H_ */
