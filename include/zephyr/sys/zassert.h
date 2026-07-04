/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Zephyrs assertion framework.
 *
 * The assertion framework provides a mechanism for checking conditions at runtime and reporting
 * failures in a structured way. It allows for module-based assertion levels, enabling developers to
 * control the verbosity and behavior of assertions on a per-module basis.
 *
 * The framework also supports custom handling of assertion failures, allowing developers to
 * override the default behavior and implement their own reporting and recovery/crash mechanism.
 */
#ifndef ZEPHYR_INCLUDE_SYS_ZASSERT_H_
#define ZEPHYR_INCLUDE_SYS_ZASSERT_H_

#include <stdarg.h>

#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

/**
 * @defgroup zassert Module based assert handling
 * @ingroup os_services
 * @{
 */

/*
 * Granular, per-source-file assertions.
 *
 * ZASSERT_MODULE() opts a scope into the module-aware ZASSERT() macro. The
 * resulting assertion level is a compile-time constant, so disabled assertions
 * (and, below ZASSERT_LEVEL_VERBOSE, the message and its arguments) are folded away by
 * the optimizer. An individual file may set its level explicitly, overriding the
 * module default.
 *
 * For headers and inline functions, place ZASSERT_MODULE() inside the function
 * body so the selection does not leak into the includer, or use ZASSERT_TERSE(),
 * ZASSERT_NORMAL() or ZASSERT_VERBOSE() for an assertion with a fixed level.
 */

/** Assertions are disabled and compiled out. */
#define ZASSERT_LEVEL_OFF     0
/** Assertions are checked; No additional information is reported. */
#define ZASSERT_LEVEL_TERSE   1
/** Assertions are checked; only the failing location is reported. */
#define ZASSERT_LEVEL_NORMAL  2
/** Assertions are checked; condition, location and message are reported. */
#define ZASSERT_LEVEL_VERBOSE 3

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Report a failed assertion and take terminal action. Weak, overridable.
 *
 * Consolidates the entire assertion cold path into a single out-of-line call:
 * the default implementation prints the location (and, when @p fmt is non-NULL,
 * the user message) and then invokes zassert_post_action(). Overriding this is
 * the single surface for capturing or redirecting the whole assertion output.
 *
 * @param cond Stringified failing condition.
 * @param file Source file of the failing assertion.
 * @param line Source line of the failing assertion.
 * @param fmt  Optional user message format string, or NULL when none.
 * @param ...  Arguments for @p fmt.
 */
#ifndef CONFIG_ASSERT_TEST
FUNC_NORETURN
#endif
__printf_like(4, 5)
void zassert_fail(const char *cond, const char *file, unsigned int line, const char *fmt, ...);

/**
 * @brief Terminal action taken on assertion failure. Weak, overridable.
 *
 */
#ifndef CONFIG_ASSERT_TEST
FUNC_NORETURN
#endif
void zassert_post_action(const char *file, unsigned int line);

/**
 * @brief Emit assertion text (va_list form). Weak, overridable.
 *
 * The single primitive through which all assertion output flows: the default
 * zassert_fail() and zassert_print() route their text here. Override it to
 * capture or redirect every assertion message from one place.
 *
 * @param fmt Message format string.
 * @param ap  Arguments for @p fmt.
 */
__printf_like(1, 0)
void zassert_vprint(const char *fmt, va_list ap);

/* @cond INTERNAL_HIDDEN */
/**
 * @brief Print a formatted assertion message.
 *
 * FIXME: This is used by the legacy __ASSERT_PRINT()/__ASSERT_LOC() compatibility shims.
 * Remove this function once the legacy shims are retired.
 *
 * @param fmt Message format string.
 * @param ... Arguments for @p fmt.
 */
static inline __printf_like(1, 2) void zassert_print(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	zassert_vprint(fmt, ap);
	va_end(ap);
}
/* @endcond */

#ifdef __cplusplus
}
#endif

/* @cond INTERNAL_HIDDEN */
/* Level of a named Kconfig module: CONFIG_ASSERT_MODULE_<module>_LEVEL. */
#define _ZASSERT_MODULE_LEVEL(module) UTIL_CAT(UTIL_CAT(CONFIG_ASSERT_MODULE_, module), _LEVEL)

/*
 * ZASSERT_MODULE(module)        -> CONFIG_ASSERT_MODULE_<module>_LEVEL
 * ZASSERT_MODULE(module, level) -> explicit level
 */
#define _ZASSERT_LEVEL_RESOLVE(...) \
	GET_ARG_N(2, __VA_ARGS__, _ZASSERT_MODULE_LEVEL(GET_ARG_N(1, __VA_ARGS__)), _)

