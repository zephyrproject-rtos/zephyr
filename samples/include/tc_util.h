/* tc_utilities.h - testcase utilities header file */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __TC_UTIL_H__
#define __TC_UTIL_H__

#if defined(CONFIG_NANOKERNEL)
#include <nanokernel.h>
#else
#include <microkernel.h>
#endif

#include <misc/printk.h>
#include <string.h>

#define PRINT_DATA(fmt, ...) printk(fmt, ##__VA_ARGS__)

#define PRINT_LINE                                                     \
	PRINT_DATA(                                                        \
		"============================================================" \
		"=======\n")

/* stack size and priority for test suite task */
#define TASK_STACK_SIZE (1024 * 2)

#define FAIL "FAIL"
#define PASS "PASS"
#define FMT_ERROR "%s - %s@%d. "

#define TC_PASS 0
#define TC_FAIL 1

#define TC_ERROR(fmt, ...)                               \
	do {                                                 \
		PRINT_DATA(FMT_ERROR, FAIL, __func__, __LINE__); \
		PRINT_DATA(fmt, ##__VA_ARGS__);                  \
	} while (0)

#define TC_PRINT(fmt, ...) PRINT_DATA(fmt, ##__VA_ARGS__)
#define TC_START(name) PRINT_DATA("tc_start() - %s\n", name)
#define TC_END(result, fmt, ...) PRINT_DATA(fmt, ##__VA_ARGS__)

/* prints result and the function name */
#define TC_END_RESULT(result)                           \
	do {                                                \
		PRINT_LINE;                                     \
		TC_END(result, "%s - %s.\n",                    \
			result == TC_PASS ? PASS : FAIL, __func__); \
	} while (0)

#define TC_END_REPORT(result)                               \
	do {                                                    \
		PRINT_LINE;                                         \
		TC_END(result,                                      \
			"PROJECT EXECUTION %s\n",               \
			result == TC_PASS ? "SUCCESSFUL" : "FAILED");   \
	} while (0)

#endif /* __TC_UTIL_H__ */
