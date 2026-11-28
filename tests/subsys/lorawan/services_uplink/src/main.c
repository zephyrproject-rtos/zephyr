/*
 * Copyright (c) 2026 Valentin Grütze
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/lorawan/emul.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/ztest.h>

#include "lorawan_services.h"

/* longer than any delay used below */
#define UPLINK_TIMEOUT K_MSEC(3000)

struct uplink {
	int64_t uptime;
	uint8_t port;
};

/* Uplinks in the order the services sent them */
K_MSGQ_DEFINE(uplink_msgq, sizeof(struct uplink), 10, 4);

static void uplink_handler(uint8_t port, uint8_t len, const uint8_t *data)
{
	struct uplink uplink = {
		.uptime = k_uptime_get(),
		.port = port,
	};

	ARG_UNUSED(len);
	ARG_UNUSED(data);

	zassert_ok(k_msgq_put(&uplink_msgq, &uplink, K_NO_WAIT));
}

static void schedule_uplink(uint8_t port, uint32_t delay_ms)
{
	uint8_t payload = port;

	zassert_ok(lorawan_services_schedule_uplink(port, &payload, sizeof(payload), delay_ms));
}

/* Receive all scheduled uplinks before checking them, so none leaks into the next test. */
static void receive_uplinks(struct uplink *uplinks, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		zassert_ok(k_msgq_get(&uplink_msgq, &uplinks[i], UPLINK_TIMEOUT),
			   "only %zu of %zu uplinks sent", i, count);
	}
}

static void assert_ports(const struct uplink *uplinks, const uint8_t *ports, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		zassert_equal(uplinks[i].port, ports[i], "uplink %zu: expected port %u, got %u", i,
			      ports[i], uplinks[i].port);
	}
}

ZTEST(services_uplink, test_earlier_deadline_overtakes_queued_uplink)
{
	const uint8_t expected[] = {2, 1};
	struct uplink uplinks[ARRAY_SIZE(expected)];
	int64_t start = k_uptime_get();

	/* e.g. a delayed FragSessionStatusAns, then an immediate AppTimeReq */
	schedule_uplink(1, 2000);
	schedule_uplink(2, 0);

	receive_uplinks(uplinks, ARRAY_SIZE(uplinks));
	assert_ports(uplinks, expected, ARRAY_SIZE(expected));
	zassert_true(uplinks[0].uptime - start < 100, "immediate uplink sent after %lld ms",
		     uplinks[0].uptime - start);
}

ZTEST(services_uplink, test_deadline_order)
{
	const uint8_t expected[] = {2, 3, 1};
	struct uplink uplinks[ARRAY_SIZE(expected)];

	schedule_uplink(1, 300);
	schedule_uplink(2, 100);
	schedule_uplink(3, 200);

	receive_uplinks(uplinks, ARRAY_SIZE(uplinks));
	assert_ports(uplinks, expected, ARRAY_SIZE(expected));
}

ZTEST(services_uplink, test_equal_deadlines_keep_arrival_order)
{
	const uint8_t expected[] = {1, 2, 3};
	struct uplink uplinks[ARRAY_SIZE(expected)];

	schedule_uplink(1, 100);
	schedule_uplink(2, 100);
	schedule_uplink(3, 100);

	receive_uplinks(uplinks, ARRAY_SIZE(uplinks));
	assert_ports(uplinks, expected, ARRAY_SIZE(expected));
}

static void *services_uplink_setup(void)
{
	lorawan_emul_register_uplink_callback(uplink_handler);

	return NULL;
}

static void services_uplink_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_msgq_purge(&uplink_msgq);
}

ZTEST_SUITE(services_uplink, NULL, services_uplink_setup, services_uplink_before, NULL, NULL);
