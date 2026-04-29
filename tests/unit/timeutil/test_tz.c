/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/timeutil.h>

#include "../../../lib/utils/timeutil_tz.c"

static struct tm make_tm(int year, int mon, int mday, int hour, int min, int sec, int isdst)
{
	struct tm t = {
		.tm_year = year - 1900,
		.tm_mon = mon - 1,
		.tm_mday = mday,
		.tm_hour = hour,
		.tm_min = min,
		.tm_sec = sec,
		.tm_isdst = isdst,
	};
	return t;
}

ZTEST(timeutil_tz, test_parse_utc_only)
{
	struct timeutil_tz_info tz;
	int ret = timeutil_tz_from_posix("UTC0", &tz);

	zassert_ok(ret, "parse UTC0 failed: %d", ret);
	zassert_str_equal(tz.std_abbr, "UTC");
	zassert_equal(tz.std_offset, 0);
	zassert_false(tz.has_dst);
}

ZTEST(timeutil_tz, test_parse_fixed_offset_west)
{
	struct timeutil_tz_info tz;
	int ret = timeutil_tz_from_posix("EST5", &tz);

	zassert_ok(ret, "parse EST5 failed: %d", ret);
	zassert_str_equal(tz.std_abbr, "EST");
	zassert_equal(tz.std_offset, 5 * 3600);
	zassert_false(tz.has_dst);
}

ZTEST(timeutil_tz, test_parse_fixed_offset_east)
{
	struct timeutil_tz_info tz;
	int ret = timeutil_tz_from_posix("IST-5:30", &tz);

	zassert_ok(ret, "parse IST-5:30 failed: %d", ret);
	zassert_str_equal(tz.std_abbr, "IST");
	zassert_equal(tz.std_offset, -(5 * 3600 + 30 * 60));
	zassert_false(tz.has_dst);
}

ZTEST(timeutil_tz, test_parse_us_eastern)
{
	struct timeutil_tz_info tz;
	int ret = timeutil_tz_from_posix("EST5EDT,M3.2.0/2,M11.1.0/2", &tz);

	zassert_ok(ret, "parse US Eastern failed: %d", ret);
	zassert_str_equal(tz.std_abbr, "EST");
	zassert_equal(tz.std_offset, 5 * 3600);
	zassert_true(tz.has_dst);
	zassert_str_equal(tz.dst_abbr, "EDT");
	zassert_equal(tz.dst_offset, 4 * 3600); /* default: std - 1h */
	zassert_equal(tz.dst_start.type, TIMEUTIL_TZ_RULE_MWD);
	zassert_equal(tz.dst_start.mwd.month, 3);
	zassert_equal(tz.dst_start.mwd.week, 2);
	zassert_equal(tz.dst_start.mwd.day, 0);
	zassert_equal(tz.dst_start.time, 7200);
	zassert_equal(tz.dst_end.mwd.month, 11);
	zassert_equal(tz.dst_end.mwd.week, 1);
	zassert_equal(tz.dst_end.mwd.day, 0);
	zassert_equal(tz.dst_end.time, 7200);
}

ZTEST(timeutil_tz, test_parse_cet)
{
	struct timeutil_tz_info tz;
	int ret = timeutil_tz_from_posix("CET-1CEST,M3.5.0/2,M10.5.0/3", &tz);

	zassert_ok(ret, "parse CET failed: %d", ret);
	zassert_str_equal(tz.std_abbr, "CET");
	zassert_equal(tz.std_offset, -3600);
	zassert_true(tz.has_dst);
	zassert_str_equal(tz.dst_abbr, "CEST");
	zassert_equal(tz.dst_offset, -7200); /* default: std - 1h */
	zassert_equal(tz.dst_end.time, 10800); /* /3 = 03:00 */
}

ZTEST(timeutil_tz, test_parse_quoted_abbr)
{
	struct timeutil_tz_info tz;
	int ret = timeutil_tz_from_posix("<+05>-5", &tz);

	zassert_ok(ret, "parse quoted abbr failed: %d", ret);
	zassert_str_equal(tz.std_abbr, "+05");
	zassert_equal(tz.std_offset, -5 * 3600);
	zassert_false(tz.has_dst);
}

ZTEST(timeutil_tz, test_parse_julian_rules)
{
	struct timeutil_tz_info tz;
	int ret = timeutil_tz_from_posix("XXX3YYY,J80/2,J300/3", &tz);

	zassert_ok(ret, "parse Julian rules failed: %d", ret);
	zassert_true(tz.has_dst);
	zassert_equal(tz.dst_start.type, TIMEUTIL_TZ_RULE_JULIAN_NOLP);
	zassert_equal(tz.dst_start.julian_day, 80);
	zassert_equal(tz.dst_start.time, 7200);
	zassert_equal(tz.dst_end.type, TIMEUTIL_TZ_RULE_JULIAN_NOLP);
	zassert_equal(tz.dst_end.julian_day, 300);
	zassert_equal(tz.dst_end.time, 10800);
}

ZTEST(timeutil_tz, test_parse_numeric_julian_rules)
{
	struct timeutil_tz_info tz;
	int ret = timeutil_tz_from_posix("XXX3YYY,79/2,299/3", &tz);

	zassert_ok(ret, "parse numeric Julian rules failed: %d", ret);
	zassert_true(tz.has_dst);
	zassert_equal(tz.dst_start.type, TIMEUTIL_TZ_RULE_JULIAN_LP);
	zassert_equal(tz.dst_start.julian_day, 79);
	zassert_equal(tz.dst_end.type, TIMEUTIL_TZ_RULE_JULIAN_LP);
	zassert_equal(tz.dst_end.julian_day, 299);
}

