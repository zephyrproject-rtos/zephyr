/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/realtime.h>
#include <zephyr/sys/internal/realtime.h>

#define MS_IN_SECOND         1000LL
#define MS_IN_MINUTE         (MS_IN_SECOND * 60)
#define MS_IN_HOUR           (MS_IN_MINUTE * 60)
#define MS_IN_DAY            (MS_IN_HOUR * 24)
#define MS_IN_YEAR           (MS_IN_DAY * 365)
#define MS_IN_LEAP_YEAR      (MS_IN_YEAR + MS_IN_DAY)
#define MS_IN_4_YEARS        (MS_IN_YEAR * 4)
#define MS_IN_4_LEAP_YEARS   (MS_IN_4_YEARS + MS_IN_DAY)
#define MS_IN_100_YEARS      ((MS_IN_YEAR * 76) + (MS_IN_LEAP_YEAR * 24))
#define MS_IN_100_LEAP_YEARS ((MS_IN_YEAR * 75) + (MS_IN_LEAP_YEAR * 25))
#define MS_IN_400_YEARS      ((MS_IN_YEAR * 303) + (MS_IN_LEAP_YEAR * 97))
#define NS_IN_MS             (1000000LL)

#define MIN_YEAR CONFIG_SYS_REALTIME_CONVERT_NATIVE_MIN_YEAR
#define MAX_YEAR CONFIG_SYS_REALTIME_CONVERT_NATIVE_MAX_YEAR

#define TIMESTAMP_MIN ((MIN_YEAR / 400) * MS_IN_400_YEARS)
#define TIMESTAMP_MAX ((MAX_YEAR / 400) * MS_IN_400_YEARS)
#define UTC_REFERENCE (62167219200000LL)
#define UTC_MIN       (TIMESTAMP_MIN - UTC_REFERENCE)
#define UTC_MAX       (TIMESTAMP_MAX - UTC_REFERENCE)

BUILD_ASSERT((MIN_YEAR % 400) == 0, "Minimum year must be divisible by 400");
BUILD_ASSERT((MAX_YEAR % 400) == 0, "Maximum year must be divisible by 400");
BUILD_ASSERT(MAX_YEAR > MIN_YEAR, "Maximum year must be larger than minimum year");

struct year_segment {
	int64_t ms;
	uint16_t years;
	bool repeats;
};

static const struct year_segment year_segments[] = {
	{
		.ms = MS_IN_400_YEARS,
		.years = 400,
		.repeats = true,
	},
	{
		.ms = MS_IN_100_LEAP_YEARS,
		.years = 100,
		.repeats = false,
	},
	{
		.ms = MS_IN_100_YEARS,
		.years = 100,
		.repeats = true,
	},
	{
		.ms = MS_IN_4_YEARS,
		.years = 4,
		.repeats = false,
	},
	{
		.ms = MS_IN_4_LEAP_YEARS,
		.years = 4,
		.repeats = true,
	},
	{
		.ms = MS_IN_LEAP_YEAR,
		.years = 1,
		.repeats = false,
	},
	{
		.ms = MS_IN_YEAR,
		.years = 1,
		.repeats = true,
	},
};

static const uint8_t days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static void timestamp_years_from_datetime(int64_t *years, const struct sys_datetime *datetime)
{
	*years = datetime->tm_year + 1900;
}

static void timestamp_move_minimum_years(int64_t *timestamp_ms, int64_t *years)
{
	*years -= MIN_YEAR;
	*timestamp_ms += TIMESTAMP_MIN;
}

static void timestamp_add_years(int64_t *timestamp_ms,
				bool *leap_year,
				const struct sys_datetime *datetime)
{
	int64_t difference;
	int64_t years;
	const struct year_segment *segment;

	*leap_year = false;

	timestamp_years_from_datetime(&years, datetime);
	timestamp_move_minimum_years(timestamp_ms, &years);

	for (uint8_t i = 0; i < ARRAY_SIZE(year_segments); i++) {
		segment = &year_segments[i];

		if (segment->years == 1) {
			*leap_year = !segment->repeats;
		}

		if (years < segment->years) {
			continue;
		}

		if (segment->repeats) {
			difference = years / segment->years;
			*timestamp_ms += difference * segment->ms;
			years %= segment->years;
		} else {
			years -= segment->years;
			*timestamp_ms += segment->ms;
		}
	}
}

