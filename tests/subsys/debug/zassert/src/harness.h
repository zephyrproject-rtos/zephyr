/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZASSERT_TEST_HARNESS_H_
#define ZASSERT_TEST_HARNESS_H_

#include <stdbool.h>
#include <setjmp.h>

extern jmp_buf harness_escape;

/*
 * Run a statement that may trip a ZASSERT(). Because the failure path is marked
 * unreachable, the harness zassert_fail() longjmp()s back here instead of
 * returning, so control resumes after the block either way.
 */
#define HARNESS_EXPECT(...)						\
	do {								\
		if (setjmp(harness_escape) == 0) {			\
			__VA_ARGS__;					\
		}							\
	} while (false)

void harness_reset(void *fixture);
int harness_counting_arg(void);
bool harness_fired(void);
bool harness_printed(void);
bool harness_contains(const char *needle);
int harness_arg_evals(void);

#endif /* ZASSERT_TEST_HARNESS_H_ */
