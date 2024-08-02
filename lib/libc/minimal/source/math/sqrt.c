/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/sys/util.h>

#define MAX_D_ITTERATIONS	8  /* usually converges in 5 loops */
				   /* this ensures we break out of the loop */
#define MAX_D_ERROR_COUNT	5  /* when result almost converges, stop */
#define EXP_MASK64	GENMASK64(62, 52)

double sqrt(double square)
{
	int i;
	int64_t	exponent;
	double root;
	double last;
	int64_t *p_square = (int64_t *)&square;
	int64_t *p_root = (int64_t *)&root;
	int64_t *p_last = (int64_t *)&last;

	if (square == 0.0) {
		return square;
	}
	if (square < 0.0) {
		return (square - square) / (square - square);
	}

	/* we need a good starting guess so that this will converge quickly,
	 * we can do this by dividing the exponent part of the float by 2
	 * this assumes IEEE-754 format doubles
	 */
	exponent = ((*p_square & EXP_MASK64)>>52)-1023;
	if (exponent == 0x7FF-1023) {
		/* the number is a NAN or inf, return NaN or inf */
		return square + square;
	}
	exponent /= 2;
	*p_root = (*p_square & ~EXP_MASK64) | (exponent+1023)<<52;

	for (i = 0; i < MAX_D_ITTERATIONS; i++) {
		last = root;
		root = (root + square / root) * 0.5;
		/* if (llabs(*p_root-*p_last)<MAX_D_ERROR_COUNT) */
		if ((*p_root ^ *p_last) < MAX_D_ERROR_COUNT) {
			break;
		}
	}
	return root;
}
