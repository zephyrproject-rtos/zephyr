/*
 * Copyright (c) 2026 Ziyou Wang
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/canbus/isotp.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/fff.h>
#include <zephyr/net_buf.h>
#if defined(CONFIG_OBJ_CORE_TIMER)
#include <zephyr/kernel/obj_core.h>
#endif
#include <zephyr/ztest.h>

#define TEST_SF_LEN 7U

/*
 * Number of failed bind attempts used to detect a leaked receive buffer. It
 * must exceed CONFIG_ISOTP_RX_SF_FF_BUF_COUNT (2 in prj.conf), so that a leak
 * exhausts the pool before the last attempt is made.
 */
#define TEST_FAILED_BINDS 3

static const struct isotp_msg_id rx_addr = {
	.std_id = 0x10,
};
static const struct isotp_msg_id tx_addr = {
	.std_id = 0x11,
};
static const struct isotp_fc_opts fc_opts = {
	.bs = 8,
	.stmin = 0,
};

static const struct device *const fake_can_dev = DEVICE_DT_GET(DT_NODELABEL(fake_can));

static const uint8_t test_payload[TEST_SF_LEN] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

DEFINE_FFF_GLOBALS;

static bool fail_filter_add;

#if defined(CONFIG_OBJ_CORE_TIMER)
static int find_obj_core_op(struct k_obj_core *obj_core, void *data)
{
	struct k_obj_core *target = data;

	return (obj_core == target) ? 1 : 0;
}

static bool timer_obj_core_registered(struct k_timer *timer)
{
	struct k_obj_type *type = k_obj_type_find(K_OBJ_TYPE_TIMER_ID);
	struct k_obj_core *target = K_OBJ_CORE(timer);

	zassert_not_null(type, "timer object type not found");

	return k_obj_type_walk_locked(type, find_obj_core_op, target) != 0;
}
#endif

/*
 * Fake can_add_rx_filter() implementation. It fails the registration when
 * fail_filter_add is set, otherwise it simulates a frame being received as
 * soon as the RX filter becomes active, that is from within the
 * can_add_rx_filter() call made by isotp_bind().
 */
static int add_rx_filter_custom(const struct device *dev, can_rx_callback_t callback,
				void *user_data, const struct can_filter *filter)
{
	const struct isotp_recv_ctx *rctx = user_data;
	struct can_frame frame = {0};

	ARG_UNUSED(filter);

	/*
	 * isotp_bind() must initialize the receive work and timer before the
	 * filter registration, because a frame may be delivered as soon as the
	 * filter is active.
	 */
	zassert_not_null(rctx->work.handler,
			 "receive work not initialized before filter registration");
	zassert_not_null(rctx->timer.expiry_fn,
			 "receive timer not initialized before filter registration");

	if (fail_filter_add) {
		return -ENOSPC;
	}

	/* Simulate the controller delivering a matching frame as soon as the
	 * filter becomes active, that is before isotp_bind() returns.
	 */
	frame.id = rx_addr.std_id;
	frame.dlc = 8U;
	/* ISO-TP single frame carrying TEST_SF_LEN payload bytes */
	frame.data[0] = TEST_SF_LEN;
	memcpy(&frame.data[1], test_payload, TEST_SF_LEN);

	callback(dev, &frame, user_data);

	return 0;
}

static void expect_sf(struct isotp_recv_ctx *rctx)
{
	struct net_buf *buf = NULL;
	int remaining;

	remaining = isotp_recv_net(rctx, &buf, K_MSEC(1000));
	zassert_equal(remaining, 0, "unexpected remaining length (%d)", remaining);
	zassert_equal(buf->len, TEST_SF_LEN, "unexpected data length (%d)", buf->len);
	zassert_mem_equal(buf->data, test_payload, TEST_SF_LEN, "received data differ");

	net_buf_unref(buf);
}

ZTEST(isotp_bind, test_bind_rx_state_ready_before_filter_registration)
{
	struct isotp_recv_ctx rctx = {0};
	int ret;

	fake_can_add_rx_filter_fake.custom_fake = add_rx_filter_custom;

	ret = isotp_bind(&rctx, fake_can_dev, &rx_addr, &tx_addr, &fc_opts, K_NO_WAIT);
	zassert_equal(ret, ISOTP_N_OK, "binding failed (%d)", ret);
	zassert_equal(fake_can_add_rx_filter_fake.call_count, 1,
		      "RX filter was not registered");

	/* The frame delivered from within can_add_rx_filter() must be received
	 * even though it arrived before isotp_bind() returned.
	 */
	expect_sf(&rctx);

	isotp_unbind(&rctx);
	zassert_equal(fake_can_remove_rx_filter_fake.call_count, 1,
		      "RX filter was not removed");
}

