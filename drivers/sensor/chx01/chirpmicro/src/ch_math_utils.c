/*
 * Copyright 2023 Chirp Microsystems. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file ch_math_utils.c
 * @date July 20, 2017
 * @author nparikh
 * @brief Functions for performing fixed point arithmetic.
 *        https://github.com/dmoulding/log2fix
 *        https://github.com/chmike/fpsqrt
 *
 *        Integer square root function:
 * 			Algorithm and code Author Christophe Meessen 1993.
 */

#include <ch_math_utils.h>

fixed_t FP_sqrt(fixed_t x)
{
	uint32_t t, q, b, r;
	r = x;
	b = 0x40000000;
	q = 0;
	while (b > 0x40) {
		t = q + b;
		if (r >= t) {
			r -= t;
			/* equivalent to q += 2*b */
			q = t + b;
		}
		r <<= 1;
		b >>= 1;
	}
	q >>= 8;
	return q;
}

fixed_t FP_log2(fixed_t x)
{
	/* This implementation is based on Clay. S. Turner's fast binary logarithm
	 * algorithm[1].
	 */
	fixed_t b = 1U << (FRACT_BITS - 1);
	fixed_t y = 0;
	size_t i;
	fixed_t z;
	if (x == 0) {
		/* represents negative infinity */
		return 0;
	}
	while (x < 1U << FRACT_BITS) {
		x <<= 1;
		y -= 1U << FRACT_BITS;
	}
	while (x >= 2U << FRACT_BITS) {
		x >>= 1;
		y += 1U << FRACT_BITS;
	}
	z = x;
	for (i = 0; i < FRACT_BITS; i++) {
		z = FIXEDMUL(z, z);
		if (z >= 2U << FRACT_BITS) {
			z >>= 1;
			y += b;
		}
		b >>= 1;
	}
	return y;
}

fixed_t FP_log(fixed_t x)
{
	fixed_t y;

	/* macro value is in Q1.31 fomat, but we use Q16.  shift in two steps to keep multiply
	 * precision
	 */
	y = FIXEDMUL(FP_log2(x), (INV_LOG2_E_Q1DOT31 >> Q31_TO_Q16_SHIFT_1));
	y >>= Q31_TO_Q16_SHIFT_2;

	return y;
}

/*
 * int32_t sqrt_int32( int32_t v );
 *
 * Compute int32_t to int32_t square root
 * RETURNS the integer square root of v
 * REQUIRES v is positive
 *
 * Algorithm and code Author Christophe Meessen 1993.
 * Initially published in usenet comp.lang.c, Thu, 28 Jan 1993 08:35:23 GMT
 */
int32_t sqrt_int32(int32_t v)
{
	uint32_t t, q, b, r;
	r = v;		/* r = v - x */
	b = 0x40000000; /* a */
	q = 0;		/* 2ax */
	while (b > 0) {
		t = q + b;  /* t = 2ax + a */
		q >>= 1;    /* if a' = a/2, then q' = q/2 */
		if (r >= t) /* if (v - x) >= 2ax + a */
		{
			r -= t; /* r' = (v - x) - (2ax + a) */
			q += b; /* if x' = (x + a) then ax' = ax + a, thus q' = q' + b */
		}
		b >>= 2; /* if a' = a/2, then b' = b / 4 */
	}
	return q;
}
