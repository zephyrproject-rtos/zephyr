/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <zephyr/sys/timeutil.h>

#define SECS_PER_MIN                60
#define SECS_PER_HOUR               3600
#define SECS_PER_DAY                86400
#define DEFAULT_DST_TRANSITION_TIME 7200 /* 02:00 */

/* POSIX default dst rules (US: M3.2.0/2,M11.1.0/2) */
static const struct timeutil_tz_rule default_dst_start = {
	.type = TIMEUTIL_TZ_RULE_MWD,
	.mwd = {.month = 3, .week = 2, .day = 0},
	.time = DEFAULT_DST_TRANSITION_TIME,
};

static const struct timeutil_tz_rule default_dst_end = {
	.type = TIMEUTIL_TZ_RULE_MWD,
	.mwd = {.month = 11, .week = 1, .day = 0},
	.time = DEFAULT_DST_TRANSITION_TIME,
};

/* Days before each month (non-leap year), 0-indexed by month [0..11] */
static const uint16_t days_before_month[12] = {
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334,
};

static bool is_leap_year(int year)
{
	return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/*
 * Day of week for a given date. Returns 0=Sunday..6=Saturday.
 * Uses Tomohiko Sakamoto's algorithm.
 */
static int day_of_week(int year, int month, int day)
{
	static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
	int y = year;

	if (month < 3) {
		y -= 1;
	}
	return (y + y / 4 - y / 100 + y / 400 + t[month - 1] + day) % 7;
}

/**
 * Convert a DST transition rule to a day-of-year (0-based) for a given year.
 * Returns -1 on error.
 */
static int rule_to_yday(const struct timeutil_tz_rule *rule, int year)
{
	switch (rule->type) {
	case TIMEUTIL_TZ_RULE_JULIAN_NOLP: {
		/* Jn: 1-365, Feb 29 is never counted */
		int jd = rule->julian_day; /* 1-based */

		if (jd < 1 || jd > 365) {
			return -1;
		}
		/* Convert to 0-based day-of-year, skipping leap day */
		int yday = jd - 1;

		if (is_leap_year(year) && yday >= 59) {
			/* 59 = March 1 in non-leap, but in leap year March 1 is day 60 */
			yday += 1;
		}
		return yday;
	}
	case TIMEUTIL_TZ_RULE_JULIAN_LP:
		/* n: 0-365, leap day counted */
		if (rule->julian_day > 365) {
			return -1;
		}
		return rule->julian_day;
	case TIMEUTIL_TZ_RULE_MWD: {
		/* Mm.w.d */
		int m = rule->mwd.month; /* 1-12 */
		int w = rule->mwd.week;  /* 1-5 */
		int d = rule->mwd.day;   /* 0-6, 0=Sunday */

		if (m < 1 || m > 12 || w < 1 || w > 5 || d < 0 || d > 6) {
			return -1;
		}

		/* Find first 'd' day-of-week in month m */
		int first_dow = day_of_week(year, m, 1);
		int first_d = 1 + ((d - first_dow + 7) % 7);

		/* Advance to week w */
		int mday = first_d + (w - 1) * 7;

		/* If week=5 means "last", clamp to month */
		int days_in_month_val;

		if (m == 2) {
			days_in_month_val = is_leap_year(year) ? 29 : 28;
		} else if (m == 12) {
			days_in_month_val = 31;
		} else {
			days_in_month_val = days_before_month[m] - days_before_month[m - 1];
		}

		while (mday > days_in_month_val) {
			mday -= 7;
		}

		int yday = days_before_month[m - 1] + mday - 1;

		if (is_leap_year(year) && m > 2) {
			yday += 1;
		}
		return yday;
	}
	default:
		return -1;
	}
}

/**
 * Compute UTC epoch of a DST transition for a given year.
 * wall_offset is the UTC offset (in POSIX sign) that is in effect
 * *before* this transition (the wall clock offset).
 */
static int64_t transition_epoch(const struct timeutil_tz_rule *rule, int year, int32_t wall_offset)
{
	int yday = rule_to_yday(rule, year);

	if (yday < 0) {
		return INT64_MIN;
	}

	/* Build a struct tm for Jan 1 00:00 of this year, then add yday + time */
	struct tm tm_ref = {
		.tm_year = year - 1900,
		.tm_mon = 0,
		.tm_mday = 1,
		.tm_hour = 0,
		.tm_min = 0,
		.tm_sec = 0,
	};
	int64_t jan1_utc = timeutil_timegm64(&tm_ref);

	/* The transition time is wall clock time, so subtract the wall offset
	 * to get UTC. POSIX offset: positive = west = behind UTC.
	 */
	return jan1_utc + (int64_t)yday * SECS_PER_DAY + (int64_t)rule->time + (int64_t)wall_offset;
}

static const char *parse_abbr(const char *p, char *abbr, size_t abbr_size)
{
	size_t len = 0;

	if (*p == '<') {
		/* Quoted form: <ABC+5> */
		p++;
		while (*p != '\0' && *p != '>') {
			if (len < abbr_size - 1) {
				abbr[len++] = *p;
			}
			p++;
		}
		if (*p != '>') {
			return NULL;
		}
		p++; /* skip '>' */
	} else {
		/* Unquoted: at least 3 alpha characters */
		while (isalpha(*p)) {
			if (len < abbr_size - 1) {
				abbr[len++] = *p;
			}
			p++;
		}
		if (len < 3) {
			return NULL;
		}
	}

	abbr[len] = '\0';
	return p;
}

static const char *parse_offset(const char *p, int32_t *offset)
{
	int sign = 1;

	if (*p == '-') {
		sign = -1;
		p++;
	} else if (*p == '+') {
		p++;
	}

	if (!isdigit(*p)) {
		return NULL;
	}

	int hours = 0;

	while (isdigit(*p)) {
		hours = hours * 10 + (*p - '0');
		p++;
	}

	int minutes = 0;
	int seconds = 0;

	if (*p == ':') {
		p++;
		while (isdigit(*p)) {
			minutes = minutes * 10 + (*p - '0');
			p++;
		}
		if (*p == ':') {
			p++;
			while (isdigit(*p)) {
				seconds = seconds * 10 + (*p - '0');
				p++;
			}
		}
	}

	*offset = sign * (hours * SECS_PER_HOUR + minutes * SECS_PER_MIN + seconds);
	return p;
}

static const char *parse_time(const char *p, int32_t *time_out)
{
	if (*p != '/') {
		*time_out = DEFAULT_DST_TRANSITION_TIME;
		return p;
	}
	p++; /* skip '/' */

	return parse_offset(p, time_out);
}

static const char *parse_rule(const char *p, struct timeutil_tz_rule *rule)
{
	if (*p == 'M') {
		/* Mm.w.d */
		p++;
		rule->type = TIMEUTIL_TZ_RULE_MWD;

		int month = 0;

		while (isdigit(*p)) {
			month = month * 10 + (*p - '0');
			p++;
		}
		if (*p != '.') {
			return NULL;
		}
		p++;

		int week = 0;

		while (isdigit(*p)) {
			week = week * 10 + (*p - '0');
			p++;
		}
		if (*p != '.') {
			return NULL;
		}
		p++;

		int day = 0;

		while (isdigit(*p)) {
			day = day * 10 + (*p - '0');
			p++;
		}

		rule->mwd.month = (uint8_t)month;
		rule->mwd.week = (uint8_t)week;
		rule->mwd.day = (uint8_t)day;
	} else if (*p == 'J') {
		/* Jn */
		p++;
		rule->type = TIMEUTIL_TZ_RULE_JULIAN_NOLP;

		int jday = 0;

		while (isdigit(*p)) {
			jday = jday * 10 + (*p - '0');
			p++;
		}
		rule->julian_day = (uint16_t)jday;
	} else if (isdigit(*p)) {
		/* n */
		rule->type = TIMEUTIL_TZ_RULE_JULIAN_LP;

		int jday = 0;

		while (isdigit(*p)) {
			jday = jday * 10 + (*p - '0');
			p++;
		}
		rule->julian_day = (uint16_t)jday;
	} else {
		return NULL;
	}

	return parse_time(p, &rule->time);
}

int timeutil_tz_from_posix(const char *tz_str, struct timeutil_tz_info *tz)
{
	memset(tz, 0, sizeof(*tz));

	const char *p = tz_str;

	/* Parse standard abbreviation */
	p = parse_abbr(p, tz->std_abbr, sizeof(tz->std_abbr));
	if (p == NULL) {
		return -EINVAL;
	}

	/* Parse standard offset (required) */
	p = parse_offset(p, &tz->std_offset);
	if (p == NULL) {
		return -EINVAL;
	}

	/* If nothing more, no DST */
	if (*p == '\0') {
		tz->has_dst = false;
		return 0;
	}

	/* Parse DST abbreviation */
	tz->has_dst = true;
	p = parse_abbr(p, tz->dst_abbr, sizeof(tz->dst_abbr));
	if (p == NULL) {
		return -EINVAL;
	}

	/* DST offset is optional; defaults to std_offset - 3600 */
	if (*p == ',' || *p == '\0') {
		tz->dst_offset = tz->std_offset - SECS_PER_HOUR;
	} else {
		p = parse_offset(p, &tz->dst_offset);
		if (p == NULL) {
			return -EINVAL;
		}
	}

	/* Parse transition rules, or use default */
	if (*p == '\0') {
		memcpy(&tz->dst_start, &default_dst_start, sizeof(struct timeutil_tz_rule));
		memcpy(&tz->dst_end, &default_dst_end, sizeof(struct timeutil_tz_rule));
		return 0;
	}

	if (*p != ',') {
		return -EINVAL;
	}
	p++;

	p = parse_rule(p, &tz->dst_start);
	if (p == NULL) {
		return -EINVAL;
	}

	if (*p == '\0') {
		/* Only start rule given; use default end */
		memcpy(&tz->dst_end, &default_dst_end, sizeof(struct timeutil_tz_rule));
		return 0;
	}

	if (*p != ',') {
		return -EINVAL;
	}
	p++;

	p = parse_rule(p, &tz->dst_end);
	if (p == NULL) {
		return -EINVAL;
	}

	/* Must have consumed entire string */
	if (*p != '\0') {
		return -EINVAL;
	}

	return 0;
}

/**
 * Break a UTC epoch into struct tm (similar to gmtime_r).
 * Uses the same civil date algorithm as timeutil.c.
 */
static int epoch_to_tm(int64_t epoch, struct tm *tp)
{
	int64_t secs = epoch;
	int64_t days = secs / SECS_PER_DAY;
	int32_t rem = (int32_t)(secs - days * SECS_PER_DAY);

	if (rem < 0) {
		rem += SECS_PER_DAY;
		days -= 1;
	}

	tp->tm_hour = rem / SECS_PER_HOUR;
	rem %= SECS_PER_HOUR;
	tp->tm_min = rem / SECS_PER_MIN;
	tp->tm_sec = rem % SECS_PER_MIN;

	/* days since epoch (1970-01-01 was a Thursday = wday 4) */
	int wday = (int)((days + 4) % 7);

	if (wday < 0) {
		wday += 7;
	}
	tp->tm_wday = wday;

	/* Convert days from epoch to civil date using Howard Hinnant's algorithm */
	int64_t z = days + 719468;
	int64_t era = ((z >= 0) ? z : (z - 146096)) / 146097;
	unsigned int doe = (unsigned int)(z - era * 146097);
	unsigned int yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	int64_t y = (int64_t)yoe + era * 400;
	unsigned int doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	unsigned int mp = (5 * doy + 2) / 153;
	unsigned int d = doy - (153 * mp + 2) / 5 + 1;
	unsigned int m = mp + ((mp < 10) ? 3 : -9);

	if (m <= 2) {
		y++;
	}

	int64_t tm_year = y - 1900;

	if (tm_year > INT_MAX || tm_year < INT_MIN) {
		return -EOVERFLOW;
	}

	tp->tm_year = (int)tm_year;
	tp->tm_mon = (int)(m - 1);
	tp->tm_mday = (int)d;

	/* Compute yday */
	int yday = (int)days_before_month[tp->tm_mon] + tp->tm_mday - 1;

	if (is_leap_year((int)y) && tp->tm_mon >= 2) {
		yday += 1;
	}
	tp->tm_yday = yday;

	return 0;
}

/**
 * Determine whether a given UTC epoch falls in DST for the given timezone.
 * Returns 1 if DST, 0 if standard time, -1 on error.
 */
static int is_dst_at(const struct timeutil_tz_info *tz, int64_t utc_epoch)
{
	if (!tz->has_dst) {
		return 0;
	}

	/* Determine the year from the UTC time */
	struct tm tmp;

	if (epoch_to_tm(utc_epoch, &tmp) != 0) {
		return -1;
	}
	int year = tmp.tm_year + 1900;

	/* DST start transition: wall clock is in standard time before this,
	 * so the wall offset is std_offset.
	 */
	int64_t start_utc = transition_epoch(&tz->dst_start, year, tz->std_offset);
	/* DST end transition: wall clock is in DST before this,
	 * so the wall offset is dst_offset.
	 */
	int64_t end_utc = transition_epoch(&tz->dst_end, year, tz->dst_offset);

	if (start_utc == INT64_MIN || end_utc == INT64_MIN) {
		return -1;
	}

	if (start_utc < end_utc) {
		/* Northern hemisphere: DST between start and end */
		return (utc_epoch >= start_utc && utc_epoch < end_utc) ? 1 : 0;
	}
	/* Southern hemisphere: DST outside [end, start) */
	return (utc_epoch < end_utc || utc_epoch >= start_utc) ? 1 : 0;
}

int timeutil_tz_utc_to_local(const struct timeutil_tz_info *tz, int64_t epoch, struct tm *tp)
{
	int dst = is_dst_at(tz, epoch);

	if (dst < 0) {
		return -EINVAL;
	}

	/* POSIX offset: positive = west = behind UTC.
	 * Local time = UTC - offset.
	 */
	int32_t offset = dst ? tz->dst_offset : tz->std_offset;
	int64_t local_epoch = epoch - (int64_t)offset;
	int ret = epoch_to_tm(local_epoch, tp);

	if (ret != 0) {
		return ret;
	}

	tp->tm_isdst = dst;

	return 0;
}

int timeutil_tz_local_to_utc(const struct timeutil_tz_info *tz, struct tm *tp, int64_t *epoch)
{
	/* Convert local tp to a "local epoch" (as if it were UTC) */
	int64_t local_epoch = timeutil_timegm64(tp);

	if (!tz->has_dst) {
		/* No DST: UTC = local + std_offset */
		*epoch = local_epoch + (int64_t)tz->std_offset;
		/* Normalize tp */
		int ret = epoch_to_tm(local_epoch, tp);

		if (ret != 0) {
			return ret;
		}
		tp->tm_isdst = 0;
		return 0;
	}

	/* Try both possible offsets and use tm_isdst as hint */
	int64_t utc_std = local_epoch + (int64_t)tz->std_offset;
	int64_t utc_dst = local_epoch + (int64_t)tz->dst_offset;

	int dst_if_std = is_dst_at(tz, utc_std);
	int dst_if_dst = is_dst_at(tz, utc_dst);

	if (dst_if_std < 0 || dst_if_dst < 0) {
		return -EINVAL;
	}

	int64_t utc;
	int isdst;

	if (dst_if_std == 0 && dst_if_dst == 1) {
		/* Ambiguous: both interpretations are valid (fall-back).
		 * Use tm_isdst as hint.
		 */
		if (tp->tm_isdst > 0) {
			utc = utc_dst;
			isdst = 1;
		} else {
			utc = utc_std;
			isdst = 0;
		}
	} else if (dst_if_std == 0) {
		utc = utc_std;
		isdst = 0;
	} else if (dst_if_dst == 1) {
		utc = utc_dst;
		isdst = 1;
	} else {
		/* Gap (spring-forward): neither offset is valid for this local time.
		 * Use standard time assumption.
		 */
		utc = utc_std;
		isdst = is_dst_at(tz, utc_std);
		if (isdst < 0) {
			return -EINVAL;
		}
	}

	*epoch = utc;

	/* Normalize: convert UTC back to local to fill in correct tp */
	int32_t offset = isdst ? tz->dst_offset : tz->std_offset;
	int ret = epoch_to_tm(utc - (int64_t)offset, tp);

	if (ret != 0) {
		return ret;
	}

	tp->tm_isdst = isdst;

	return 0;
}
