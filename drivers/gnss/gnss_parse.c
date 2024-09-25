/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "gnss_parse.h"

#define GNSS_PARSE_NANO_KNOTS_IN_MMS               (1943840LL)
#define GNSS_PARSE_NANO                            (1000000000LL)
#define GNSS_PARSE_MICRO                           (1000000LL)
#define GNSS_PARSE_MILLI                           (1000LL)

int gnss_parse_dec_to_nano(const char *str, int64_t *nano)
{
	int64_t sum = 0;
	int8_t decimal = -1;
	int8_t pos = 0;
	int8_t start = 0;
	int64_t increment;

	__ASSERT(str != NULL, "str argument must be provided");
	__ASSERT(nano != NULL, "nano argument must be provided");

	/* Find decimal */
	while (str[pos] != '\0') {
		/* Verify if char is decimal */
		if (str[pos] == '.') {
			decimal = pos;
			break;
		}

		/* Advance position */
		pos++;
	}

	/* Determine starting position based on decimal location */
	pos = decimal < 0 ? pos - 1 : decimal - 1;

	/* Skip sign if it exists */
	start = str[0] == '-' ? 1 : 0;

	/* Add whole value to sum */
	increment = GNSS_PARSE_NANO;
	while (start <= pos) {
		/* Verify char is decimal */
		if (str[pos] < '0' || str[pos] > '9') {
			return -EINVAL;
		}

		/* Add value to sum */
		sum += (str[pos] - '0') * increment;

		/* Update increment */
		increment *= 10;

		/* Degrement position */
		pos--;
	}

	/* Check if decimal was found */
	if (decimal < 0) {
		/* Set sign of sum */
		sum = start == 1 ? -sum : sum;

		*nano = sum;
		return 0;
	}

	/* Convert decimal part to nano fractions and add it to sum */
	pos = decimal + 1;
	increment = GNSS_PARSE_NANO / 10LL;
	while (str[pos] != '\0') {
		/* Verify char is decimal */
		if (str[pos] < '0' || str[pos] > '9') {
			return -EINVAL;
		}

		/* Add value to micro_degrees */
		sum += (str[pos] - '0') * increment;

		/* Update unit */
		increment /= 10;

		/* Increment position */
		pos++;
	}

	/* Set sign of sum */
	sum = start == 1 ? -sum : sum;

	*nano = sum;
	return 0;
}

int gnss_parse_dec_to_micro(const char *str, uint64_t *micro)
{
	int ret;

	__ASSERT(str != NULL, "str argument must be provided");
	__ASSERT(micro != NULL, "micro argument must be provided");

	ret = gnss_parse_dec_to_nano(str, micro);
	if (ret < 0) {
		return ret;
	}

	*micro = (*micro) / GNSS_PARSE_MILLI;
	return 0;
}


int gnss_parse_dec_to_milli(const char *str, int64_t *milli)
{
	int ret;

	__ASSERT(str != NULL, "str argument must be provided");
	__ASSERT(milli != NULL, "milli argument must be provided");

	ret = gnss_parse_dec_to_nano(str, milli);
	if (ret < 0) {
		return ret;
	}

	(*milli) = (*milli) / GNSS_PARSE_MICRO;
	return 0;
}

int gnss_parse_atoi(const char *str, uint8_t base, int32_t *integer)
{
	char *end;

	__ASSERT(str != NULL, "str argument must be provided");
	__ASSERT(integer != NULL, "integer argument must be provided");

	*integer = (int32_t)strtol(str, &end, (int)base);

	if ('\0' != (*end)) {
		return -EINVAL;
	}

	return 0;
}