ZTEST(timeutil_tz, test_parse_malformed_strings)
{
	struct timeutil_tz_info tz;

	/* Empty string */
	zassert_equal(timeutil_tz_from_posix("", &tz), -EINVAL);
	/* Missing offset */
	zassert_equal(timeutil_tz_from_posix("EST", &tz), -EINVAL);
	/* Trailing garbage */
	zassert_equal(timeutil_tz_from_posix("EST5EDT,M3.2.0/2,M11.1.0/2extra", &tz), -EINVAL);
}

ZTEST(timeutil_tz, test_utc_to_local_utc_zone)
{
	struct timeutil_tz_info tz;
	struct tm tp;

	timeutil_tz_from_posix("UTC0", &tz);

	/* 2024-06-15 12:30:00 UTC = epoch 1718454600 */
	int ret = timeutil_tz_utc_to_local(&tz, 1718454600, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_year, 2024 - 1900);
	zassert_equal(tp.tm_mon, 5); /* June = 5 */
	zassert_equal(tp.tm_mday, 15);
	zassert_equal(tp.tm_hour, 12);
	zassert_equal(tp.tm_min, 30);
	zassert_equal(tp.tm_sec, 0);
	zassert_equal(tp.tm_isdst, 0);
}

ZTEST(timeutil_tz, test_utc_to_local_fixed_offset)
{
	struct timeutil_tz_info tz;
	struct tm tp;

	/* IST = UTC+5:30, POSIX offset = -5:30 */
	timeutil_tz_from_posix("IST-5:30", &tz);

	/* 2024-01-01 00:00:00 UTC -> 2024-01-01 05:30:00 IST */
	int ret = timeutil_tz_utc_to_local(&tz, 1704067200, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_year, 2024 - 1900);
	zassert_equal(tp.tm_mon, 0);
	zassert_equal(tp.tm_mday, 1);
	zassert_equal(tp.tm_hour, 5);
	zassert_equal(tp.tm_min, 30);
	zassert_equal(tp.tm_isdst, 0);
}

ZTEST(timeutil_tz, test_utc_to_local_us_eastern_winter)
{
	struct timeutil_tz_info tz;
	struct tm tp;

	timeutil_tz_from_posix("EST5EDT,M3.2.0/2,M11.1.0/2", &tz);

	/* 2024-01-15 17:00:00 UTC = 2024-01-15 12:00:00 EST */
	int ret = timeutil_tz_utc_to_local(&tz, 1705338000, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 12);
	zassert_equal(tp.tm_isdst, 0);
}

ZTEST(timeutil_tz, test_utc_to_local_us_eastern_summer)
{
	struct timeutil_tz_info tz;
	struct tm tp;

	timeutil_tz_from_posix("EST5EDT,M3.2.0/2,M11.1.0/2", &tz);

	/* 2024-06-15 17:00:00 UTC = 2024-06-15 13:00:00 EDT */
	int ret = timeutil_tz_utc_to_local(&tz, 1718470800, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 13);
	zassert_equal(tp.tm_isdst, 1);
}

ZTEST(timeutil_tz, test_utc_to_local_cet_summer)
{
	struct timeutil_tz_info tz;
	struct tm tp;

	timeutil_tz_from_posix("CET-1CEST,M3.5.0/2,M10.5.0/3", &tz);

	/* 2024-07-01 12:00:00 UTC = 2024-07-01 14:00:00 CEST */
	int ret = timeutil_tz_utc_to_local(&tz, 1719835200, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 14);
	zassert_equal(tp.tm_isdst, 1);
}

ZTEST(timeutil_tz, test_utc_to_local_cet_winter)
{
	struct timeutil_tz_info tz;
	struct tm tp;

	timeutil_tz_from_posix("CET-1CEST,M3.5.0/2,M10.5.0/3", &tz);

	/* 2024-12-01 12:00:00 UTC = 2024-12-01 13:00:00 CET */
	int ret = timeutil_tz_utc_to_local(&tz, 1733054400, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 13);
	zassert_equal(tp.tm_isdst, 0);
}

ZTEST(timeutil_tz, test_spring_forward_boundary)
{
	struct timeutil_tz_info tz;
	struct tm tp;

	timeutil_tz_from_posix("EST5EDT,M3.2.0/2,M11.1.0/2", &tz);

	/* 2024-03-10: DST starts at 02:00 EST = 07:00 UTC
	 * One second before: 06:59:59 UTC = 01:59:59 EST (standard)
	 */
	int ret = timeutil_tz_utc_to_local(&tz, 1710054000 - 1, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 1);
	zassert_equal(tp.tm_min, 59);
	zassert_equal(tp.tm_sec, 59);
	zassert_equal(tp.tm_isdst, 0);

	/* At 07:00:00 UTC = 03:00:00 EDT (DST) */
	ret = timeutil_tz_utc_to_local(&tz, 1710054000, &tp);
	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 3);
	zassert_equal(tp.tm_min, 0);
	zassert_equal(tp.tm_isdst, 1);
}

ZTEST(timeutil_tz, test_fall_back_boundary)
{
	struct timeutil_tz_info tz;
	struct tm tp;

	timeutil_tz_from_posix("EST5EDT,M3.2.0/2,M11.1.0/2", &tz);

	/* 2024-11-03: DST ends at 02:00 EDT = 06:00 UTC
	 * One second before: 05:59:59 UTC = 01:59:59 EDT (DST)
	 */
	int ret = timeutil_tz_utc_to_local(&tz, 1730613600 - 1, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 1);
	zassert_equal(tp.tm_min, 59);
	zassert_equal(tp.tm_sec, 59);
	zassert_equal(tp.tm_isdst, 1);

	/* At 06:00:00 UTC = 01:00:00 EST (standard) */
	ret = timeutil_tz_utc_to_local(&tz, 1730613600, &tp);
	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 1);
	zassert_equal(tp.tm_min, 0);
	zassert_equal(tp.tm_isdst, 0);
}