static void timestamp_add_months(int64_t *timestamp_ms,
				 bool leap_year,
				 const struct sys_datetime *datetime)
{
	uint8_t months;
	uint32_t days;

	months = (uint8_t)datetime->tm_mon;

	for (uint8_t i = 0; i < months; i++) {
		days = days_in_month[i];
		*timestamp_ms += days * MS_IN_DAY;
	}

	if (leap_year && months > 1) {
		*timestamp_ms += MS_IN_DAY;
	}
}

static void timestamp_add_days(int64_t *timestamp_ms, const struct sys_datetime *datetime)
{
	uint32_t days;

	days = (uint32_t)(datetime->tm_mday - 1);
	*timestamp_ms += days * MS_IN_DAY;
}

static void timestamp_add_hours(int64_t *timestamp_ms, const struct sys_datetime *datetime)
{
	uint32_t hours;

	hours = (uint32_t)datetime->tm_hour;
	*timestamp_ms += hours * MS_IN_HOUR;
}

static void timestamp_add_minutes(int64_t *timestamp_ms, const struct sys_datetime *datetime)
{
	uint16_t minutes;

	minutes = (uint16_t)datetime->tm_min;
	*timestamp_ms += minutes * MS_IN_MINUTE;
}

static void timestamp_add_seconds(int64_t *timestamp_ms, const struct sys_datetime *datetime)
{
	uint16_t seconds;

	seconds = (uint16_t)datetime->tm_sec;
	*timestamp_ms += seconds * MS_IN_SECOND;
}

static void timestamp_add_ms(int64_t *timestamp_ms, const struct sys_datetime *datetime)
{
	uint32_t ns;

	ns = (uint32_t)datetime->tm_nsec;
	*timestamp_ms += ns / NS_IN_MS;
}

static void timestamp_sub_utc_reference(int64_t *timestamp_ms)
{
	*timestamp_ms -= UTC_REFERENCE;
}

static void datetime_init(struct sys_datetime *datetime)
{
	datetime->tm_year = -1900;
	datetime->tm_wday = -1;
	datetime->tm_yday = -1;
	datetime->tm_isdst = -1;
}

static void timestamp_add_utc_reference(int64_t *timestamp_ms)
{
	*timestamp_ms += UTC_REFERENCE;
}

static void datetime_move_minimum_years(struct sys_datetime *datetime, int64_t *timestamp_ms)
{
	*timestamp_ms -= TIMESTAMP_MIN;
	datetime->tm_year += MIN_YEAR;
}

static void datetime_move_years(struct sys_datetime *datetime,
				bool *leap_year,
				int64_t *timestamp_ms)
{
	int difference;
	const struct year_segment *segment;

	*leap_year = false;

	for (uint8_t i = 0; i < ARRAY_SIZE(year_segments); i++) {
		segment = &year_segments[i];

		if (segment->years == 1) {
			*leap_year = !segment->repeats;
		}

		if (*timestamp_ms < segment->ms) {
			continue;
		}

		if (segment->repeats) {
			difference = (int)(*timestamp_ms / segment->ms);
			datetime->tm_year += difference * segment->years;
			*timestamp_ms %= segment->ms;
		} else {
			datetime->tm_year += segment->years;
			*timestamp_ms -= segment->ms;
		}
	}
}

static void datetime_move_months(struct sys_datetime *datetime,
				 bool leap_year,
				 int64_t *timestamp_ms)
{
	uint32_t days;
	uint32_t ms;

	datetime->tm_mon = 0;

	for (uint8_t i = 0; i < ARRAY_SIZE(days_in_month); i++) {
		days = days_in_month[i];

		if (i == 1 && leap_year) {
			days++;
		}

		ms = days * MS_IN_DAY;

		if (*timestamp_ms < ms) {
			return;
		}

		datetime->tm_mon++;
		*timestamp_ms -= ms;
	}
}

