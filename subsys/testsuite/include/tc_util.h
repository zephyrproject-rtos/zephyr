/* tc_utilities.h - testcase utilities header file */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TC_UTIL_H__
#define __TC_UTIL_H__

#include <zephyr.h>

#include <string.h>
#ifdef CONFIG_SHELL
#include <shell/shell.h>
#endif
#include <sys/printk.h>

#if defined CONFIG_ZTEST_TC_UTIL_USER_OVERRIDE
#include <tc_util_user_override.h>
#endif

#ifndef PRINT_DATA
#define PRINT_DATA(fmt, ...) printk(fmt, ##__VA_ARGS__)
#endif

#if defined CONFIG_ARCH_POSIX
#include "posix_board_if.h"
#endif

/**
 * @def TC_PRINT_RUNID
 * @brief Report a Run ID
 *
 * When the CPP symbol \c TC_RUNID is defined (for example, from the
 * compile environment), print the defined string ``RunID:
 * <TC_RUNID>`` when called (TC_END_REPORT() will also call it).
 *
 * This is used mainly when automating the execution and running of
 * multiple test cases, to verify that the expected image is being
 * executed (as sometimes the targets fail to flash or reset
 * properly).
 *
 * TC_RUNID is any string, that will be converted to a string literal.
 */
#define TC_STR_HELPER(x) #x
#define TC_STR(x) TC_STR_HELPER(x)
#ifdef TC_RUNID
#define TC_PRINT_RUNID PRINT_DATA("RunID: " TC_STR(TC_RUNID) "\n")
#else
#define TC_PRINT_RUNID do {} while (0)
#endif

#ifndef PRINT_LINE
#define PRINT_LINE                          \
	PRINT_DATA(                                                        \
		"============================================================" \
		"=======\n")
#endif

/* stack size and priority for test suite task */
#define TASK_STACK_SIZE (1024 * 2)

#define FMT_ERROR "%s - %s@%d. "

#define TC_PASS 0
#define TC_FAIL 1
#define TC_SKIP 2

#ifndef TC_PASS_STR
#define TC_PASS_STR "PASS"
#endif
#ifndef TC_FAIL_STR
#define TC_FAIL_STR "FAIL"
#endif
#ifndef TC_SKIP_STR
#define TC_SKIP_STR "SKIP"
#endif

static inline const char *TC_RESULT_TO_STR(int result)
{
	switch (result) {
	case TC_PASS:
		return TC_PASS_STR;
	case TC_FAIL:
		return TC_FAIL_STR;
	case TC_SKIP:
		return TC_SKIP_STR;
	default:
		return "?";
	}
}

#ifndef TC_ERROR
#define TC_ERROR(fmt, ...)                               \
	do {                                                 \
		PRINT_DATA(FMT_ERROR, "FAIL", __func__, __LINE__); \
		PRINT_DATA(fmt, ##__VA_ARGS__);                  \
	} while (0)
#endif

#ifndef TC_PRINT
#define TC_PRINT(fmt, ...) PRINT_DATA(fmt, ##__VA_ARGS__)
#endif

#ifndef TC_START
#define TC_START(name) PRINT_DATA("START - %s\n", name)
#endif

#ifndef TC_END
#define TC_END(result, fmt, ...) PRINT_DATA(fmt, ##__VA_ARGS__)
#endif

#ifndef Z_TC_END_RESULT
/* prints result and the function name */
#define Z_TC_END_RESULT(result, func)					\
	do {								\
		TC_END(result, " %s - %s\n", TC_RESULT_TO_STR(result), func); \
		PRINT_LINE;						\
	} while (0)
#endif

#ifndef TC_END_RESULT
#define TC_END_RESULT(result)                           \
	Z_TC_END_RESULT((result), __func__)
#endif

#if defined(CONFIG_ARCH_POSIX)
#define TC_END_POST(result) posix_exit(result)
#else
#define TC_END_POST(result)
#endif /* CONFIG_ARCH_POSIX */

#ifndef TC_END_REPORT
#define TC_END_REPORT(result)                               \
	do {                                                    \
		PRINT_LINE;                                         \
		TC_PRINT_RUNID;                                         \
		TC_END(result,                                      \
		       "PROJECT EXECUTION %s\n",               \
		       (result) == TC_PASS ? "SUCCESSFUL" : "FAILED");	\
		TC_END_POST(result);                                    \
	} while (0)
#endif

#if defined(CONFIG_SHELL)
#define TC_CMD_DEFINE(name)						\
	static int cmd_##name(const struct shell *shell, size_t argc,	\
			      char **argv) \
	{								\
		TC_START(__func__);					\
		name();							\
		TC_END_RESULT(TC_PASS);					\
		return 0;						\
	}
#define TC_CMD_ITEM(name)	cmd_##name
#else
#define TC_CMD_DEFINE(name)				\
	int cmd_##name(int argc, char *argv[]) 		\
	{						\
		TC_START(__func__);			\
		name();					\
		TC_END_RESULT(TC_PASS);			\
		return 0;				\
	}
#define TC_CMD_ITEM(name) {STRINGIFY(name), cmd_##name, "none"}
#endif

#endif /* __TC_UTIL_H__ */