ZTEST(timeutil_tz, test_southern_hemisphere)
{
	struct timeutil_tz_info tz;
	struct tm tp;

	/* NZST/NZDT: UTC+12/+13, DST starts last Sunday September, ends first Sunday April */
	timeutil_tz_from_posix("NZST-12NZDT,M9.5.0/2,M4.1.0/3", &tz);

	/* 2024-01-15 00:00:00 UTC -> should be NZDT (summer in southern hemisphere)
	 * = 2024-01-15 13:00:00 NZDT
	 */
	int ret = timeutil_tz_utc_to_local(&tz, 1705276800, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 13);
	zassert_equal(tp.tm_isdst, 1);

	/* 2024-07-15 00:00:00 UTC -> should be NZST (winter in southern hemisphere)
	 * = 2024-07-15 12:00:00 NZST
	 */
	ret = timeutil_tz_utc_to_local(&tz, 1721001600, &tp);
	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 12);
	zassert_equal(tp.tm_isdst, 0);
}

ZTEST(timeutil_tz, test_local_to_utc_simple)
{
	struct timeutil_tz_info tz;
	struct tm tp;
	int64_t utc;

	timeutil_tz_from_posix("UTC0", &tz);
	tp = make_tm(2024, 6, 15, 12, 30, 0, 0);

	int ret = timeutil_tz_local_to_utc(&tz, &tp, &utc);

	zassert_ok(ret);
	zassert_equal(utc, 1718454600);
}

ZTEST(timeutil_tz, test_local_to_utc_fixed_offset)
{
	struct timeutil_tz_info tz;
	struct tm tp;
	int64_t utc;

	timeutil_tz_from_posix("EST5", &tz);
	tp = make_tm(2024, 1, 15, 12, 0, 0, 0);

	int ret = timeutil_tz_local_to_utc(&tz, &tp, &utc);

	zassert_ok(ret);
	/* 2024-01-15 12:00:00 EST = 2024-01-15 17:00:00 UTC */
	zassert_equal(utc, 1705338000);
}

ZTEST(timeutil_tz, test_local_to_utc_dst)
{
	struct timeutil_tz_info tz;
	struct tm tp;
	int64_t utc;

	timeutil_tz_from_posix("EST5EDT,M3.2.0/2,M11.1.0/2", &tz);

	/* Summer: 2024-06-15 13:00:00 EDT */
	tp = make_tm(2024, 6, 15, 13, 0, 0, 1);
	int ret = timeutil_tz_local_to_utc(&tz, &tp, &utc);

	zassert_ok(ret);
	/* Should be 17:00:00 UTC */
	zassert_equal(utc, 1718470800);
}

ZTEST(timeutil_tz, test_local_to_utc_ambiguous_fallback)
{
	struct timeutil_tz_info tz;
	struct tm tp;
	int64_t utc;

	timeutil_tz_from_posix("EST5EDT,M3.2.0/2,M11.1.0/2", &tz);

	/* 2024-11-03 01:30:00 — this time occurs twice during fall-back.
	 * With tm_isdst=1 (EDT): UTC = 01:30 + 4h = 05:30 UTC
	 * With tm_isdst=0 (EST): UTC = 01:30 + 5h = 06:30 UTC
	 */
	tp = make_tm(2024, 11, 3, 1, 30, 0, 1);
	int ret = timeutil_tz_local_to_utc(&tz, &tp, &utc);

	zassert_ok(ret);
	/* EDT interpretation: 05:30 UTC */
	struct tm check;

	epoch_to_tm(utc, &check);
	zassert_equal(check.tm_hour, 5);
	zassert_equal(check.tm_min, 30);

	/* Now with tm_isdst=0 (EST) */
	tp = make_tm(2024, 11, 3, 1, 30, 0, 0);
	ret = timeutil_tz_local_to_utc(&tz, &tp, &utc);
	zassert_ok(ret);
	/* EST interpretation: 06:30 UTC */
	epoch_to_tm(utc, &check);
	zassert_equal(check.tm_hour, 6);
	zassert_equal(check.tm_min, 30);
}

ZTEST(timeutil_tz, test_roundtrip_utc_local_utc)
{
	struct timeutil_tz_info tz;
	struct tm tp;
	int64_t original_utc = 1718470800; /* 2024-06-15 17:00:00 UTC */
	int64_t recovered_utc;

	timeutil_tz_from_posix("EST5EDT,M3.2.0/2,M11.1.0/2", &tz);

	/* UTC -> local */
	int ret = timeutil_tz_utc_to_local(&tz, original_utc, &tp);

	zassert_ok(ret);

	/* local -> UTC */
	ret = timeutil_tz_local_to_utc(&tz, &tp, &recovered_utc);
	zassert_ok(ret);
	zassert_equal(original_utc, recovered_utc,
		      "round-trip failed: %" PRId64 " != %" PRId64,
		      original_utc, recovered_utc);
}