static void datetime_move_days(struct sys_datetime *datetime, int64_t *timestamp_ms)
{
	datetime->tm_mday = (int)(1 + (*timestamp_ms / MS_IN_DAY));
	*timestamp_ms %= MS_IN_DAY;
}

static void datetime_move_hours(struct sys_datetime *datetime, int64_t *timestamp_ms)
{
	datetime->tm_hour = (int)(*timestamp_ms / MS_IN_HOUR);
	*timestamp_ms %= MS_IN_HOUR;
}

static void datetime_move_minutes(struct sys_datetime *datetime, int64_t *timestamp_ms)
{
	datetime->tm_min = (int)(*timestamp_ms / MS_IN_MINUTE);
	*timestamp_ms %= MS_IN_MINUTE;
}

static void datetime_move_seconds(struct sys_datetime *datetime, int64_t *timestamp_ms)
{
	datetime->tm_sec = (int)(*timestamp_ms / MS_IN_SECOND);
	*timestamp_ms %= MS_IN_SECOND;
}

static void datetime_move_ms(struct sys_datetime *datetime, int64_t *timestamp_ms)
{
	datetime->tm_nsec = (int)(*timestamp_ms * NS_IN_MS);
	*timestamp_ms = 0;
}

bool sys_realtime_validate_timestamp(const int64_t *timestamp_ms)
{
	return *timestamp_ms >= UTC_MIN && *timestamp_ms < UTC_MAX;
}

bool sys_realtime_validate_datetime(const struct sys_datetime *datetime)
{
	return datetime->tm_year >= (MIN_YEAR - 1900) &&
	       datetime->tm_year <= (MAX_YEAR - 1900) &&
	       datetime->tm_mon <= 11 &&
	       datetime->tm_mday >= 1 &&
	       datetime->tm_mday <= 31 &&
	       datetime->tm_hour >= 0 &&
	       datetime->tm_hour <= 23 &&
	       datetime->tm_min >= 0 &&
	       datetime->tm_min <= 59 &&
	       datetime->tm_sec >= 0 &&
	       datetime->tm_sec <= 59;
}

int sys_realtime_datetime_to_timestamp(int64_t *timestamp_ms, const struct sys_datetime *datetime)
{
	bool leap_year;

	if (!sys_realtime_validate_datetime(datetime)) {
		return -EINVAL;
	}

	*timestamp_ms = 0;
	timestamp_add_years(timestamp_ms, &leap_year, datetime);
	timestamp_add_months(timestamp_ms, leap_year, datetime);
	timestamp_add_days(timestamp_ms, datetime);
	timestamp_add_hours(timestamp_ms, datetime);
	timestamp_add_minutes(timestamp_ms, datetime);
	timestamp_add_seconds(timestamp_ms, datetime);
	timestamp_add_ms(timestamp_ms, datetime);
	timestamp_sub_utc_reference(timestamp_ms);
	return 0;
}

int sys_realtime_timestamp_to_datetime(struct sys_datetime *datetime, const int64_t *timestamp_ms)
{
	bool leap_year;
	int64_t remaining_ms;

	if (!sys_realtime_validate_timestamp(timestamp_ms)) {
		return -EINVAL;
	}

	datetime_init(datetime);

	remaining_ms = *timestamp_ms;
	timestamp_add_utc_reference(&remaining_ms);
	datetime_move_minimum_years(datetime, &remaining_ms);
	datetime_move_years(datetime, &leap_year, &remaining_ms);
	datetime_move_months(datetime, leap_year, &remaining_ms);
	datetime_move_days(datetime, &remaining_ms);
	datetime_move_hours(datetime, &remaining_ms);
	datetime_move_minutes(datetime, &remaining_ms);
	datetime_move_seconds(datetime, &remaining_ms);
	datetime_move_ms(datetime, &remaining_ms);
	return 0;
}
