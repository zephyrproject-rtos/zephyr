/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/precision_timing/precision_time.h>

int precision_time_add(precision_time_t a, precision_time_t b, precision_time_t *result)
{
	if (result == NULL) {
		return -EINVAL;
	}

	if ((b > 0 && a > PRECISION_TIME_MAX - b) || (b < 0 && a < PRECISION_TIME_MIN - b)) {
		return -ERANGE;
	}

	*result = a + b;

	return 0;
}

int precision_time_sub(precision_time_t a, precision_time_t b, precision_time_t *result)
{
	if (result == NULL) {
		return -EINVAL;
	}

	if ((b > 0 && a < PRECISION_TIME_MIN + b) || (b < 0 && a > PRECISION_TIME_MAX + b)) {
		return -ERANGE;
	}

	*result = a - b;

	return 0;
}