ZTEST(timeutil_tz, test_roundtrip_winter)
{
	struct timeutil_tz_info tz;
	struct tm tp;
	int64_t original_utc = 1705338000; /* 2024-01-15 17:00:00 UTC */
	int64_t recovered_utc;

	timeutil_tz_from_posix("EST5EDT,M3.2.0/2,M11.1.0/2", &tz);

	int ret = timeutil_tz_utc_to_local(&tz, original_utc, &tp);

	zassert_ok(ret);

	ret = timeutil_tz_local_to_utc(&tz, &tp, &recovered_utc);
	zassert_ok(ret);
	zassert_equal(original_utc, recovered_utc);
}

ZTEST(timeutil_tz, test_roundtrip_cet)
{
	struct timeutil_tz_info tz;
	struct tm tp;
	int64_t original_utc = 1719835200; /* 2024-07-01 12:00:00 UTC */
	int64_t recovered_utc;

	timeutil_tz_from_posix("CET-1CEST,M3.5.0/2,M10.5.0/3", &tz);

	int ret = timeutil_tz_utc_to_local(&tz, original_utc, &tp);

	zassert_ok(ret);

	ret = timeutil_tz_local_to_utc(&tz, &tp, &recovered_utc);
	zassert_ok(ret);
	zassert_equal(original_utc, recovered_utc);
}

ZTEST(timeutil_tz, test_apply_us_eastern_summer)
{
	struct timeutil_tz_info tz;

	timeutil_tz_from_posix("EST5EDT,M3.2.0/2,M11.1.0/2", &tz);

	/* 2024-06-15 17:00:00 UTC -> 13:00:00 EDT */
	struct tm tp = make_tm(2024, 6, 15, 17, 0, 0, 0);
	int ret = timeutil_tz_apply(&tz, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 13);
	zassert_equal(tp.tm_isdst, 1);
}

ZTEST(timeutil_tz, test_apply_us_eastern_winter)
{
	struct timeutil_tz_info tz;

	timeutil_tz_from_posix("EST5EDT,M3.2.0/2,M11.1.0/2", &tz);

	/* 2024-01-15 17:00:00 UTC -> 12:00:00 EST */
	struct tm tp = make_tm(2024, 1, 15, 17, 0, 0, 0);
	int ret = timeutil_tz_apply(&tz, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 12);
	zassert_equal(tp.tm_isdst, 0);
}

ZTEST(timeutil_tz, test_apply_utc)
{
	struct timeutil_tz_info tz;

	timeutil_tz_from_posix("UTC0", &tz);

	struct tm tp = make_tm(2024, 6, 15, 12, 30, 0, 0);
	int ret = timeutil_tz_apply(&tz, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 12);
	zassert_equal(tp.tm_min, 30);
	zassert_equal(tp.tm_isdst, 0);
}


ZTEST(timeutil_tz, test_cet_spring_forward_boundary)
{
	struct timeutil_tz_info tz;
	struct tm tp;

	timeutil_tz_from_posix("CET-1CEST,M3.5.0/2,M10.5.0/3", &tz);

	/* 2024-03-31: CET→CEST at 02:00 CET = 01:00 UTC
	 * One second before: 00:59:59 UTC = 01:59:59 CET (standard)
	 */
	int ret = timeutil_tz_utc_to_local(&tz, 1711846800 - 1, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 1);
	zassert_equal(tp.tm_min, 59);
	zassert_equal(tp.tm_sec, 59);
	zassert_equal(tp.tm_isdst, 0);

	/* At 01:00:00 UTC = 03:00:00 CEST (DST, clock jumps from 02:00 to 03:00) */
	ret = timeutil_tz_utc_to_local(&tz, 1711846800, &tp);
	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 3);
	zassert_equal(tp.tm_min, 0);
	zassert_equal(tp.tm_isdst, 1);
}

ZTEST(timeutil_tz, test_cet_fall_back_boundary)
{
	struct timeutil_tz_info tz;
	struct tm tp;

	timeutil_tz_from_posix("CET-1CEST,M3.5.0/2,M10.5.0/3", &tz);

	/* 2024-10-27: CEST→CET at 03:00 CEST = 01:00 UTC
	 * One second before: 00:59:59 UTC = 02:59:59 CEST (DST)
	 */
	int ret = timeutil_tz_utc_to_local(&tz, 1729990800 - 1, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 2);
	zassert_equal(tp.tm_min, 59);
	zassert_equal(tp.tm_sec, 59);
	zassert_equal(tp.tm_isdst, 1);

	/* At 01:00:00 UTC = 02:00:00 CET (standard, clock falls back from 03:00 to 02:00) */
	ret = timeutil_tz_utc_to_local(&tz, 1729990800, &tp);
	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 2);
	zassert_equal(tp.tm_min, 0);
	zassert_equal(tp.tm_isdst, 0);
}

ZTEST(timeutil_tz, test_cet_ambiguous_fallback)
{
	struct timeutil_tz_info tz;
	struct tm tp;
	int64_t utc;

	timeutil_tz_from_posix("CET-1CEST,M3.5.0/2,M10.5.0/3", &tz);

	/* 2024-10-27 02:30:00 local occurs twice:
	 * As CEST (DST): UTC = 02:30 - 2h = 00:30 UTC
	 * As CET (std):  UTC = 02:30 - 1h = 01:30 UTC
	 */
	tp = make_tm(2024, 10, 27, 2, 30, 0, 1);
	int ret = timeutil_tz_local_to_utc(&tz, &tp, &utc);

	zassert_ok(ret);
	struct tm check;

	epoch_to_tm(utc, &check);
	zassert_equal(check.tm_hour, 0);
	zassert_equal(check.tm_min, 30);

	tp = make_tm(2024, 10, 27, 2, 30, 0, 0);
	ret = timeutil_tz_local_to_utc(&tz, &tp, &utc);
	zassert_ok(ret);
	epoch_to_tm(utc, &check);
	zassert_equal(check.tm_hour, 1);
	zassert_equal(check.tm_min, 30);
}

