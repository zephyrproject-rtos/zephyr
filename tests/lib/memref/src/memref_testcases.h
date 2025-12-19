/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MEMREF_TESTCASES_H_
#define MEMREF_TESTCASES_H_

#include <zephyr/sys/memref.h>
#include <zephyr/ztest.h>

void memref_test_basic(const struct memref_backend *be);
void memref_test_cleanup_called(const struct memref_backend *be);
void memref_test_multi_owners(const struct memref_backend *be);
void memref_test_calloc_zeroed(const struct memref_backend *be);
void memref_test_stress_allocs(const struct memref_backend *be);

#define MEMREF_TESTCASES(suite, be)						\
	ZTEST_SUITE(suite, NULL, NULL, NULL, NULL, NULL);			\
	ZTEST(suite, test_basic)						\
	{									\
		memref_test_basic(be);					\
	}									\
	ZTEST(suite, test_cleanup_called)					\
	{									\
		memref_test_cleanup_called(be);				\
	}									\
	ZTEST(suite, test_multi_owners)						\
	{									\
		memref_test_multi_owners(be);				\
	}									\
	ZTEST(suite, test_calloc_zeroed)					\
	{									\
		memref_test_calloc_zeroed(be);				\
	}									\
	ZTEST(suite, test_stress_allocs)					\
	{									\
		memref_test_stress_allocs(be);				\
	}

#endif /* MEMREF_TESTCASES_H_ */
