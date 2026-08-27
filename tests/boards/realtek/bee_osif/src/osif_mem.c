/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * Implementation based on: zephyr/tests/lib/mem_alloc/src/main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <os_mem.h>
#include <trace.h>
#include <mem_types.h>

#define TOO_BIG PTRDIFF_MAX

#define BUF_LEN 10

/* The access test allocates objects of this type and dereferences members. */
union aligntest {
	long long thelonglong;
	double thedouble;
	uintmax_t theuintmax_t;
	void (*thepfunc)(void);
	time_t thetime_t;
};

/* Make sure we can access some built-in types. */
static void do_the_access(volatile union aligntest *aptr)
{
	aptr->thelonglong = 2;
	aptr->thelonglong;

	if (IS_ENABLED(CONFIG_FPU)) {
		aptr->thedouble = 3.0;
		aptr->thedouble;
	}

	aptr->theuintmax_t = 4;
	aptr->theuintmax_t;

	aptr->thepfunc = test_main;
	aptr->thepfunc;

	aptr->thetime_t = 3;
	aptr->thetime_t;
}

#define PRINT_TYPE_INFO(_t) TC_PRINT("    %-14s  %4zu  %5zu\n", #_t, sizeof(_t), __alignof__(_t))

ZTEST(osif_mem, test_align_access)
{
	char *ptr[64] = {NULL};

	TC_PRINT("  Compiler type info for " CONFIG_ARCH " " CONFIG_BOARD "\n");
	TC_PRINT("    TYPE            SIZE  ALIGN\n");
	PRINT_TYPE_INFO(int);
	PRINT_TYPE_INFO(long);
	PRINT_TYPE_INFO(long long);
	PRINT_TYPE_INFO(double);
	PRINT_TYPE_INFO(size_t);
	PRINT_TYPE_INFO(void *);
	PRINT_TYPE_INFO(void (*)(void));
	PRINT_TYPE_INFO(time_t);

	/* Tries to use the os_mem_alloc( implementation when in different states. */
	for (size_t i = 0; i < ARRAY_SIZE(ptr); i++) {
		union aligntest *aptr = NULL;

		ptr[i] = os_mem_alloc(RAM_TYPE_DATA_ON, sizeof *(ptr[i]));
		aptr = os_mem_alloc(RAM_TYPE_DATA_ON, sizeof(*aptr));
		if (aptr) {
			do_the_access(aptr);
		}
		os_mem_free(aptr);
	}
	for (size_t i = 0; i < ARRAY_SIZE(ptr); i++) {
		os_mem_free(ptr[i]);
		ptr[i] = NULL;
	}

	for (size_t n = 0; n < ARRAY_SIZE(ptr); n++) {
		union aligntest *aptr = NULL;

		for (size_t i = 0; i < n; i++) {
			ptr[i] = os_mem_alloc(RAM_TYPE_DATA_ON, sizeof *(ptr[i]));
		}
		aptr = os_mem_alloc(RAM_TYPE_DATA_ON, sizeof(*aptr));
		if (aptr) {
			do_the_access(aptr);
		}
		os_mem_free(aptr);
		for (size_t i = 0; i < n; i++) {
			os_mem_free(ptr[i]);
			ptr[i] = NULL;
		}
	}

	for (size_t n = 0; n < ARRAY_SIZE(ptr); n++) {
		union aligntest *aptr = NULL;

		ptr[n] = os_mem_alloc(RAM_TYPE_DATA_ON, n);
		aptr = os_mem_alloc(RAM_TYPE_DATA_ON, sizeof(*aptr));
		if (aptr) {
			do_the_access(aptr);
		}
		os_mem_free(aptr);
		os_mem_free(ptr[n]);
		ptr[n] = NULL;
	}
}

/**
 * @brief Test dynamic memory allocation using os_mem_alloc.
 *
 * @see os_mem_alloc_intern_zephyr()
 */
ZTEST(osif_mem, test_os_mem_alloc)
{
	int *iptr = NULL;

	/* Basic alloc and free testing. */
	iptr = os_mem_alloc(RAM_TYPE_DATA_ON, BUF_LEN * sizeof(int));
	zassert_not_null((iptr), "os_mem_alloc failed, type RAM_TYPE_DATA_ON, errno: %d", errno);
	memset(iptr, 'p', BUF_LEN * sizeof(int));
	os_mem_free(iptr);
	iptr = NULL;

	iptr = os_mem_alloc(RAM_TYPE_BUFFER_ON, BUF_LEN * sizeof(int));
	zassert_not_null((iptr), "os_mem_alloc failed, type RAM_TYPE_BUFFER_ON errno: %d", errno);
	memset(iptr, 'p', BUF_LEN * sizeof(int));
	os_mem_free(iptr);
	iptr = NULL;

	/* Alloc a memory that is too big */
	iptr = os_mem_alloc(RAM_TYPE_DATA_ON, 180 * 1024);
	zassert_is_null((iptr), "os_mem_alloc passed unexpectedly");

	iptr = NULL;
}

/**
 * @brief Test dynamic memory peek function using os_mem_peek.
 *
 * @see os_mem_peek_zephyr()
 */
ZTEST(osif_mem, test_os_mem_peek)
{
	size_t p_size;

	p_size = os_mem_peek(RAM_TYPE_DATA_ON);
}

/**
 * @brief Test dynamic memory free function using os_mem_free.
 *
 * @see os_mem_free()
 */
ZTEST(osif_mem, test_os_mem_free)
{
	/*
	 * In os_mem_free, if ptr is passed as NULL, no operation is performed.
	 * Just make sure, no exception occurs and test pass
	 */
	os_mem_free(NULL);
	os_mem_aligned_free(NULL);
}

/**
 * @brief Test dynamic memory allocation function using os_mem_zalloc.
 *
 * @see os_mem_zalloc()
 */
ZTEST(osif_mem, test_os_mem_zalloc)
{
	int *iptr = NULL;

	/* Basic alloc and free testing. */
	iptr = os_mem_zalloc(RAM_TYPE_DATA_ON, BUF_LEN * sizeof(int));
	zassert_not_null((iptr), "os_mem_zalloc failed, type RAM_TYPE_DATA_ON, errno: %d", errno);
	memset(iptr, 'p', BUF_LEN * sizeof(int));
	os_mem_free(iptr);
	iptr = NULL;

	iptr = os_mem_zalloc(RAM_TYPE_DATA_ON, BUF_LEN * sizeof(int));
	zassert_not_null((iptr), "os_mem_zalloc failed, type RAM_TYPE_DATA_ON, errno: %d", errno);
	zassert_true((*iptr == 0 && *(iptr + BUF_LEN - 1) == 0),
		     "os_mem_zalloc failed, the value in the alloced region is not zero! *iptr:%d, "
		     "*(iptr+BUF_LEN-1) %d",
		     *iptr, *(iptr + BUF_LEN - 1));
	os_mem_free(iptr);
	iptr = NULL;
}

#define MAX_LEN (10 * BUF_LEN)

/**
 *
 * @brief Test dynamic memory allocation up to maximum size
 * Negative test case
 *
 */
__no_optimization void _test_memalloc_max(void)
{
	char *ptr = NULL;

	ptr = os_mem_alloc(RAM_TYPE_DATA_ON, TOO_BIG);
	zassert_is_null(ptr, "malloc passed unexpectedly");
	os_mem_free(ptr);
	ptr = NULL;
}

ZTEST(osif_mem, test_memalloc_max)
{
	_test_memalloc_max();
}

ZTEST_SUITE(osif_mem, NULL, NULL, NULL, NULL, NULL);