ZTEST(timeutil_tz, test_spring_forward_gap)
{
	struct timeutil_tz_info tz;
	struct tm tp;
	int64_t utc;
	int64_t recovered_utc;

	timeutil_tz_from_posix("EST5EDT,M3.2.0/2,M11.1.0/2", &tz);

	/* 2024-03-10 02:30:00 EST doesn't exist — clocks skip from 02:00 to 03:00.
	 * local_to_utc should still succeed (standard-time assumption) and
	 * the round-trip through utc_to_local should produce a valid, consistent time.
	 */
	tp = make_tm(2024, 3, 10, 2, 30, 0, 0);
	int ret = timeutil_tz_local_to_utc(&tz, &tp, &utc);

	zassert_ok(ret);

	/* The result should round-trip cleanly */
	struct tm rt;

	ret = timeutil_tz_utc_to_local(&tz, utc, &rt);
	zassert_ok(ret);
	ret = timeutil_tz_local_to_utc(&tz, &rt, &recovered_utc);
	zassert_ok(ret);
	zassert_equal(utc, recovered_utc, "gap round-trip failed");
}

ZTEST(timeutil_tz, test_nz_spring_forward_boundary)
{
	struct timeutil_tz_info tz;
	struct tm tp;

	timeutil_tz_from_posix("NZST-12NZDT,M9.5.0/2,M4.1.0/3", &tz);

	/* 2024-09-29: NZST→NZDT at 02:00 NZST = 14:00 UTC Sept 28
	 * One second before: 13:59:59 UTC Sep 28 = 01:59:59 NZST (standard)
	 */
	int ret = timeutil_tz_utc_to_local(&tz, 1727532000 - 1, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 1);
	zassert_equal(tp.tm_min, 59);
	zassert_equal(tp.tm_sec, 59);
	zassert_equal(tp.tm_isdst, 0);
	zassert_equal(tp.tm_mday, 29);

	/* At 14:00:00 UTC Sep 28 = 03:00:00 NZDT Sep 29 (clock jumps 02:00→03:00) */
	ret = timeutil_tz_utc_to_local(&tz, 1727532000, &tp);
	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 3);
	zassert_equal(tp.tm_min, 0);
	zassert_equal(tp.tm_isdst, 1);
	zassert_equal(tp.tm_mday, 29);
}

ZTEST(timeutil_tz, test_nz_fall_back_boundary)
{
	struct timeutil_tz_info tz;
	struct tm tp;

	timeutil_tz_from_posix("NZST-12NZDT,M9.5.0/2,M4.1.0/3", &tz);

	/* 2024-04-07: NZDT→NZST at 03:00 NZDT = 14:00 UTC April 6
	 * One second before: 13:59:59 UTC Apr 6 = 02:59:59 NZDT (DST)
	 */
	int ret = timeutil_tz_utc_to_local(&tz, 1712412000 - 1, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 2);
	zassert_equal(tp.tm_min, 59);
	zassert_equal(tp.tm_sec, 59);
	zassert_equal(tp.tm_isdst, 1);

	/* At 14:00:00 UTC Apr 6 = 02:00:00 NZST (standard, clock falls back 03:00→02:00) */
	ret = timeutil_tz_utc_to_local(&tz, 1712412000, &tp);
	zassert_ok(ret);
	zassert_equal(tp.tm_hour, 2);
	zassert_equal(tp.tm_min, 0);
	zassert_equal(tp.tm_isdst, 0);
}

ZTEST(timeutil_tz, test_date_crosses_midnight)
{
	struct timeutil_tz_info tz;
	struct tm tp;

	timeutil_tz_from_posix("EST5EDT,M3.2.0/2,M11.1.0/2", &tz);

	/* 2024-06-15 03:00:00 UTC = 2024-06-14 23:00:00 EDT (previous day) */
	int ret = timeutil_tz_utc_to_local(&tz, 1718420400, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_mday, 14);
	zassert_equal(tp.tm_hour, 23);
	zassert_equal(tp.tm_isdst, 1);
}

ZTEST(timeutil_tz, test_date_crosses_year_boundary)
{
	struct timeutil_tz_info tz;
	struct tm tp;

	timeutil_tz_from_posix("NZST-12NZDT,M9.5.0/2,M4.1.0/3", &tz);

	/* 2023-12-31 12:00:00 UTC = 2024-01-01 01:00:00 NZDT (crosses year boundary) */
	int ret = timeutil_tz_utc_to_local(&tz, 1704024000, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_year, 2024 - 1900);
	zassert_equal(tp.tm_mon, 0);
	zassert_equal(tp.tm_mday, 1);
	zassert_equal(tp.tm_hour, 1);
	zassert_equal(tp.tm_isdst, 1);
}

ZTEST(timeutil_tz, test_roundtrip_at_spring_forward)
{
	struct timeutil_tz_info tz;
	struct tm tp;
	int64_t original_utc;
	int64_t recovered_utc;

	timeutil_tz_from_posix("EST5EDT,M3.2.0/2,M11.1.0/2", &tz);

	/* Exactly at the spring-forward transition: 2024-03-10 07:00:00 UTC */
	original_utc = 1710054000;
	int ret = timeutil_tz_utc_to_local(&tz, original_utc, &tp);

	zassert_ok(ret);
	ret = timeutil_tz_local_to_utc(&tz, &tp, &recovered_utc);
	zassert_ok(ret);
	zassert_equal(original_utc, recovered_utc);
}

