/*
 * Copyright 2019-2020 Peter Bigot Consulting
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include "timeutil_test.h"
#include "../../../lib/utils/timeutil.c"

void timeutil_check(const struct timeutil_test_data *tp,
		    size_t count)
{
	const struct timeutil_test_data *tpe = tp + count;

	while (tp < tpe) {
		struct tm tm = *gmtime(&tp->ux);
		time_t uxtime = timeutil_timegm(&tm);

		zassert_equal(&tm, gmtime_r(&tp->ux, &tm),
			      "gmtime_r failed");

		zassert_equal(tm.tm_year, tp->tm.tm_year,
			      "datetime %s year %d != %d",
			      tp->civil, tm.tm_year, tp->tm.tm_year);
		zassert_equal(tm.tm_mon, tp->tm.tm_mon,
			      "datetime %s mon %d != %d",
			      tp->civil, tm.tm_mon, tp->tm.tm_mon);
		zassert_equal(tm.tm_mday, tp->tm.tm_mday,
			      "datetime %s mday %d != %d",
			      tp->civil, tm.tm_mday, tp->tm.tm_mday);
		zassert_equal(tm.tm_hour, tp->tm.tm_hour,
			      "datetime %s hour %d != %d",
			      tp->civil, tm.tm_hour, tp->tm.tm_hour);
		zassert_equal(tm.tm_min, tp->tm.tm_min,
			      "datetime %s min %d != %d",
			      tp->civil, tm.tm_min, tp->tm.tm_min);
		zassert_equal(tm.tm_sec, tp->tm.tm_sec,
			      "datetime %s sec %d != %d",
			      tp->civil, tm.tm_sec, tp->tm.tm_sec);
		zassert_equal(tm.tm_wday, tp->tm.tm_wday,
			      "datetime %s wday %d != %d",
			      tp->civil, tm.tm_wday, tp->tm.tm_wday);
		zassert_equal(tm.tm_yday, tp->tm.tm_yday,
			      "datetime %s yday %d != %d",
			      tp->civil, tm.tm_yday, tp->tm.tm_yday);
		zassert_equal(tp->ux, uxtime,
			      "datetime %s reverse != %ld",
			      tp->civil, uxtime);

		++tp;
	}
}

ZTEST_SUITE(timeutil_api, NULL, NULL, NULL, NULL, NULL);
