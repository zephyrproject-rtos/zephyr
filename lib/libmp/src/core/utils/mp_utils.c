/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>
#include <string.h>

#include "mp_utils.h"

int mp_util_gcd(int a, int b)
{
	int abs_a = (a < 0) ? -a : a;
	int abs_b = (b < 0) ? -b : b;

	if (abs_a == 0) {
		return abs_b;
	}
	if (abs_b == 0) {
		return abs_a;
	}

	int c = abs_a % abs_b;

	while (c != 0) {
		abs_a = abs_b;
		abs_b = c;
		c = abs_a % abs_b;
	}

	return abs_b;
}

int mp_util_lcm(int a, int b)
{
	if (a == 0 || b == 0) {
		return 0;
	}

	int abs_a = (a < 0) ? -a : a;
	int abs_b = (b < 0) ? -b : b;
	int gcd = mp_util_gcd(a, b);
	int result = (abs_a / gcd) * abs_b;

	if (result > INT_MAX) {
		return -1;
	}

	return result;
}
