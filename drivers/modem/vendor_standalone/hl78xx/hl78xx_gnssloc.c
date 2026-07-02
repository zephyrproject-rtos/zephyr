/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Netfeasa Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hl78xx_gnssloc.h"
#include "../../../gnss/gnss_parse.h"

int gnssloc_dms_to_ndeg(const char *str, int64_t *ndeg)
{
	int64_t degrees = 0;
	int64_t minutes = 0;
	int64_t seconds_nano = 0;
	int64_t result;
	char direction = '\0';
	const char *ptr = str;
	char *end;
	char sec_buf[16];
	size_t sec_len;

	if (str == NULL || ndeg == NULL) {
		return -EINVAL;
	}

	while (*ptr == ' ') {
		ptr++;
	}

	degrees = strtol(ptr, &end, 10);
	if (ptr == end || degrees < 0 || degrees > 180) {
		return -EINVAL;
	}
	ptr = end;

	ptr = strstr(ptr, "Deg");
	if (ptr == NULL) {
		return -EINVAL;
	}
	ptr += 3;

	while (*ptr == ' ') {
		ptr++;
	}

	minutes = strtol(ptr, &end, 10);
	if (ptr == end || minutes < 0 || minutes > 59) {
		return -EINVAL;
	}
	ptr = end;

	ptr = strstr(ptr, "Min");
	if (ptr == NULL) {
		return -EINVAL;
	}
	ptr += 3;

	while (*ptr == ' ') {
		ptr++;
	}

	sec_len = 0;
	while (ptr[sec_len] &&
	       (ptr[sec_len] == '.' || (ptr[sec_len] >= '0' && ptr[sec_len] <= '9') ||
		ptr[sec_len] == '-')) {
		if (sec_len >= sizeof(sec_buf) - 1) {
			return -EINVAL;
		}

		sec_buf[sec_len] = ptr[sec_len];
		sec_len++;
	}
	sec_buf[sec_len] = '\0';

	if (sec_len == 0) {
		return -EINVAL;
	}

	if (gnss_parse_dec_to_nano(sec_buf, &seconds_nano) < 0) {
		return -EINVAL;
	}

	if (seconds_nano < 0 || seconds_nano >= 60000000000LL) {
		return -EINVAL;
	}

	ptr += sec_len;

	ptr = strstr(ptr, "Sec");
	if (ptr == NULL) {
		return -EINVAL;
	}
	ptr += 3;

	while (*ptr) {
		if (*ptr == 'N' || *ptr == 'S' || *ptr == 'E' || *ptr == 'W') {
			direction = *ptr;
			break;
		}
		ptr++;
	}

	if (direction == '\0') {
		return -EINVAL;
	}

	if ((direction == 'N' || direction == 'S') && degrees > 90) {
		return -EINVAL;
	}

	result = degrees * 1000000000LL;
	result += (minutes * 1000000000LL) / 60LL;
	result += seconds_nano / 3600LL;

	if (direction == 'S' || direction == 'W') {
		result = -result;
	}

	*ndeg = result;
	return 0;
}

int gnssloc_parse_value_with_unit(const char *str, int64_t *milli)
{
	int ret;
	char value_buf[32];
	const char *unit;
	size_t value_len;

	if (str == NULL || milli == NULL) {
		return -EINVAL;
	}

	unit = str;

	while (*unit) {
		if (*unit == ' ' && (*(unit + 1) == 'm' || *(unit + 1) == 'k')) {
			break;
		}

		unit++;
	}

	value_len = unit - str;

	if (value_len == 0 || value_len >= sizeof(value_buf)) {
		return -EINVAL;
	}

	memcpy(value_buf, str, value_len);
	value_buf[value_len] = '\0';

	ret = gnss_parse_dec_to_milli(value_buf, milli);
	return ret;
}

int gnssloc_parse_speed_to_mms(const char *str, uint32_t *mms)
{
	int64_t milli;
	int ret;

	if (str == NULL || mms == NULL) {
		return -EINVAL;
	}

	ret = gnssloc_parse_value_with_unit(str, &milli);
	if (ret < 0) {
		return ret;
	}

	if (milli < 0 || milli > UINT32_MAX) {
		return -ERANGE;
	}

	*mms = (uint32_t)milli;
	return 0;
}

int gnssloc_parse_gpstime(const char *str, struct gnss_time *utc)
{
	int year, month, day, hour, minute, second;

	if (str == NULL || utc == NULL) {
		return -EINVAL;
	}

	if (sscanf(str, "%d %d %d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) {
		return -EINVAL;
	}

	if (year < 2000 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31 ||
	    hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
		return -ERANGE;
	}

	utc->century_year = (uint8_t)(year % 100);
	utc->month = (uint8_t)month;
	utc->month_day = (uint8_t)day;
	utc->hour = (uint8_t)hour;
	utc->minute = (uint8_t)minute;
	utc->millisecond = (uint16_t)(second * 1000);

	return 0;
}