ZTEST(isotp_bind, test_bind_failure_cleanup)
{
	struct isotp_recv_ctx rctx = {0};
	int ret;

	fail_filter_add = true;
	fake_can_add_rx_filter_fake.custom_fake = add_rx_filter_custom;

	/* Fail more registrations than the FF/SF buffer pool can hold, without
	 * unbinding in between. If isotp_bind() leaked the receive buffer on
	 * this path, the pool would be exhausted and a later attempt would fail
	 * with ISOTP_NO_NET_BUF_LEFT instead of ISOTP_NO_FREE_FILTER.
	 */
	for (int i = 0; i < TEST_FAILED_BINDS; i++) {
		ret = isotp_bind(&rctx, fake_can_dev, &rx_addr, &tx_addr, &fc_opts,
				 K_NO_WAIT);
		zassert_equal(ret, ISOTP_NO_FREE_FILTER,
			      "unexpected error on failed bind %d (%d)", i, ret);
	}

	/* Restore the filter registration: the same context must bind and
	 * receive successfully, proving the failed attempts released their
	 * buffers.
	 */
	fail_filter_add = false;

	ret = isotp_bind(&rctx, fake_can_dev, &rx_addr, &tx_addr, &fc_opts, K_NO_WAIT);
	zassert_equal(ret, ISOTP_N_OK, "binding retry failed (%d)", ret);
	expect_sf(&rctx);
	isotp_unbind(&rctx);
}

ZTEST(isotp_bind, test_repeated_bind_unbind_with_early_rx)
{
	struct isotp_recv_ctx rctx = {0};
	int ret;

	fake_can_add_rx_filter_fake.custom_fake = add_rx_filter_custom;

	for (int i = 0; i < 5; i++) {
		ret = isotp_bind(&rctx, fake_can_dev, &rx_addr, &tx_addr, &fc_opts,
				 K_NO_WAIT);
		zassert_equal(ret, ISOTP_N_OK, "binding %d failed (%d)", i, ret);
		expect_sf(&rctx);
		isotp_unbind(&rctx);
	}
}

#if defined(CONFIG_OBJ_CORE_TIMER)
static int add_rx_filter_objcore(const struct device *dev, can_rx_callback_t callback,
				 void *user_data, const struct can_filter *filter)
{
	struct isotp_recv_ctx *rctx = user_data;

	ARG_UNUSED(dev);
	ARG_UNUSED(callback);
	ARG_UNUSED(filter);

	/* k_timer_init() runs before the RX filter is registered, so the timer
	 * must already be registered when the filter becomes active.
	 */
	zassert_true(timer_obj_core_registered(&rctx->timer),
		     "receive timer not registered before filter registration");

	return -ENOSPC;
}

ZTEST(isotp_bind, test_failed_bind_unregisters_timer)
{
	/* Object core registration is skipped for objects in stack storage, so a
	 * static context is required to observe the receive timer registration.
	 */
	static struct isotp_recv_ctx objcore_ctx;
	struct isotp_recv_ctx *rctx = &objcore_ctx;
	int ret;

	fake_can_add_rx_filter_fake.custom_fake = add_rx_filter_objcore;

	ret = isotp_bind(rctx, fake_can_dev, &rx_addr, &tx_addr, &fc_opts, K_NO_WAIT);
	zassert_equal(ret, ISOTP_NO_FREE_FILTER, "unexpected error on failed bind (%d)", ret);

	/* k_timer_cleanup() must undo the registration performed by
	 * k_timer_init(), otherwise a stale timer entry stays in the registry.
	 */
	zassert_false(timer_obj_core_registered(&rctx->timer),
		      "timer still registered after failed bind");

	/* The same context must stay usable: binding again registers the timer
	 * and the receive path still works.
	 */
	fake_can_add_rx_filter_fake.custom_fake = add_rx_filter_custom;
	fail_filter_add = false;

	ret = isotp_bind(rctx, fake_can_dev, &rx_addr, &tx_addr, &fc_opts, K_NO_WAIT);
	zassert_equal(ret, ISOTP_N_OK, "binding failed (%d)", ret);
	zassert_true(timer_obj_core_registered(&rctx->timer),
		     "timer not registered after successful bind");

	expect_sf(rctx);
	isotp_unbind(rctx);
}
#endif

static void isotp_bind_before(void *fixture)
{
	ARG_UNUSED(fixture);

	fail_filter_add = false;
}

ZTEST_SUITE(isotp_bind, NULL, NULL, isotp_bind_before, NULL, NULL);
