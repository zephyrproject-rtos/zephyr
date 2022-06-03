/*
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>

#define MINDIFF 2.25e-308

float sqrtf(float square)
{
	float root, last, diff;

	root = square / 3.0;

	if (square <= 0) {
		return 0;
	}

	do {
		last = root;
		root = (root + square / root) / 2.0;
		diff = root - last;
	} while (diff > MINDIFF || diff < -MINDIFF);

	return root;
}
