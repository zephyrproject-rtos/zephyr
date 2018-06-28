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
#include <shell/shell.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define  PRINT_DATA(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#include <misc/printk.h>
#define PRINT_DATA(fmt, ...) printk(fmt, ##__VA_ARGS__)
#endif /* CONFIG_STDOUT_CONSOLE */

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
#define __str(x) #x
#define _str(x) __str(x)
#ifdef TC_RUNID
#define TC_PRINT_RUNID PRINT_DATA("RunID: " _str(TC_RUNID) "\n")
#else
#define TC_PRINT_RUNID do {} while (0)
#endif

#define PRINT_LINE                          \
	PRINT_DATA(                                                        \
		"============================================================" \
		"=======\n")

/* stack size and priority for test suite task */
#define TASK_STACK_SIZE (1024 * 2)

#define FMT_ERROR "%s - %s@%d. "

#define TC_PASS 0
#define TC_FAIL 1
#define TC_SKIP 2

static __unused const char *TC_RESULT_STR[] = {
	[TC_PASS] = "PASS",
	[TC_FAIL] = "FAIL",
	[TC_SKIP] = "SKIP",
};

#define TC_RESULT_TO_STR(result) TC_RESULT_STR[result]

#define TC_ERROR(fmt, ...)                               \
	do {                                                 \
		PRINT_DATA(FMT_ERROR, "FAIL", __func__, __LINE__); \
		PRINT_DATA(fmt, ##__VA_ARGS__);                  \
	} while (0)

#define TC_PRINT(fmt, ...) PRINT_DATA(fmt, ##__VA_ARGS__)
#define TC_START(name) PRINT_DATA("starting test - %s\n", name)
#define TC_END(result, fmt, ...) PRINT_DATA(fmt, ##__VA_ARGS__)

/* prints result and the function name */
#define _TC_END_RESULT(result, func)					\
	do {								\
		TC_END(result, "%s - %s\n", TC_RESULT_TO_STR(result), func); \
		PRINT_LINE;						\
	} while (0)
#define TC_END_RESULT(result)                           \
	_TC_END_RESULT((result), __func__)

#if defined(CONFIG_ARCH_POSIX)
#define TC_END_POST(result) posix_exit(result)
#else
#define TC_END_POST(result)
#endif /* CONFIG_ARCH_POSIX */

#define TC_END_REPORT(result)                               \
	do {                                                    \
		PRINT_LINE;                                         \
		TC_PRINT_RUNID;                                         \
		TC_END(result,                                      \
		       "PROJECT EXECUTION %s\n",               \
		       (result) == TC_PASS ? "SUCCESSFUL" : "FAILED");	\
		TC_END_POST(result);                                    \
	} while (0)

#define TC_CMD_DEFINE(name)				\
	int cmd_##name(int argc, char *argv[])		\
	{						\
		TC_START(__func__);			\
		name();					\
		TC_END_RESULT(TC_PASS);			\
		return 0;				\
	}

#define TC_CMD_ITEM(name) {STRINGIFY(name), cmd_##name, "none"}
#endif /* __TC_UTIL_H__ */
