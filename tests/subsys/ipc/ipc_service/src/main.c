/* Copyright 2021 Carlo Caione, <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/ztest.h>
#include <zephyr/ipc/ipc_service.h>

static void received_cb(const void *data, size_t len, void *priv)
{
	uintptr_t expected = (uintptr_t) priv;
	uint8_t *msg = (uint8_t *) data;

	zassert_equal(*msg, expected, "msg doesn't match the expected value");

	printk("Received: %d, expected: %ld\n", *msg, expected);
}

static struct ipc_ept_cfg ept_cfg = {
	.name = "test_ept",
	.cb = {
		.received = received_cb,
	},
};

ZTEST(ipc_service, test_ipc_service)
{
	const struct device *dev_10;
	const struct device *dev_20;
	struct ipc_ept ept_10;
	struct ipc_ept ept_20;
	uint8_t msg;
	int ret;

	dev_10 = DEVICE_DT_GET(DT_NODELABEL(ipc10));
	dev_20 = DEVICE_DT_GET(DT_NODELABEL(ipc20));

	/*
	 * We send 10 through the ipc10 instance so we expect 20 in the
	 * receiving callback (10 + 10 == 20)
	 */
	msg = 10;
	printk("Sending %d\n", msg);

	/* Save the expected result in priv */
	ept_cfg.priv = (void *) 20;

	ret = ipc_service_register_endpoint(dev_10, &ept_10, &ept_cfg);
	zassert_ok(ret, "ipc_service_register_endpoint() failed");

	ret = ipc_service_send(&ept_10, &msg, sizeof(msg));
	zassert_ok(ret, "ipc_service_send() failed");

	/*
	 * We send 10 again this time through the ipc20 instance so we expect
	 * 20 in the receiving callback (10 + 20 == 30)
	 */
	msg = 10;
	printk("Sending %d\n", msg);

	/* Save the expected result in priv */
	ept_cfg.priv = (void *) 30;

	ret = ipc_service_register_endpoint(dev_20, &ept_20, &ept_cfg);
	zassert_ok(ret, "ipc_service_register_endpoint() failed");

	ret = ipc_service_send(&ept_20, &msg, sizeof(msg));
	zassert_ok(ret, "ipc_service_send() failed");

	/*
	 * Deregister the endpoint and ensure that we fail
	 * correctly
	 */
	ret = ipc_service_deregister_endpoint(&ept_10);
	zassert_ok(ret, "ipc_service_deregister_endpoint() failed");

	ret = ipc_service_send(&ept_10, &msg, sizeof(msg));
	zassert_equal(ret, -ENOENT, "ipc_service_send() should return -ENOENT");
}

ZTEST_SUITE(ipc_service, NULL, NULL, NULL, NULL, NULL);