ZTEST(timeutil_tz, test_roundtrip_at_fall_back)
{
	struct timeutil_tz_info tz;
	struct tm tp;
	int64_t original_utc;
	int64_t recovered_utc;

	timeutil_tz_from_posix("EST5EDT,M3.2.0/2,M11.1.0/2", &tz);

	/* Exactly at the fall-back transition: 2024-11-03 06:00:00 UTC */
	original_utc = 1730613600;
	int ret = timeutil_tz_utc_to_local(&tz, original_utc, &tp);

	zassert_ok(ret);
	/* tm_isdst should be set correctly by utc_to_local, enabling clean round-trip */
	ret = timeutil_tz_local_to_utc(&tz, &tp, &recovered_utc);
	zassert_ok(ret);
	zassert_equal(original_utc, recovered_utc);
}

ZTEST(timeutil_tz, test_epoch_zero)
{
	struct timeutil_tz_info tz;
	struct tm tp;

	timeutil_tz_from_posix("EST5EDT,M3.2.0/2,M11.1.0/2", &tz);

	/* Epoch 0 = 1970-01-01 00:00:00 UTC = 1969-12-31 19:00:00 EST */
	int ret = timeutil_tz_utc_to_local(&tz, 0, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_year, 1969 - 1900);
	zassert_equal(tp.tm_mon, 11); /* December */
	zassert_equal(tp.tm_mday, 31);
	zassert_equal(tp.tm_hour, 19);
	zassert_equal(tp.tm_isdst, 0);
}

ZTEST(timeutil_tz, test_negative_epoch)
{
	struct timeutil_tz_info tz;
	struct tm tp;

	timeutil_tz_from_posix("IST-5:30", &tz);

	/* -1 = 1969-12-31 23:59:59 UTC = 1970-01-01 05:29:59 IST */
	int ret = timeutil_tz_utc_to_local(&tz, -1, &tp);

	zassert_ok(ret);
	zassert_equal(tp.tm_year, 1970 - 1900);
	zassert_equal(tp.tm_mon, 0);
	zassert_equal(tp.tm_mday, 1);
	zassert_equal(tp.tm_hour, 5);
	zassert_equal(tp.tm_min, 29);
	zassert_equal(tp.tm_sec, 59);
}

ZTEST(timeutil_tz, test_utc_to_local_overflow)
{
	struct timeutil_tz_info tz;
	struct tm tp;

	timeutil_tz_from_posix("UTC0", &tz);

	/* INT64_MAX seconds produces a year far beyond INT_MAX */
	zassert_equal(timeutil_tz_utc_to_local(&tz, INT64_MAX, &tp), -EOVERFLOW);
	zassert_equal(timeutil_tz_utc_to_local(&tz, INT64_MIN, &tp), -EOVERFLOW);
}

/*
 * ---- Picolibc-compatible POSIX TZ string parse validation ----
 *
 * Test vectors adapted from picolibc's test_tz_posix_parse.c.
 * Validates that our parser produces the same offsets.
 */

#define NO_DST       INT32_MIN
#define HMS(h, m, s) ((int32_t)(h) * 3600 + (m) * 60 + (s))

static void check_tz_parse(const char *tzstr, int exp_ret, int32_t exp_std, int32_t exp_dst)
{
	struct timeutil_tz_info tz;
	int ret = timeutil_tz_from_posix(tzstr, &tz);

	zassert_equal(ret, exp_ret, "parse \"%s\": ret=%d expected=%d", tzstr, ret, exp_ret);
	if (ret != 0) {
		return;
	}
	zassert_equal(tz.std_offset, exp_std, "\"%s\" std=%d expected=%d", tzstr, tz.std_offset,
		      exp_std);
	if (exp_dst != NO_DST) {
		zassert_true(tz.has_dst, "\"%s\" expected has_dst", tzstr);
		zassert_equal(tz.dst_offset, exp_dst, "\"%s\" dst=%d expected=%d", tzstr,
			      tz.dst_offset, exp_dst);
	} else {
		zassert_false(tz.has_dst, "\"%s\" unexpected has_dst", tzstr);
	}
}

ZTEST(timeutil_tz, test_picolibc_no_dst)
{
	/* Standard name + offset, no DST */
	check_tz_parse("MAR1", 0, HMS(1, 0, 0), NO_DST);
	check_tz_parse("MAR-1", 0, -HMS(1, 0, 0), NO_DST);
	check_tz_parse("MAR+2", 0, HMS(2, 0, 0), NO_DST);
	check_tz_parse("MAR7", 0, HMS(7, 0, 0), NO_DST);
	check_tz_parse("MAR-7", 0, -HMS(7, 0, 0), NO_DST);
	check_tz_parse("MARS5", 0, HMS(5, 0, 0), NO_DST);
	check_tz_parse("MARSM5", 0, HMS(5, 0, 0), NO_DST);
	check_tz_parse("MARSMOON5", 0, HMS(5, 0, 0), NO_DST); /* abbr truncated */
	check_tz_parse("MARS5:23:42", 0, HMS(5, 23, 42), NO_DST);
	check_tz_parse("SUN-7:14:24", 0, -HMS(7, 14, 24), NO_DST);

	/* Quoted abbreviations */
	check_tz_parse("<UNK>-1", 0, -HMS(1, 0, 0), NO_DST);
	check_tz_parse("<UNKNOWN>-2", 0, -HMS(2, 0, 0), NO_DST); /* abbr truncated */
	check_tz_parse("<003>3", 0, HMS(3, 0, 0), NO_DST);
	check_tz_parse("<+04>4", 0, HMS(4, 0, 0), NO_DST);
	check_tz_parse("<-05>-5", 0, -HMS(5, 0, 0), NO_DST);
	check_tz_parse("<A-5>6", 0, HMS(6, 0, 0), NO_DST);
	check_tz_parse("<+A5>-7", 0, -HMS(7, 0, 0), NO_DST);
	check_tz_parse("<0123456>8", 0, HMS(8, 0, 0), NO_DST); /* abbr truncated */
	check_tz_parse("<0A1B2C3>9", 0, HMS(9, 0, 0), NO_DST); /* abbr truncated */

	/* Real-world no-DST zones */
	check_tz_parse("<+14>-14", 0, -HMS(14, 0, 0), NO_DST);     /* Etc/GMT-14 */
	check_tz_parse("<-12>12", 0, HMS(12, 0, 0), NO_DST);       /* Etc/GMT+12 */
	check_tz_parse("<+01>-1", 0, -HMS(1, 0, 0), NO_DST);       /* Africa/Casablanca */
	check_tz_parse("<-03>3", 0, HMS(3, 0, 0), NO_DST);         /* America/Araguaina */
	check_tz_parse("<+0530>-5:30", 0, -HMS(5, 30, 0), NO_DST); /* Asia/Colombo */
}

