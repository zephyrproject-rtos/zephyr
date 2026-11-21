/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MEMREF_TESTCASES_H_
#define MEMREF_TESTCASES_H_

#include <zephyr/sys/memref.h>
#include <zephyr/ztest.h>

typedef void *(*memref_test_alloc_t)(void);

void memref_test_basic(memref_test_alloc_t alloc);
void memref_test_stress_allocs(memref_test_alloc_t alloc);
void memref_test_payload_rw(memref_test_alloc_t alloc);
void memref_test_alignment(memref_test_alloc_t alloc, size_t align);
void memref_test_reuse_after_free(memref_test_alloc_t alloc);
void memref_test_independent_allocs(memref_test_alloc_t alloc);
void memref_test_unref_null(void);
void memref_test_drop(memref_test_alloc_t alloc);
void memref_test_drop_single_eval(memref_test_alloc_t alloc);

void memref_test_alloc_overflow(const struct memref_backend *be);
void memref_test_zero_size_alloc(const struct memref_backend *be);
void memref_test_varied_sizes(const struct memref_backend *be);

void memref_test_exhaustion(memref_test_alloc_t alloc, size_t capacity);
void memref_test_full_reuse(memref_test_alloc_t alloc, size_t capacity);

typedef void *(*memref_test_alloc_cb_t)(memref_destroy_cb_t destroy);

void memref_test_destroy_called(memref_test_alloc_cb_t alloc_cb);
void memref_test_multi_owners(memref_test_alloc_cb_t alloc_cb);
void memref_test_destroy_pointer_identity(memref_test_alloc_cb_t alloc_cb);
void memref_test_ref_depth(memref_test_alloc_cb_t alloc_cb);
void memref_test_drop_releases(memref_test_alloc_cb_t alloc_cb);

#define MEMREF_TESTCASES(suite, alloc, alloc_cb, align)				\
	ZTEST_SUITE(suite, NULL, NULL, NULL, NULL, NULL);			\
	ZTEST(suite, test_destroy_called)					\
	{									\
		Z_TEST_SKIP_IFNDEF(CONFIG_MEMREF_DESTROY_CB);			\
		memref_test_destroy_called(alloc_cb);				\
	}									\
	ZTEST(suite, test_multi_owners)						\
	{									\
		Z_TEST_SKIP_IFNDEF(CONFIG_MEMREF_DESTROY_CB);			\
		memref_test_multi_owners(alloc_cb);				\
	}									\
	ZTEST(suite, test_destroy_pointer_identity)				\
	{									\
		Z_TEST_SKIP_IFNDEF(CONFIG_MEMREF_DESTROY_CB);			\
		memref_test_destroy_pointer_identity(alloc_cb);			\
	}									\
	ZTEST(suite, test_ref_depth)						\
	{									\
		Z_TEST_SKIP_IFNDEF(CONFIG_MEMREF_DESTROY_CB);			\
		memref_test_ref_depth(alloc_cb);				\
	}									\
	ZTEST(suite, test_drop_releases)					\
	{									\
		Z_TEST_SKIP_IFNDEF(CONFIG_MEMREF_DESTROY_CB);			\
		memref_test_drop_releases(alloc_cb);				\
	}									\
	ZTEST(suite, test_basic)						\
	{									\
		memref_test_basic(alloc);					\
	}									\
	ZTEST(suite, test_stress_allocs)					\
	{									\
		memref_test_stress_allocs(alloc);				\
	}									\
	ZTEST(suite, test_payload_rw)						\
	{									\
		memref_test_payload_rw(alloc);					\
	}									\
	ZTEST(suite, test_alignment)						\
	{									\
		memref_test_alignment(alloc, align);				\
	}									\
	ZTEST(suite, test_reuse_after_free)					\
	{									\
		memref_test_reuse_after_free(alloc);				\
	}									\
	ZTEST(suite, test_independent_allocs)					\
	{									\
		memref_test_independent_allocs(alloc);				\
	}									\
	ZTEST(suite, test_unref_null)						\
	{									\
		memref_test_unref_null();					\
	}									\
	ZTEST(suite, test_drop)							\
	{									\
		memref_test_drop(alloc);					\
	}									\
	ZTEST(suite, test_drop_single_eval)					\
	{									\
		memref_test_drop_single_eval(alloc);				\
	}

#define MEMREF_VARLEN_TESTCASES(suite, be, alloc, alloc_cb, align)		\
	MEMREF_TESTCASES(suite, alloc, alloc_cb, align)				\
	ZTEST(suite, test_alloc_overflow)					\
	{									\
		memref_test_alloc_overflow(be);					\
	}									\
	ZTEST(suite, test_zero_size_alloc)					\
	{									\
		memref_test_zero_size_alloc(be);				\
	}									\
	ZTEST(suite, test_varied_sizes)						\
	{									\
		memref_test_varied_sizes(be);					\
	}

#define MEMREF_BLOCK_TESTCASES(suite, alloc, alloc_cb, capacity, align)		\
	MEMREF_TESTCASES(suite, alloc, alloc_cb, align)				\
	ZTEST(suite, test_exhaustion)						\
	{									\
		memref_test_exhaustion(alloc, capacity);			\
	}									\
	ZTEST(suite, test_full_reuse)						\
	{									\
		memref_test_full_reuse(alloc, capacity);			\
	}

#endif /* MEMREF_TESTCASES_H_ */