#ifndef CONFIG_ASSERT
#define _ZASSERT(_level, _test, ...)
#define _ZASSERT_MODULE(...)
#else
#define _ZASSERT(_level, _test, ...)							\
	do {										\
		if ((_level) != ZASSERT_LEVEL_OFF && unlikely(!(_test))) {		\
			switch ((_level)) {						\
			case ZASSERT_LEVEL_VERBOSE:					\
				zassert_fail(Z_STRINGIFY(_test), __FILE__, __LINE__,	\
					     COND_CODE_1(IS_EMPTY(__VA_ARGS__),		\
							 (NULL), (__VA_ARGS__)));	\
				break;							\
			case ZASSERT_LEVEL_NORMAL:					\
				zassert_fail(NULL, __FILE__, __LINE__, NULL);		\
				break;							\
			case ZASSERT_LEVEL_TERSE:					\
				zassert_fail(NULL, NULL, 0, NULL);			\
				break;							\
			case ZASSERT_LEVEL_OFF: /* To silence warnings */		\
				break;							\
			}								\
		}									\
	} while (false)

#define _ZASSERT_MODULE(...)									\
	BUILD_ASSERT((_ZASSERT_LEVEL_RESOLVE(__VA_ARGS__)) >= ZASSERT_LEVEL_OFF &&		\
			     (_ZASSERT_LEVEL_RESOLVE(__VA_ARGS__)) <= ZASSERT_LEVEL_VERBOSE,	\
		     "Invalid assert level");							\
	static const int __zassert_level __unused =						\
		IS_ENABLED(CONFIG_ASSERT) ? (_ZASSERT_LEVEL_RESOLVE(__VA_ARGS__))		\
					   : ZASSERT_LEVEL_OFF

#endif /* ifndef CONFIG_ASSERT */
#define _ZASSERT_LEVEL(_level, _test, ...)							\
	do {											\
		BUILD_ASSERT((_level) >= ZASSERT_LEVEL_OFF && (_level) <= ZASSERT_LEVEL_VERBOSE,\
			     "Invalid assert level");						\
		_ZASSERT(_level, _test, ##__VA_ARGS__);						\
	} while (false)
/* @endcond */

/**
 * @brief Assertion that reports only a fixed banner (no location or message).
 *
 * @param _test Condition to check. A fatal error is raised if it is false.
 * @param ...  Ignored optional message format string and arguments.
 */
#define ZASSERT_TERSE(_test, ...) _ZASSERT_LEVEL(ZASSERT_LEVEL_TERSE, (_test), ##__VA_ARGS__)

/**
 * @brief Assertion that reports only the failing location.
 *
 * @param _test Condition to check. A fatal error is raised if it is false.
 * @param ...  Ignored optional message format string and arguments.
 */
#define ZASSERT_NORMAL(_test, ...) _ZASSERT_LEVEL(ZASSERT_LEVEL_NORMAL, (_test), ##__VA_ARGS__)

/**
 * @brief Assertion that reports the condition, location and optional message.
 *
 * @param _test Condition to check. A fatal error is raised if it is false.
 * @param ...  Optional message format string followed by its arguments.
 */
#define ZASSERT_VERBOSE(_test, ...) _ZASSERT_LEVEL(ZASSERT_LEVEL_VERBOSE, (_test), ##__VA_ARGS__)

/**
 * @brief Select the Kconfig-defined assertion module used by ZASSERT() in this file.
 *
 * Must be placed at file scope before any use of ZASSERT() in the translation
 * unit. Every file that uses ZASSERT() must select a module.
 *
 * @param ... The assertion module name (an UPPERCASE identifier) followed by an
 *            optional explicit level. The module's default level is taken from
 *            the Kconfig symbol CONFIG_ASSERT_MODULE_<module>_LEVEL, which must exist
 *            (the name is pasted verbatim, so it must match). An optional
 *            second argument (ZASSERT_LEVEL_OFF, ZASSERT_LEVEL_TERSE,
 *            ZASSERT_LEVEL_NORMAL or ZASSERT_LEVEL_VERBOSE) overrides the module
 *            default for this file.
 *
 * @note CONFIG_ASSERT is the master switch: when it is disabled every module is
 *       forced to ZASSERT_LEVEL_OFF, regardless of the module default or an explicit
 *       level.
 */
#define ZASSERT_MODULE(...) _ZASSERT_MODULE(__VA_ARGS__)
/**
 * @brief Module-aware assertion.
 *
 * Gated by the level from ZASSERT_MODULE() in this file. At ZASSERT_LEVEL_TERSE
 * only a fixed "ASSERTION FAIL" banner is reported. At ZASSERT_LEVEL_NORMAL the
 * failing location is added. At ZASSERT_LEVEL_VERBOSE the stringified condition
 * and the optional message and its arguments are also compiled in and printed.
 *
 * @param _test Condition to check. A fatal error is raised if it is false.
 * @param ...  Optional message format string followed by its arguments. May be
 *             omitted when the check is self-explanatory.
 */
#define ZASSERT(_test, ...) _ZASSERT(__zassert_level, _test, ##__VA_ARGS__)

/** @} */

#endif /* ZEPHYR_INCLUDE_SYS_ZASSERT_H_ */