ZTEST(timeutil_tz, test_picolibc_dst_with_rules)
{
	/* DST with complete start,end transition rules */
	check_tz_parse("WMARS3SMARS,J80,J134", 0, HMS(3, 0, 0), HMS(2, 0, 0));
	check_tz_parse("WMARS3SMARS,76,134", 0, HMS(3, 0, 0), HMS(2, 0, 0));
	check_tz_parse("WMARS3SMARS,76/02,134/03", 0, HMS(3, 0, 0), HMS(2, 0, 0));
	check_tz_parse("WMARS3SMARS,76/02:15:45,134/03:40:20", 0, HMS(3, 0, 0), HMS(2, 0, 0));
	check_tz_parse("WMARS3SMARS,M3.4.1/02:15:45,M8.3.1/03:40:20", 0, HMS(3, 0, 0),
		       HMS(2, 0, 0));

	/* Real-world DST zones */
	check_tz_parse("<-04>4<-03>,M10.1.0/0,M3.4.0/0", 0, HMS(4, 0, 0),
		       HMS(3, 0, 0)); /* America/Asuncion */
	check_tz_parse("PST8PDT,M3.2.0,M11.1.0", 0, HMS(8, 0, 0),
		       HMS(7, 0, 0)); /* America/Los_Angeles */
	check_tz_parse("EST5EDT,M3.2.0,M11.1.0", 0, HMS(5, 0, 0),
		       HMS(4, 0, 0)); /* America/New_York */
	check_tz_parse("<-01>1<+00>,M3.5.0/0,M10.5.0/1", 0, HMS(1, 0, 0),
		       HMS(0, 0, 0)); /* America/Scoresbysund */
	check_tz_parse("CET-1CEST,M3.5.0,M10.5.0/3", 0, -HMS(1, 0, 0),
		       -HMS(2, 0, 0)); /* Europe/Berlin */
}

ZTEST(timeutil_tz, test_picolibc_dst_default_rules)
{
	/*
	 * DST name without explicit transition rules uses the POSIX default
	 * US rule: M3.2.0/2,M11.1.0/2
	 */
	check_tz_parse("MAR5SMAR", 0, HMS(5, 0, 0), HMS(4, 0, 0));
	check_tz_parse("MAR5SMAR2", 0, HMS(5, 0, 0), HMS(2, 0, 0));
	check_tz_parse("MAR3SMAR-3", 0, HMS(3, 0, 0), -HMS(3, 0, 0));
	check_tz_parse("MARSWINTER4MARSUMMER", 0, HMS(4, 0, 0), HMS(3, 0, 0));
	check_tz_parse("MARSWINTER4MARSUMMER3", 0, HMS(4, 0, 0), HMS(3, 0, 0));
	check_tz_parse("<RD-04>-4<RD+005>5", 0, -HMS(4, 0, 0), HMS(5, 0, 0));
	check_tz_parse("<WINT+03>3<SUM+02>", 0, HMS(3, 0, 0), HMS(2, 0, 0));
	check_tz_parse("<WINT+03>3<SUM+02>2", 0, HMS(3, 0, 0), HMS(2, 0, 0));
	check_tz_parse("<WINT+03>3:15<SUM+02>2:30:15", 0, HMS(3, 15, 0), HMS(2, 30, 15));
	check_tz_parse("<H3M15>3:15<H2M30S15>2:30:15", 0, HMS(3, 15, 0), HMS(2, 30, 15));
	check_tz_parse("<+H6M20S12>6:20:12<-H4M40S14>-4:40:14", 0,
		       HMS(6, 20, 12), -HMS(4, 40, 14));

	/* Verify the default rules are M3.2.0/2,M11.1.0/2 */
	struct timeutil_tz_info tz;

	timeutil_tz_from_posix("MAR5SMAR", &tz);
	zassert_equal(tz.dst_start.type, TIMEUTIL_TZ_RULE_MWD);
	zassert_equal(tz.dst_start.mwd.month, 3);
	zassert_equal(tz.dst_start.mwd.week, 2);
	zassert_equal(tz.dst_start.mwd.day, 0);
	zassert_equal(tz.dst_start.time, 7200);
	zassert_equal(tz.dst_end.mwd.month, 11);
	zassert_equal(tz.dst_end.mwd.week, 1);
	zassert_equal(tz.dst_end.mwd.day, 0);
	zassert_equal(tz.dst_end.time, 7200);
}

