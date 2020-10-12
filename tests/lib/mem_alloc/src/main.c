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

#include <zephyr.h>
#include <ztest.h>
#include <stdlib.h>
#include <errno.h>

#define BUF_LEN 10

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
 * @brief Test dynamic memory allocation using calloc
 *
 * @see calloc(), free()
 */
#define CALLOC_BUFLEN (200)
static ZTEST_BMEM unsigned char zerobuf[CALLOC_BUFLEN];

void test_calloc(void)
{
	char *cptr = NULL;

	cptr =  calloc(CALLOC_BUFLEN, sizeof(char));
	zassert_not_null((cptr), "calloc failed, errno: %d", errno);
	zassert_true(((memcmp(cptr, zerobuf, CALLOC_BUFLEN)) == 0),
			"calloc failed to set zero value, errno: %d", errno);
	memset(cptr, 'p', CALLOC_BUFLEN);
	free(cptr);
	cptr = NULL;
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
	zassert_not_null((ptr), "malloc/realloc failed, errno: %d", errno);
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
#else
void test_reallocarray(void)
{
	char orig_size = BUF_LEN;
	char *ptr = NULL;

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
	zassert_not_null((mlc_ptr), "malloc/realloc failed, errno: %d", errno);
	mlc_ptr = reloc_ptr;

	free(mlc_ptr);
	free(clc_ptr);
	mlc_ptr = NULL;
	clc_ptr = NULL;
	reloc_ptr = NULL;
}

/**
 *
 * @brief Test dynamic memory allocation upto maximum size
 * Negative test case
 *
 */

__no_optimization void test_memalloc_max(void)
{
	char *ptr = NULL;

	ptr = malloc(0x7fffffff);
	zassert_is_null(ptr, "malloc passed unexpectedly");
	free(ptr);
	ptr = NULL;
}

void test_main(void)
{
	ztest_test_suite(test_c_lib_dynamic_memalloc,
			 ztest_user_unit_test(test_malloc),
			 ztest_user_unit_test(test_free),
			 ztest_user_unit_test(test_calloc),
			 ztest_user_unit_test(test_realloc),
			 ztest_user_unit_test(test_reallocarray),
			 ztest_user_unit_test(test_memalloc_all),
			 ztest_user_unit_test(test_memalloc_max)
			 );
	ztest_run_test_suite(test_c_lib_dynamic_memalloc);
}
