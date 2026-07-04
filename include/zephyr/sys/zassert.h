/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_ZASSERT_H_
#define ZEPHYR_INCLUDE_SYS_ZASSERT_H_

#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

/**
 * @defgroup zassert Group based assert handling
 * @ingroup os_services
 * @{
 */

/*
 * Granular, per-source-file assertions.
 *
 * ZASSERT_GROUP() opts a translation unit into the group-aware ZASSERT()
 * macro. The resulting assertion level is a compile-time constant, so disabled
 * assertions (and, below ZASSERT_VERBOSE, the message and its arguments) are
 * folded away by the optimizer. An individual file may set its level
 * explicitly, overriding the group default.
 */

enum zassert_level {
	/* Asserts are disabled */
	ZASSERT_OFF = 0,
	/* Asserts enabled, message and its arguments are not compiled in */
	ZASSERT_ON,
	/* Asserts enabled with the message and its arguments */
	ZASSERT_VERBOSE
};

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
FUNC_NORETURN void zassert_fail(const char *cond, const char *file,
				unsigned int line, const char *fmt, ...);

/**
 * @brief Terminal action taken on assertion failure. Weak, overridable.
 *
 */
FUNC_NORETURN void zassert_post_action(const char *file, unsigned int line);

#ifdef __cplusplus
}
#endif

/** @brief Default assertion level of a named group, from Kconfig. */
#define _ZASSERT_GROUP_LEVEL(group) UTIL_CAT(UTIL_CAT(CONFIG_, group), _ZASSERT_LEVEL)

/*
 * ZASSERT_GROUP(group)        -> CONFIG_<group>_ZASSERT_LEVEL
 * ZASSERT_GROUP(group, level) -> explicit level
 */
#define _ZASSERT_LEVEL_RESOLVE(...) \
	GET_ARG_N(2, __VA_ARGS__, _ZASSERT_GROUP_LEVEL(GET_ARG_N(1, __VA_ARGS__)), _)

/**
 * @brief Select the Kconfig-defined assertion group used by ZASSERT() in this file.
 *
 * Must be placed at file scope before any use of ZASSERT() in the translation
 * unit. Every file that uses ZASSERT() must select a group.
 *
 * @param group Assertion group name, an UPPERCASE identifier. Its default level
 *              is taken from the Kconfig symbol CONFIG_<group>_ZASSERT_LEVEL,
 *              which must exist (the name is pasted verbatim, so it must match).
 * @param ...   Optional explicit level (ZASSERT_OFF, ZASSERT_ON or
 *              ZASSERT_VERBOSE) overriding the group default for this file.
 *
 * @note CONFIG_ZASSERT is the master switch: when it is disabled every group is
 *       forced to ZASSERT_OFF, regardless of the group default or an explicit
 *       level.
 */
#define ZASSERT_GROUP(...)								\
	BUILD_ASSERT((_ZASSERT_LEVEL_RESOLVE(__VA_ARGS__)) >= ZASSERT_OFF &&		\
			     (_ZASSERT_LEVEL_RESOLVE(__VA_ARGS__)) <= ZASSERT_VERBOSE,	\
		     "Invalid assert level");						\
	static const int __zassert_level __unused =					\
		IS_ENABLED(CONFIG_ZASSERT) ? (_ZASSERT_LEVEL_RESOLVE(__VA_ARGS__))	\
					   : ZASSERT_OFF

/**
 * @brief Group-aware assertion.
 *
 * Gated by the level from ZASSERT_GROUP() in this file. At
 * ZASSERT_ON the failing test and its location are reported; the message and
 * its arguments are only compiled in and printed at ZASSERT_VERBOSE.
 *
 * @param test Condition to check. A fatal error is raised if it is false.
 * @param ...  Optional message format string followed by its arguments. May be
 *             omitted when the check is self-explanatory.
 */
#define ZASSERT(test, ...)								\
	do {										\
		if ((__zassert_level >= ZASSERT_ON) && unlikely(!(test))) {		\
			if (__zassert_level == ZASSERT_VERBOSE) {			\
				zassert_fail(Z_STRINGIFY(test), __FILE__, __LINE__,	\
					     COND_CODE_1(IS_EMPTY(__VA_ARGS__),		\
							 (NULL), (__VA_ARGS__)));	\
			} else {							\
				zassert_fail(Z_STRINGIFY(test), __FILE__, __LINE__,	\
					     NULL);					\
			}								\
		}									\
	} while (false)
#endif /* ZEPHYR_INCLUDE_SYS_ZASSERT_H_ */
