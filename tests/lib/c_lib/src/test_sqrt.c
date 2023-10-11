/*
 * Copyright (c) 2023 Lawrence King
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <math.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>


#define local_abs(x) (((x) < 0) ? -(x) : (x))

#ifndef NAN
#define NAN	(__builtin_nansf(""))
#endif

#ifndef NANF
#define NANF	(__builtin_nans(""))
#endif

#ifndef INF
#define INF	(__builtin_inf())
#endif

#ifndef INFF
#define INFF	(__builtin_inff())
#endif

static float test_floats[] = {
	1.0f, 2.0f, 3.0f, 4.0f,
	5.0f, 6.0f, 7.0f, 8.0f, 9.0f,	/* numbers across the decade */
	3.14159265359f, 2.718281828f,	/* irrational numbers pi and e */
	123.4f,	0.025f,	0.10f,	1.875f	/* numbers with infinite */
					/* repeating binary representation */
	};
#define	NUM_TEST_FLOATS	(sizeof(test_floats)/sizeof(float))

	static double test_doubles[] = {
		1.0, 2.0, 3.0, 4.0,
		5.0, 6.0, 7.0, 8.0, 9.0,	/* numbers across the decade */
		3.14159265359, 2.718281828,	/* irrational numbers pi and e */
		123.4,	0.025,	0.10,	1.875	/* numbers with infinite */
						/* repeating binary representationa */
};
#define	NUM_TEST_DOUBLES	(sizeof(test_floats)/sizeof(float))

#ifndef	isinf
static int isinf(double x)
{
	union { uint64_t u; double d; } ieee754;
	ieee754.d = x;
	ieee754.u &= ~0x8000000000000000;	/* ignore the sign */
	return ((ieee754.u >> 52) == 0x7FF) &&
		((ieee754.u & 0x000fffffffffffff) == 0);
}
#endif

#ifndef	isnan
static int isnan(double x)
{
	union { uint64_t u; double d; } ieee754;
	ieee754.d = x;
	ieee754.u &= ~0x8000000000000000;	/* ignore the sign */
	return ((ieee754.u >> 52) == 0x7FF) &&
		((ieee754.u & 0x000fffffffffffff) != 0);
}
#endif

#ifndef	isinff
static int isinff(float x)
{
	union { uint32_t u; float f; } ieee754;
	ieee754.f = x;
	ieee754.u &= ~0x80000000;		/* ignore the sign */
	return ((ieee754.u >> 23) == 0xFF) &&
		((ieee754.u & 0x7FFFFF) == 0);
}
#endif

#ifndef	isnanf
static int isnanf(float x)
{
	union { uint32_t u; float f; } ieee754;
	ieee754.f = x;
	ieee754.u &= ~0x80000000;		/* ignore the sign */
	return ((ieee754.u >> 23) == 0xFF) &&
		((ieee754.u & 0x7FFFFF) != 0);
}
#endif

/* small errors are expected, computed as percentage error */
#define MAX_FLOAT_ERROR_PERCENT		(3.5e-5)
#define MAX_DOUBLE_ERROR_PERCENT	(4.5e-14)