ZTEST(timeutil_tz, test_picolibc_dst_start_rule_only)
{
	/* DST with only a start rule; end defaults to M11.1.0/2 */
	check_tz_parse("WMARS3SMARS,J80", 0, HMS(3, 0, 0), HMS(2, 0, 0));
	check_tz_parse("WMARS3SMARS,79", 0, HMS(3, 0, 0), HMS(2, 0, 0));

	/* Verify the default end rule M11.1.0/2 */
	struct timeutil_tz_info tz;

	timeutil_tz_from_posix("WMARS3SMARS,J80", &tz);
	zassert_equal(tz.dst_start.type, TIMEUTIL_TZ_RULE_JULIAN_NOLP);
	zassert_equal(tz.dst_start.julian_day, 80);
	zassert_equal(tz.dst_end.type, TIMEUTIL_TZ_RULE_MWD);
	zassert_equal(tz.dst_end.mwd.month, 11);
	zassert_equal(tz.dst_end.mwd.week, 1);
	zassert_equal(tz.dst_end.mwd.day, 0);
	zassert_equal(tz.dst_end.time, 7200);
}

ZTEST(timeutil_tz, test_picolibc_invalid)
{
	/* Names with invalid characters (not alpha for unquoted) */
	check_tz_parse("N?ME2:10:56", -EINVAL, 0, 0);
	check_tz_parse("N!ME2:10:56", -EINVAL, 0, 0);
	check_tz_parse("N/ME2:10:56", -EINVAL, 0, 0);
	check_tz_parse("N$ME2:10:56", -EINVAL, 0, 0);
	check_tz_parse("NAME?2:10:56", -EINVAL, 0, 0);
	check_tz_parse("?NAME2:10:56", -EINVAL, 0, 0);
	check_tz_parse("NAME?UNK4:21:15", -EINVAL, 0, 0);
	check_tz_parse("NAME!UNK4:22:15NEXT/NAME4:23:15", -EINVAL, 0, 0);

	/* Bogus strings */
	check_tz_parse("NOINFO", -EINVAL, 0, 0);
	check_tz_parse("HOUR:16:18", -EINVAL, 0, 0);
	check_tz_parse("<BEGIN", -EINVAL, 0, 0);
	check_tz_parse("<NEXT:55", -EINVAL, 0, 0);
	check_tz_parse(">WRONG<2:15:00", -EINVAL, 0, 0);
	check_tz_parse("ST<ART4:30:00", -EINVAL, 0, 0);
	check_tz_parse("", -EINVAL, 0, 0);
}

ZTEST(timeutil_tz, test_picolibc_local_to_utc)
{
	/*
	 * Validate local_to_utc matches picolibc's mktime for zones where
	 * the test dates' tm_isdst values are consistent with the DST rules.
	 *
	 * winter: March 21, 2022 20:15:20 (isdst=0)
	 * summer: July 15, 2022 10:50:40 (isdst=1)
	 */
	static const int64_t winter_time = 1647893720;
	static const int64_t summer_time = 1657882240;
	struct tm winter_tp = make_tm(2022, 3, 21, 20, 15, 20, 0);
	struct tm summer_tp = make_tm(2022, 7, 15, 10, 50, 40, 1);
	struct timeutil_tz_info tz;
	struct tm tp;
	int64_t epoch;
	int ret;

	/* No-DST zones: winter_tm (isdst=0) always correct */
	timeutil_tz_from_posix("MAR1", &tz);
	tp = winter_tp;
	ret = timeutil_tz_local_to_utc(&tz, &tp, &epoch);
	zassert_ok(ret);
	zassert_equal(epoch, winter_time + HMS(1, 0, 0), "MAR1 winter");

	timeutil_tz_from_posix("<+0530>-5:30", &tz);
	tp = winter_tp;
	ret = timeutil_tz_local_to_utc(&tz, &tp, &epoch);
	zassert_ok(ret);
	zassert_equal(epoch, winter_time + -HMS(5, 30, 0), "Asia/Colombo winter");

	/*
	 * CET: DST starts last Sunday of March (March 27 in 2022).
	 * March 21 is before DST start → isdst=0 correct.
	 * July 15 is in DST → isdst=1 correct.
	 */
	timeutil_tz_from_posix("CET-1CEST,M3.5.0,M10.5.0/3", &tz);
	tp = winter_tp;
	ret = timeutil_tz_local_to_utc(&tz, &tp, &epoch);
	zassert_ok(ret);
	zassert_equal(epoch, winter_time + -HMS(1, 0, 0), "CET winter");

	tp = summer_tp;
	ret = timeutil_tz_local_to_utc(&tz, &tp, &epoch);
	zassert_ok(ret);
	zassert_equal(epoch, summer_time + -HMS(2, 0, 0), "CET summer");

	/*
	 * Scoresbysund: DST starts last Sunday of March at 00:00 (March 27 in 2022).
	 * March 21 is before → isdst=0 correct.
	 * July 15 is in DST → isdst=1 correct.
	 */
	timeutil_tz_from_posix("<-01>1<+00>,M3.5.0/0,M10.5.0/1", &tz);
	tp = winter_tp;
	ret = timeutil_tz_local_to_utc(&tz, &tp, &epoch);
	zassert_ok(ret);
	zassert_equal(epoch, winter_time + HMS(1, 0, 0), "Scoresbysund winter");

	tp = summer_tp;
	ret = timeutil_tz_local_to_utc(&tz, &tp, &epoch);
	zassert_ok(ret);
	zassert_equal(epoch, summer_time + HMS(0, 0, 0), "Scoresbysund summer");
}

ZTEST_SUITE(timeutil_tz, NULL, NULL, NULL, NULL, NULL);
