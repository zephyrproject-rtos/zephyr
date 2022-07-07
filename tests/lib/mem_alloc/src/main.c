/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file test dynamic memory allocation using C libraries
 *
 * This module verifies that the various dynamic memory allocation functions
 * works fine with minimal C library and newlib C library.
 *
 * IMPORTANT: The module only ensures that each supported library is present,
 * and that a bare minimum of its functionality is operating correctly. It does
 * NOT guarantee that ALL standards-defined functionality is present, nor does
 * it guarantee that ALL functionality provided is working correctly.
 */

#if defined(__GNUC__)
/*
 * Don't complain about ridiculous alloc size requests
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Walloc-size-larger-than="
#endif

#define _BSD_SOURCE
#include <zephyr/zephyr.h>
#include <ztest.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>

#define TOO_BIG PTRDIFF_MAX

/**
 *
 * @brief Test implementation-defined constants library
 * @defgroup libc_api
 * @ingroup all_tests
 * @{
 *
 */

#define BUF_LEN 10


/* The access test allocates objects of this type and dereferences members. */
union aligntest {
	long long       thelonglong;
	double          thedouble;
	uintmax_t       theuintmax_t;
	void            (*thepfunc)(void);
	time_t          thetime_t;
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

#define PRINT_TYPE_INFO(_t) \
	TC_PRINT("    %-14s  %4zu  %5zu\n", #_t, sizeof(_t), __alignof__(_t))

void test_malloc_align(void)
{
	char *ptr[64] = { NULL };

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

	/* Tries to use the malloc() implementation when in different states. */
	for (size_t i = 0; i < ARRAY_SIZE(ptr); i++) {
		union aligntest *aptr = NULL;

		ptr[i] = malloc(sizeof *(ptr[i]));
		aptr = malloc(sizeof(*aptr));
		if (aptr) {
			do_the_access(aptr);
		}
		free(aptr);
	}
	for (size_t i = 0; i < ARRAY_SIZE(ptr); i++) {
		free(ptr[i]);
		ptr[i] = NULL;
	}

	for (size_t n = 0; n < ARRAY_SIZE(ptr); n++) {
		union aligntest *aptr = NULL;

		for (size_t i = 0; i < n; i++) {
			ptr[i] = malloc(sizeof *(ptr[i]));
		}
		aptr = malloc(sizeof(*aptr));
		if (aptr) {
			do_the_access(aptr);
		}
		free(aptr);
		for (size_t i = 0; i < n; i++) {
			free(ptr[i]);
			ptr[i] = NULL;
		}
	}

	for (size_t n = 0; n < ARRAY_SIZE(ptr); n++) {
		union aligntest *aptr = NULL;

		ptr[n] = malloc(n);
		aptr = malloc(sizeof(*aptr));
		if (aptr) {
			do_the_access(aptr);
		}
		free(aptr);
		free(ptr[n]);
		ptr[n] = NULL;
	}
}

/**
 * @brief Test dynamic memory allocation using malloc
 *
 * @see malloc(), free()
 */
void test_malloc(void)
{
	/* Initialize error number to avoid garbage value, in case of SUCCESS */
	int *iptr = NULL;

	iptr = malloc(BUF_LEN * sizeof(int));
	zassert_not_null((iptr), "malloc failed, errno: %d", errno);
	memset(iptr, 'p', BUF_LEN * sizeof(int));
	free(iptr);
	iptr = NULL;
}
#if (CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE == 0)
__no_optimization void test_no_mem_malloc(void)
{
	int *iptr = NULL;

	iptr = malloc(BUF_LEN);
	zassert_is_null((iptr), "malloc failed, errno: %d", errno);
	free(iptr);
	iptr = NULL;
}
__no_optimization void test_no_mem_realloc(void)
{
	char *ptr = NULL;
	char *reloc_ptr = NULL;

	reloc_ptr = realloc(ptr, BUF_LEN);
	zassert_is_null(reloc_ptr, "realloc failed, errno: %d", errno);
	free(reloc_ptr);
	reloc_ptr = NULL;
}
#endif

/**
 * @brief Test dynamic memory allocation free function
 *
 * @see free()
 */
void test_free(void)
{
/*
 * In free, if ptr is passed as NULL, no operation is performed
 * Just make sure, no exception occurs and test pass
 */
	free(NULL);
}

/**
 * @brief Test dynamic memory allocation using realloc
 *
 * @see malloc(), realloc(), free()
 */
ZTEST_BMEM unsigned char filled_buf[BUF_LEN];

void test_realloc(void)
{
	char orig_size = BUF_LEN;
	char new_size = BUF_LEN + BUF_LEN;
	char *ptr = NULL;
	char *reloc_ptr = NULL;

	ptr = malloc(orig_size);

	zassert_not_null((ptr), "malloc failed, errno: %d", errno);
	(void)memset(ptr, 'p', orig_size);

	reloc_ptr = realloc(ptr, new_size);

	zassert_not_null(reloc_ptr, "realloc failed, errno: %d", errno);
	ptr = reloc_ptr;

	(void)memset(filled_buf, 'p', BUF_LEN);
	zassert_true(((memcmp(ptr, filled_buf, BUF_LEN)) == 0),
			"realloc failed to copy malloc data, errno: %d", errno);

	free(ptr);
	ptr = NULL;
}

/**
 * @brief Test dynamic memory allocation using reallocarray
 *
 * @see malloc(), reallocarray(), free()
 */
#ifdef CONFIG_NEWLIB_LIBC
void test_reallocarray(void)
{
	/* reallocarray not implemented for newlib */
	ztest_test_skip();
}
void test_calloc(void)
{
	ztest_test_skip();
}
#else
/**
 * @brief Test dynamic memory allocation using calloc
 *
 * @see calloc(), free()
 */
#define CALLOC_BUFLEN (200)
static ZTEST_BMEM unsigned char zerobuf[CALLOC_BUFLEN];

__no_optimization void test_calloc(void)
{
	char *cptr = NULL;

	cptr =  calloc(TOO_BIG, sizeof(int));
	zassert_is_null((cptr), "calloc failed, errno: %d", errno);
	free(cptr);

	cptr =  calloc(TOO_BIG, sizeof(char));
	zassert_is_null((cptr), "calloc failed, errno: %d", errno);
	free(cptr);

	cptr =  calloc(CALLOC_BUFLEN, sizeof(char));
	zassert_not_null((cptr), "calloc failed, errno: %d", errno);
	zassert_true(((memcmp(cptr, zerobuf, CALLOC_BUFLEN)) == 0),
			"calloc failed to set zero value, errno: %d", errno);
	memset(cptr, 'p', CALLOC_BUFLEN);
	free(cptr);
	cptr = NULL;
}

void test_reallocarray(void)
{
	char orig_size = BUF_LEN;
	char *ptr = NULL;
	char *cptr = NULL;

	cptr =  reallocarray(ptr, TOO_BIG, sizeof(int));
	zassert_is_null((ptr), "reallocarray failed, errno: %d", errno);
	zassert_is_null((cptr), "reallocarray failed, errno: %d");
	free(cptr);

	ptr = malloc(orig_size);

	zassert_not_null((ptr), "malloc failed, errno: %d", errno);
	(void)memset(ptr, 'p', orig_size);

	char *reloc_ptr = reallocarray(ptr, 2, orig_size);

	zassert_not_null(reloc_ptr, "reallocarray failed");
	zassert_not_null((ptr), "malloc/reallocarray failed, errno: %d", errno);
	ptr = reloc_ptr;

	(void)memset(filled_buf, 'p', BUF_LEN);
	zassert_true(((memcmp(ptr, filled_buf, BUF_LEN)) == 0),
			"realloc failed to copy malloc data, errno: %d", errno);

	free(ptr);
	ptr = NULL;
}
#endif


#define MAX_LEN (10 * BUF_LEN)

/**
 * @brief Test dynamic memory allocation functions
 *
 * @see malloc(), calloc(), realloc(), free()
 */
void test_memalloc_all(void)
{
	char *mlc_ptr = NULL;
	char *clc_ptr = NULL;
	char *reloc_ptr = NULL;
	int orig_size = BUF_LEN;
	int new_size = MAX_LEN;

	mlc_ptr = malloc(orig_size);
	zassert_not_null((mlc_ptr), "malloc failed, errno: %d", errno);

	clc_ptr = calloc(100, sizeof(char));
	zassert_not_null((clc_ptr), "calloc failed, errno: %d", errno);

	reloc_ptr = realloc(mlc_ptr, new_size);
	zassert_not_null(reloc_ptr, "realloc failed, errno: %d", errno);
	mlc_ptr = reloc_ptr;

	free(mlc_ptr);
	free(clc_ptr);
	mlc_ptr = NULL;
	clc_ptr = NULL;
	reloc_ptr = NULL;
}

/**
 * @}
 */

/**
 *
 * @brief Test dynamic memory allocation upto maximum size
 * Negative test case
 *
 */

__no_optimization void test_memalloc_max(void)
{
	char *ptr = NULL;

	ptr = malloc(TOO_BIG);
	zassert_is_null(ptr, "malloc passed unexpectedly");
	free(ptr);
	ptr = NULL;
}

void test_main(void)
{
#if defined(CONFIG_MINIMAL_LIBC) && CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE == 0
	ztest_test_suite(test_c_lib_dynamic_memalloc,
			 ztest_user_unit_test(test_no_mem_malloc),
			 ztest_user_unit_test(test_no_mem_realloc)
			 );
	ztest_run_test_suite(test_c_lib_dynamic_memalloc);
#else
	ztest_test_suite(test_c_lib_dynamic_memalloc,
			 ztest_user_unit_test(test_malloc_align),
			 ztest_user_unit_test(test_malloc),
			 ztest_user_unit_test(test_free),
			 ztest_user_unit_test(test_calloc),
			 ztest_user_unit_test(test_realloc),
			 ztest_user_unit_test(test_reallocarray),
			 ztest_user_unit_test(test_memalloc_all),
			 ztest_user_unit_test(test_memalloc_max)
			 );
	ztest_run_test_suite(test_c_lib_dynamic_memalloc);
#endif
}
