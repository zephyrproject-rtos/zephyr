/*
 * Copyright (c) 2021 Sun Amar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

/*
 * computes the value numerator/denominator and returns
 * the quotient and remainder in a struct format
 */
div_t div(int numerator, int denominator)
{
	div_t result = {
		.quot = numerator / denominator,
		.rem = numerator % denominator
	};

	return result;
}

ldiv_t ldiv(long numerator, long denominator)
{
	ldiv_t result = {
		.quot = numerator / denominator,
		.rem = numerator % denominator
	};

	return result;
}
