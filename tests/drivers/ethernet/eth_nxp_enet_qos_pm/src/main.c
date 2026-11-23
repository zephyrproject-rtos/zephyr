/*
 * Copyright (c) 2026 Fr. Sauter AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/pm/device.h>
#include <zephyr/ztest.h>

static struct net_if *test_iface(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_enet_qos));
	struct net_if *iface;

	zassert_true(device_is_ready(dev), "ENET QoS device is not ready");
	iface = net_if_lookup_by_dev(dev);
	zassert_not_null(iface);
	return iface;
}

static void assert_device_state(const struct device *dev, enum pm_device_state expected)
{
	enum pm_device_state state;

	zassert_ok(pm_device_state_get(dev, &state));
	zassert_equal(state, expected);
}

static int suspend_device(const struct device *dev)
{
	/* Allow RX work queued during interface initialization to finish. */
	for (uint32_t retry = 0U; retry < 100U; retry++) {
		int ret = pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);

		if (ret != -EBUSY) {
			return ret;
		}
		k_msleep(1);
	}

	return -EBUSY;
}

static void restore_active(void *fixture)
{
	struct net_if *iface = test_iface();
	const struct device *dev = net_if_get_device(iface);
	enum pm_device_state state;
	int ret;

	ARG_UNUSED(fixture);

	zassert_ok(pm_device_state_get(dev, &state));
	if (state == PM_DEVICE_STATE_SUSPENDED) {
		zassert_ok(pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME));
	}
	ret = net_if_resume(iface);
	zassert_true(ret == 0 || ret == -EALREADY);
	assert_device_state(dev, PM_DEVICE_STATE_ACTIVE);
}

ZTEST(enet_qos_pm, test_suspend_resume)
{
	struct net_if *iface = test_iface();
	const struct device *dev = net_if_get_device(iface);

	zassert_ok(suspend_device(dev));
	assert_device_state(dev, PM_DEVICE_STATE_SUSPENDED);
	zassert_true(net_if_is_suspended(iface));
	zassert_ok(pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME));
	assert_device_state(dev, PM_DEVICE_STATE_ACTIVE);
	zassert_false(net_if_is_suspended(iface));
}

ZTEST(enet_qos_pm, test_suspend_already_suspended_iface)
{
	struct net_if *iface = test_iface();
	const struct device *dev = net_if_get_device(iface);

	zassert_ok(net_if_suspend(iface));
	zassert_equal(suspend_device(dev), -EALREADY);
	assert_device_state(dev, PM_DEVICE_STATE_ACTIVE);
	zassert_true(net_if_is_suspended(iface));
}

ZTEST(enet_qos_pm, test_resume_already_resumed_iface)
{
	struct net_if *iface = test_iface();
	const struct device *dev = net_if_get_device(iface);

	zassert_ok(suspend_device(dev));
	zassert_ok(net_if_resume(iface));
	zassert_ok(pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME));
	assert_device_state(dev, PM_DEVICE_STATE_ACTIVE);
	zassert_false(net_if_is_suspended(iface));
}

ZTEST(enet_qos_pm, test_tx_rejected_while_suspended)
{
	struct net_if *iface = test_iface();
	const struct device *dev = net_if_get_device(iface);
	uint8_t frame[NET_ETH_MINIMAL_FRAME_SIZE] = {0};
	struct net_pkt *pkt;
	int ret;

	zassert_ok(suspend_device(dev));
	pkt = net_pkt_alloc_with_buffer(iface, sizeof(frame), NET_AF_UNSPEC, 0, K_NO_WAIT);
	zassert_not_null(pkt);
	ret = net_pkt_write(pkt, frame, sizeof(frame));
	if (ret == 0) {
		net_pkt_set_ll_proto_type(pkt, 0x88B5U);
		/* Reach the driver through L2 without the network core's suspend check. */
		ret = net_if_l2(iface)->send(iface, pkt);
	}
	if (ret < 0) {
		net_pkt_unref(pkt);
	}
	zassert_equal(ret, -EBUSY);
	assert_device_state(dev, PM_DEVICE_STATE_SUSPENDED);
}

ZTEST_SUITE(enet_qos_pm, NULL, NULL, restore_active, restore_active, NULL);
