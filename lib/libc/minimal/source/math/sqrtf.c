/*
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <float.h>

float sqrtf(float square)
{
	float root, last, diff;

	root = square / 3.0f;

	if (square <= FLT_MIN) {
		return 0;
	}

	do {
		last = root;
		root = (root + square / root) / 2.0f;
		diff = root - last;
	} while (diff != 0.0f);

	return root;
}
