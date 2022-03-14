/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

static size_t whitespace_trim(char *out, size_t len, const char *str)
{
	if (len == 0) {
		return 0;
	}

	const char *end;
	size_t out_size;

	while (str[0] == ' ') {
		str++;
	}

	if (*str == 0) {
		*out = 0;
		return 1;
	}

	end = str + strlen(str) - 1;
	while (end > str && (end[0] == ' ')) {
		end--;
	}
	end++;

	out_size = (end - str) + 1;

	if (out_size > len) {
		return 0;
	}

	memcpy(out, str, out_size - 1);
	out[out_size - 1] = 0;

	return out_size;
}

int string_conv_str2long(char *str, long *val)
{
	char trimmed_buf[12] = { 0 };
	size_t len = whitespace_trim(trimmed_buf, sizeof(trimmed_buf), str);

	if (len < 2) {
		return -EINVAL;
	}

	long temp_val;
	int idx = 0;

	if ((trimmed_buf[0] == '-') || (trimmed_buf[0] == '+')) {
		idx++;
	}

	for (int i = idx; i < (len - 1); i++) {
		if (!isdigit((int)trimmed_buf[i])) {
			return -EINVAL;
		}
	}

	errno = 0;
	temp_val = strtol(trimmed_buf, NULL, 10);

	if (errno == ERANGE) {
		return -ERANGE;
	}

	*val = temp_val;
	return 0;
}

int string_conv_str2ulong(char *str, unsigned long *val)
{
	char trimmed_buf[12] = { 0 };
	size_t len = whitespace_trim(trimmed_buf, sizeof(trimmed_buf), str);

	if (len < 2) {
		return -EINVAL;
	}

	unsigned long temp_val;
	int idx = 0;

	if (trimmed_buf[0] == '+') {
		idx++;
	}

	for (int i = idx; i < (len - 1); i++) {
		if (!isdigit((int)trimmed_buf[i])) {
			return -EINVAL;
		}
	}

	errno = 0;
	temp_val = strtoul(trimmed_buf, NULL, 10);

	if (errno == ERANGE) {
		return -ERANGE;
	}

	*val = temp_val;
	return 0;
}

int string_conv_str2dbl(const char *str, double *val)
{
	char trimmed_buf[22] = { 0 };
	long decimal;
	unsigned long frac;
	double frac_dbl;
	int err = 0;

	size_t len = whitespace_trim(trimmed_buf, sizeof(trimmed_buf), str);

	if (len < 2) {
		return -EINVAL;
	}

	int comma_idx = strcspn(trimmed_buf, ".");
	int frac_len = strlen(trimmed_buf + comma_idx + 1);

	/* Covers corner case "." input */
	if (strlen(trimmed_buf) < 2 && trimmed_buf[comma_idx] != 0) {
		return -EINVAL;
	}

	trimmed_buf[comma_idx] = 0;

	/* Avoid fractional overflow by losing one precision point */
	if (frac_len > 9) {
		trimmed_buf[comma_idx + 10] = 0;
		frac_len = 9;
	}

	/* Avoid doing str2long if decimal part is empty */
	if (trimmed_buf[0] == '\0') {
		decimal = 0;
	} else {
		err = string_conv_str2long(trimmed_buf, &decimal);

		if (err) {
			return err;
		}
	}

	/* Avoid doing str2ulong if fractional part is empty */
	if ((trimmed_buf + comma_idx + 1)[0] == '\0') {
		frac = 0;
	} else {
		err = string_conv_str2ulong(trimmed_buf + comma_idx + 1, &frac);

		if (err) {
			return err;
		}
	}

	frac_dbl = (double)frac;

	for (int i = 0; i < frac_len; i++) {
		frac_dbl /= 10;
	}

	*val = (trimmed_buf[0] == '-') ? ((double)decimal - frac_dbl) :
					       ((double)decimal + frac_dbl);

	return err;
}
