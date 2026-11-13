/*
 * SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its
 * SPDX-FileCopyrightText: affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/profiling/perf.h>
#include <zephyr/ztest.h>

ZTEST(perf_events, test_lookup)
{
	struct perf_event_handle event = {
		.token = UINT64_MAX,
		.baseline = UINT64_MAX,
		.final_count = UINT64_MAX,
		.status = -1,
	};

	zassert_ok(perf_event_lookup("cpu0.cycles", &event));
	zassert_not_equal(event.token, 0U);
	zassert_equal(event.baseline, 0U);
	zassert_equal(event.final_count, 0U);
	zassert_equal(event.status, 0);
	zassert_equal(perf_event_lookup("cpu0.unknown", &event), -ENOENT);
	zassert_equal(perf_event_lookup("unknown.cycles", &event), -ENOENT);
	zassert_equal(perf_event_lookup("cycles", &event), -EINVAL);
	zassert_equal(perf_event_lookup(NULL, &event), -EINVAL);
	zassert_equal(perf_event_lookup("cpu0.cycles", NULL), -EINVAL);
}

ZTEST(perf_events, test_snapshot_session)
{
	struct perf_event_handle event;
	struct perf_stat_config config = {
		.events = &event,
		.num_events = 1U,
	};

	zassert_ok(perf_event_lookup("cpu0.cycles", &event));
	zassert_ok(perf_stat_start(&config));
	zassert_equal(perf_stat_start(&config), -EBUSY);
	zassert_ok(perf_stat_stop(&config));
	zassert_equal(event.status, 0);

	zassert_ok(perf_stat_start(&config));
	zassert_ok(perf_stat_stop(&config));
}

ZTEST(perf_events, test_reject_duplicate_event)
{
	struct perf_event_handle events[2];
	struct perf_stat_config config = {
		.events = events,
		.num_events = ARRAY_SIZE(events),
	};

	zassert_ok(perf_event_lookup("cpu0.cycles", &events[0]));
	events[1] = events[0];
	zassert_equal(perf_stat_start(&config), -EINVAL);
}

ZTEST(perf_events, test_validate_config_and_active_identity)
{
	struct perf_event_handle event;
	struct perf_stat_config config = {
		.events = &event,
		.num_events = 1U,
	};
	struct perf_stat_config other_config = config;
	struct perf_event_handle invalid_event = {0};
	struct perf_stat_config invalid_config = {
		.events = &invalid_event,
		.num_events = 1U,
	};

	zassert_equal(perf_stat_start(NULL), -EINVAL);
	zassert_equal(perf_stat_start(&invalid_config), -EINVAL);
	zassert_ok(perf_event_lookup("cpu0.cycles", &event));
	zassert_ok(perf_stat_start(&config));
	zassert_equal(perf_stat_stop(&other_config), -EINVAL);
	zassert_ok(perf_stat_stop(&config));
	zassert_equal(perf_stat_stop(&config), -EINVAL);
}

ZTEST_SUITE(perf_events, NULL, NULL, NULL, NULL, NULL);
