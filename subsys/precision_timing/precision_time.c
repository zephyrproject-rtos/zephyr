/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

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

	if (b == PRECISION_TIME_MIN) {
		if (a >= 0) {
			return -ERANGE;
		}

		*result = PRECISION_TIME_MAX + a + 1;

		return 0;
	}

	return precision_time_add(a, -b, result);
}

int precision_time_from_u64_sec_nsec(uint64_t sec, uint32_t nsec, precision_time_t *result)
{
	precision_time_t sec_ns;

	if (result == NULL) {
		return -EINVAL;
	}

	if (nsec >= NSEC_PER_SEC || sec > (uint64_t)(PRECISION_TIME_MAX / NSEC_PER_SEC)) {
		return -ERANGE;
	}

	sec_ns = (precision_time_t)sec * NSEC_PER_SEC;

	return precision_time_add(sec_ns, (precision_time_t)nsec, result);
}

int precision_time_to_u64_sec_nsec(precision_time_t time, uint64_t *sec, uint32_t *nsec)
{
	if (sec == NULL || nsec == NULL) {
		return -EINVAL;
	}

	if (time < 0) {
		return -ERANGE;
	}

	*sec = (uint64_t)(time / NSEC_PER_SEC);
	*nsec = (uint32_t)(time % NSEC_PER_SEC);

	return 0;
}
