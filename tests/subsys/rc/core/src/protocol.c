/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/rc.h>
#include <zephyr/drivers/rc/rc_loopback.h>
#include <zephyr/rc/rc.h>
#include <zephyr/rc/rc_protocol.h>
#include <zephyr/ztest.h>

#define TEST_PROTO_A0   0x1f0
#define TEST_PROTO_A1   0x1f1
#define TEST_PROTO_B    0x1f2
#define TEST_PROTO_C    0x1f3
#define TEST_PROTO_NONE 0x1fe

static int test_decode(void *state, uint32_t word, struct rc_decode_result *res)
{
	ARG_UNUSED(state);
	ARG_UNUSED(word);
	ARG_UNUSED(res);

	return 0;
}

RC_PROTOCOL_DEFINE(test_proto_a) = {
	.name = "test-a",
	.proto_first = TEST_PROTO_A0,
	.proto_last = TEST_PROTO_A1,
	.min_timeout_us = 50000,
	.decode = test_decode,
};

RC_PROTOCOL_DEFINE(test_proto_b) = {
	.name = "test-b",
	.proto_first = TEST_PROTO_B,
	.proto_last = TEST_PROTO_B,
	.min_timeout_us = 30000,
	.decode = test_decode,
};

RC_PROTOCOL_DEFINE(test_proto_c) = {
	.name = "test-c",
	.proto_first = TEST_PROTO_C,
	.proto_last = TEST_PROTO_C,
	.min_timeout_us = 2000000,
	.decode = test_decode,
};

static const struct device *const dev0 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback0));
static const struct device *const dev1 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback1));
static const struct device *const dev2 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback2));
static const struct device *const dev3 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback3));
static const struct device *const dev4 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback4));

struct rc_protocol_fixture {
	int dev0_a;
	int dev0_b;
	int dev1_a;
	int dev1_b;
};

static void *rc_protocol_setup(void)
{
	static struct rc_protocol_fixture defaults;

	defaults.dev0_a = rc_protocol_is_enabled(dev0, TEST_PROTO_A0);
	defaults.dev0_b = rc_protocol_is_enabled(dev0, TEST_PROTO_B);
	defaults.dev1_a = rc_protocol_is_enabled(dev1, TEST_PROTO_A0);
	defaults.dev1_b = rc_protocol_is_enabled(dev1, TEST_PROTO_B);

	return &defaults;
}

static void rc_protocol_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(rc_protocol_enable(dev0, TEST_PROTO_A0));
	zassert_ok(rc_protocol_enable(dev0, TEST_PROTO_B));
	zassert_ok(rc_protocol_disable(dev1, TEST_PROTO_A0));
	zassert_ok(rc_protocol_enable(dev1, TEST_PROTO_B));
}

ZTEST_F(rc_protocol, test_default_all)
{
	zassert_equal(fixture->dev0_a, 1);
	zassert_equal(fixture->dev0_b, 1);
}

ZTEST_F(rc_protocol, test_default_dt)
{
	zassert_equal(fixture->dev1_a, 0);
	zassert_equal(fixture->dev1_b, 1);
}

ZTEST(rc_protocol, test_enable_disable)
{
	zassert_ok(rc_protocol_disable(dev0, TEST_PROTO_B));
	zassert_equal(rc_protocol_is_enabled(dev0, TEST_PROTO_B), 0);
	zassert_equal(rc_protocol_is_enabled(dev0, TEST_PROTO_A0), 1);

	zassert_ok(rc_protocol_enable(dev0, TEST_PROTO_B));
	zassert_equal(rc_protocol_is_enabled(dev0, TEST_PROTO_B), 1);
}

ZTEST(rc_protocol, test_same_decoder)
{
	zassert_ok(rc_protocol_disable(dev0, TEST_PROTO_A1));
	zassert_equal(rc_protocol_is_enabled(dev0, TEST_PROTO_A0), 0);
	zassert_ok(rc_protocol_enable(dev0, TEST_PROTO_A0));
	zassert_equal(rc_protocol_is_enabled(dev0, TEST_PROTO_A1), 1);
}

ZTEST(rc_protocol, test_devices_independent)
{
	zassert_ok(rc_protocol_disable(dev0, TEST_PROTO_B));
	zassert_equal(rc_protocol_is_enabled(dev1, TEST_PROTO_B), 1);
}

ZTEST(rc_protocol, test_not_built)
{
	zassert_equal(rc_protocol_enable(dev0, TEST_PROTO_NONE), -ENOTSUP);
	zassert_equal(rc_protocol_disable(dev0, TEST_PROTO_NONE), -ENOTSUP);
	zassert_equal(rc_protocol_is_enabled(dev0, TEST_PROTO_NONE), -ENOTSUP);
	zassert_equal(rc_protocol_is_enabled(dev1, TEST_PROTO_NONE), -ENOTSUP);
	zassert_equal(rc_protocol_enable(dev0, RC_PROTO_NEC), -ENOTSUP);
}

ZTEST_SUITE(rc_protocol, NULL, rc_protocol_setup, rc_protocol_before, NULL, NULL);

ZTEST(rc_rx_timeout, test_auto)
{
	zassert_equal(rc_loopback_rx_timeout_us(dev3), 1000000U);

	zassert_ok(rc_protocol_disable(dev3, TEST_PROTO_C));
	zassert_equal(rc_loopback_rx_timeout_us(dev3), 60000U);
	zassert_ok(rc_protocol_disable(dev3, TEST_PROTO_A0));
	zassert_equal(rc_loopback_rx_timeout_us(dev3), 40000U);

	zassert_ok(rc_protocol_disable(dev3, TEST_PROTO_B));
	zassert_equal(rc_loopback_rx_timeout_us(dev3), 125000U);

	zassert_ok(rc_protocol_enable(dev3, TEST_PROTO_B));
	zassert_equal(rc_loopback_rx_timeout_us(dev3), 40000U);
	zassert_ok(rc_protocol_enable(dev3, TEST_PROTO_A0));
	zassert_ok(rc_protocol_enable(dev3, TEST_PROTO_C));
	zassert_equal(rc_loopback_rx_timeout_us(dev3), 1000000U);
}

ZTEST(rc_rx_timeout, test_explicit_dt)
{
	uint32_t dt_us = DT_PROP(DT_NODELABEL(rc_loopback2), rx_timeout_us);

	zassert_equal(rc_loopback_rx_timeout_us(dev2), dt_us);
	zassert_ok(rc_protocol_enable(dev2, TEST_PROTO_A0));
	zassert_equal(rc_loopback_rx_timeout_us(dev2), dt_us);
	zassert_ok(rc_protocol_disable(dev2, TEST_PROTO_A0));
	zassert_equal(rc_loopback_rx_timeout_us(dev2), dt_us);
}

ZTEST(rc_rx_timeout, test_explicit_set)
{
	zassert_equal(rc_loopback_rx_timeout_us(dev4), 1000000U);

	zassert_equal(rc_set_rx_timeout(dev4, 0), -EINVAL);
	zassert_ok(rc_set_rx_timeout(dev4, 7000));
	zassert_equal(rc_loopback_rx_timeout_us(dev4), 7000U);

	zassert_ok(rc_protocol_disable(dev4, TEST_PROTO_C));
	zassert_equal(rc_loopback_rx_timeout_us(dev4), 7000U);
	zassert_ok(rc_protocol_enable(dev4, TEST_PROTO_C));
	zassert_equal(rc_loopback_rx_timeout_us(dev4), 7000U);
}

ZTEST_SUITE(rc_rx_timeout, NULL, NULL, NULL, NULL, NULL);
