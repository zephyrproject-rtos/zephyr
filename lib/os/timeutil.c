/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The time_days_from_civil function is derived directly from public
 * domain content written by Howard Hinnant and available at:
 * http://howardhinnant.github.io/date_algorithms.html#days_from_civil
 */

#include <zephyr/types.h>
#include <errno.h>
#include <stddef.h>
#include <sys/timeutil.h>

/** Convert a civil (proleptic Gregorian) date to days relative to
 * 1970-01-01.
 *
 * @param y the calendar year
 * @param m the calendar month, in the range [1, 12]
 * @param d the day of the month, in the range [1, last_day_of_month(y, m)]
 *
 * @return the signed number of days between the specified day and
 * 1970-01-01
 *
 * @see http://howardhinnant.github.io/date_algorithms.html#days_from_civil
 */
static int64_t time_days_from_civil(int64_t y,
				  unsigned int m,
				  unsigned int d)
{
	y -= m <= 2;

	int64_t era = (y >= 0 ? y : y - 399) / 400;
	unsigned int yoe = y - era * 400;
	unsigned int doy = (153U * (m + (m > 2 ? -3 : 9)) + 2U) / 5U + d;
	unsigned int doe = yoe * 365U + yoe / 4U - yoe / 100U + doy;

	return era * 146097 + (time_t)doe - 719468;
}

int64_t timeutil_timegm64(const struct tm *tm)
{
	int64_t y = 1900 + (int64_t)tm->tm_year;
	unsigned int m = tm->tm_mon + 1;
	unsigned int d = tm->tm_mday - 1;
	int64_t ndays = time_days_from_civil(y, m, d);
	int64_t time = tm->tm_sec;

	time += 60LL * (tm->tm_min + 60LL * tm->tm_hour);
	time += 86400LL * ndays;

	return time;
}

time_t timeutil_timegm(const struct tm *tm)
{
	int64_t time = timeutil_timegm64(tm);
	time_t rv = (time_t)time;

	errno = 0;
	if ((sizeof(rv) == sizeof(int32_t))
	    && ((time < (int64_t)INT32_MIN)
		|| (time > (int64_t)INT32_MAX))) {
		errno = ERANGE;
		rv = -1;
	}

	return rv;
}

int timeutil_sync_state_update(struct timeutil_sync_state *tsp,
			       const struct timeutil_sync_instant *inst)
{
	int rv = -EINVAL;

	if (((tsp->base.ref == 0) && (inst->ref > 0))
	    || ((inst->ref > tsp->base.ref)
		&& (inst->local > tsp->base.local))) {
		if (tsp->base.ref == 0) {
			tsp->base = *inst;
			tsp->latest = (struct timeutil_sync_instant){};
			tsp->skew = 1.0;
			rv = 0;
		} else {
			tsp->latest = *inst;
			rv = 1;
		}
	}

	return rv;
}

int timeutil_sync_state_set_skew(struct timeutil_sync_state *tsp, float skew,
				 const struct timeutil_sync_instant *base)
{
	int rv = -EINVAL;

	if (skew > 0) {
		tsp->skew = skew;
		if (base != NULL) {
			tsp->base = *base;
			tsp->latest = (struct timeutil_sync_instant){};
		}
		rv = 0;
	}

	return rv;
}

float timeutil_sync_estimate_skew(const struct timeutil_sync_state *tsp)
{
	float rv = 0;

	if ((tsp->base.ref != 0) && (tsp->latest.ref != 0)
	    && (tsp->latest.local > tsp->base.local)) {
		const struct timeutil_sync_config *cfg = tsp->cfg;
		double ref_delta = tsp->latest.ref - tsp->base.ref;
		double local_delta = tsp->latest.local - tsp->base.local;

		rv = ref_delta * cfg->local_Hz / local_delta / cfg->ref_Hz;
	}

	return rv;
}

int timeutil_sync_ref_from_local(const struct timeutil_sync_state *tsp,
				 uint64_t local, uint64_t *refp)
{
	int rv = -EINVAL;

	if ((tsp->skew > 0) && (tsp->base.ref > 0) && (refp != NULL)) {
		const struct timeutil_sync_config *cfg = tsp->cfg;
		int64_t local_delta = local - tsp->base.local;
		int64_t ref_delta = (int64_t)(tsp->skew * local_delta) *
			cfg->ref_Hz / cfg->local_Hz;
		int64_t ref_abs = (int64_t)tsp->base.ref + ref_delta;

		if (ref_abs < 0) {
			rv = -ERANGE;
		} else {
			*refp = ref_abs;
			rv = (int)(tsp->skew != 1.0);
		}
	}

	return rv;
}

int timeutil_sync_local_from_ref(const struct timeutil_sync_state *tsp,
				 uint64_t ref, int64_t *localp)
{
	int rv = -EINVAL;

	if ((tsp->skew > 0) && (tsp->base.ref > 0) && (localp != NULL)) {
		const struct timeutil_sync_config *cfg = tsp->cfg;
		int64_t ref_delta = (int64_t)(ref - tsp->base.ref);
		double local_delta = (ref_delta * cfg->local_Hz) / cfg->ref_Hz
			/ tsp->skew;
		int64_t local_abs = (int64_t)tsp->base.local
			+ (int64_t)local_delta;

		*localp = local_abs;
		rv = (int)(tsp->skew != 1.0);
	}

	return rv;
}

int32_t timeutil_sync_skew_to_ppb(float skew)
{
	int64_t ppb64 = (int64_t)((1.0 - skew) * 1E9);
	int32_t ppb32 = (int32_t)ppb64;

	return (ppb64 == ppb32) ? ppb32 : INT32_MIN;
}