ZTEST(test_c_lib, test_sqrtf)
{
int i;
float	exponent, resf, square, root_squared;
double	error;
uint32_t max_error;
int32_t ierror;
int32_t *p_square = (int32_t *)&square;
int32_t *p_root_squared = (int32_t *)&root_squared;


	max_error = 0;
	/* Conversion not supported with minimal_libc without
	 * CBPRINTF_FP_SUPPORT.
	 *
	 * Conversion not supported without FPU except on native POSIX.
	 */
	if (!(IS_ENABLED(CONFIG_FPU)
		 || IS_ENABLED(CONFIG_BOARD_NATIVE_POSIX))) {
		ztest_test_skip();
		return;
	}

	/* test the special cases of 0.0, NAN, -NAN, INF, -INF,  and -10.0 */
	zassert_true(sqrtf(0.0f) == 0.0f, "sqrtf(0.0)");
	zassert_true(isnanf(sqrtf(NANF)), "sqrt(nan)");
#ifdef issignallingf
	zassert_true(issignallingf(sqrtf(NANF)), "ssignalingf(sqrtf(nan))");
/*	printf("issignallingf();\n"); */
#endif
	zassert_true(isnanf(sqrtf(-NANF)), "isnanf(sqrtf(-nan))");
	zassert_true(isinff(sqrtf(INFF)), "isinff(sqrt(inf))");
	zassert_true(isnanf(sqrtf(-INFF)), "isnanf(sqrt(-inf))");
	zassert_true(isnanf(sqrtf(-10.0f)), "isnanf(sqrt(-10.0))");

	for (exponent = 1.0e-10f; exponent < 1.0e10f; exponent *= 10.0f) {
		for (i = 0; i < NUM_TEST_FLOATS; i++) {
			square = test_floats[i] * exponent;
			resf = sqrtf(square);
			root_squared = resf * resf;
			zassert_true((resf > 0.0f) && (resf < INFF),
					"sqrtf out of range");
			if ((resf > 0.0f) && (resf < INFF)) {
				error = (square - root_squared) /
					square * 100;
				if (error < 0.0) {
					error = -error;
				}
				/* square and root_squared should be almost identical
				 * except the last few bits, the EXOR will only set
				 * the bits that are different
				 */
				ierror = (*p_square - *p_root_squared);
				ierror = local_abs(ierror);
				if (ierror > max_error) {
					max_error = ierror;
				}
			} else {
				/* negative, +NaN, -NaN, inf or -inf */
				error = 0.0;
			}
			zassert_true(error < MAX_FLOAT_ERROR_PERCENT,
					"max sqrtf error exceeded");
		}
	}
	zassert_true(max_error < 0x03, "huge errors in sqrt implementation");
	/* print the max error */
	TC_PRINT("test_sqrtf max error %d counts\n", max_error);
}

ZTEST(test_c_lib, test_sqrt)
{
int i;
double	resd, error, square, root_squared, exponent;
uint64_t max_error;
int64_t ierror;
int64_t *p_square = (int64_t *)&square;
int64_t *p_root_squared = (int64_t *)&root_squared;


	max_error = 0;
	/*
	 * sqrt is not supported without FPU except on native POSIX.
	 */
	if (!(IS_ENABLED(CONFIG_FPU)
		 || IS_ENABLED(CONFIG_BOARD_NATIVE_POSIX))) {
		ztest_test_skip();
		return;
	}

	/* test the special cases of 0.0, NAN, -NAN, INF, -INF,  and -10.0 */
	zassert_true(sqrt(0.0) == 0.0, "sqrt(0.0)");
	zassert_true(isnan(sqrt(NAN)), "sqrt(nan)");
#ifdef issignalling
	zassert_true(issignalling(sqrt(NAN)), "ssignaling(sqrt(nan))");
/*	printf("issignalling();\n"); */
#endif
	zassert_true(isnan(sqrt(-NAN)), "isnan(sqrt(-nan))");
	zassert_true(isinf(sqrt(INF)), "isinf(sqrt(inf))");
	zassert_true(isnan(sqrt(-INF)), "isnan(sqrt(-inf))");
	zassert_true(isnan(sqrt(-10.0)), "isnan(sqrt(-10.0))");

	for (exponent = 1.0e-10; exponent < 1.0e10; exponent *= 10.0) {
		for (i = 0; i < NUM_TEST_DOUBLES; i++) {
			square = test_doubles[i] * exponent;
			resd = sqrt(square);
			root_squared = resd * resd;
			zassert_true((resd > 0.0) && (resd < INF),
					"sqrt out of range");
			if ((resd > 0.0) && (resd < INF)) {
				error = (square - root_squared) /
					square * 100;
				if (error < 0.0) {
					error = -error;
				}
				/* square and root_squared should be almost identical
				 * except the last few bits, the EXOR will only set
				 * the bits that are different
				 */
				ierror = (*p_square - *p_root_squared);
				ierror = local_abs(ierror);
				if (ierror > max_error) {
					max_error = ierror;
				}
			} else {
				/* negative, +NaN, -NaN, inf or -inf */
				error = 0.0;
			}
			zassert_true(error < MAX_DOUBLE_ERROR_PERCENT,
					"max sqrt error exceeded");
		}
	}
	zassert_true(max_error < 0x04, "huge errors in sqrt implementation");
	/* print the max error */
	TC_PRINT("test_sqrt max error %d counts\n", (uint32_t)max_error);
}
