/*
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/sys/util.h>

#define MAX_F_ITTERATIONS	6  /* usually converges in 4 loops */
				   /* this ensures we break out of the loop */
#define MAX_F_ERROR_COUNT	3  /* when result almost converges, stop */
#define EXP_MASK32	GENMASK(30, 23)

typedef union {
	float f;
	int32_t i;
} intfloat_t;

float sqrtf(float square)
{
	int i;
	int32_t exponent;
	intfloat_t root;
	intfloat_t last;
	intfloat_t p_square;

	p_square.f = square;

	if (square == 0.0f) {
		return square;
	}
	if (square < 0.0f) {
		return (square - square) / (square - square);
	}

	/* we need a good starting guess so that this will converge quickly,
	 * we can do this by dividing the exponent part of the float by 2
	 * this assumes IEEE-754 format doubles
	 */
	exponent = ((p_square.i & EXP_MASK32) >> 23) - 127;
	if (exponent == 0xFF - 127) {
		/* the number is a NAN or inf, return NaN or inf */
		return square + square;
	}
	exponent /= 2;
	root.i = (p_square.i & ~EXP_MASK32) | (exponent + 127) << 23;

	for (i = 0; i < MAX_F_ITTERATIONS; i++) {
		last = root;
		root.f = (root.f + square / root.f) * 0.5f;
		/* if (labs(*p_root - *p_last) < MAX_F_ERROR_COUNT) */
		if ((root.i ^ last.i) < MAX_F_ERROR_COUNT) {
			break;
		}
	}
	return root.f;
}
