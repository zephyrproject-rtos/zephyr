/*
 * Copyright 2021 Carlo Caione, <ccaione@baylibre.com>
 * Copyright 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/ztest.h>
#include <zephyr/ipc/ipc_msg_service.h>

static K_SEM_DEFINE(evt_done_sem, 0, 1);

static int received_cb(uint16_t msg_type, const void *msg_data, void *priv)
{
	uintptr_t expected = (uintptr_t)priv;
	const struct ipc_msg_type_cmd *msg = (const struct ipc_msg_type_cmd *)msg_data;

	zassert_equal(msg_type, IPC_MSG_TYPE_CMD, "received incorrect type of message");

	printk("<<< Received cmd: %d, expected: %ld\n", msg->cmd, expected);

	zassert_equal(msg->cmd, expected, "msg doesn't match the expected value");

	return 0;
}

static int event_cb(uint16_t evt_type, const void *evt_data, void *priv)
{
	ARG_UNUSED(evt_data);

	zassert_equal(evt_type, IPC_MSG_EVT_REMOTE_DONE, "received incorrect event");

	k_sem_give(&evt_done_sem);

	return 0;
}

static struct ipc_msg_ept_cfg ept_cfg = {
	.name = "test_ept",
	.cb = {
		.received = received_cb,
		.event = event_cb,
	},
};

ZTEST(ipc_msg_service, test_ipc_msg_service_send)
{
	const struct device *dev_10;
	const struct device *dev_20;
	struct ipc_msg_ept ept_10;
	struct ipc_msg_ept ept_20;
	struct ipc_msg_type_cmd msg;
	int ret;

	dev_10 = DEVICE_DT_GET(DT_NODELABEL(ipc10));
	dev_20 = DEVICE_DT_GET(DT_NODELABEL(ipc20));

	/*
	 * We send 10 through the ipc10 instance so we expect 20 in the
	 * receiving callback (10 + 10 == 20)
	 */
	msg.cmd = 10;
	printk(">>> Sending cmd %d\n", msg.cmd);

	/* Save the expected result in priv */
	ept_cfg.priv = (void *)20;

	ret = ipc_msg_service_register_endpoint(dev_10, &ept_10, &ept_cfg);
	zassert_ok(ret, "ipc_msg_service_register_endpoint() failed");

	ret = ipc_msg_service_send(&ept_10, IPC_MSG_TYPE_CMD, (const void *)&msg);
	zassert_ok(ret, "ipc_msg_service_send() failed");

	zassert_ok(k_sem_take(&evt_done_sem, K_MSEC(100)), "done event not received");

	/*
	 * We send 10 again this time through the ipc20 instance so we expect
	 * 20 in the receiving callback (10 + 20 == 30)
	 */
	msg.cmd = 10;
	printk(">>> Sending cmd %d\n", msg.cmd);

	/* Save the expected result in priv */
	ept_cfg.priv = (void *)30;

	ret = ipc_msg_service_register_endpoint(dev_20, &ept_20, &ept_cfg);
	zassert_ok(ret, "ipc_msg_service_register_endpoint() failed");

	ret = ipc_msg_service_send(&ept_20, IPC_MSG_TYPE_CMD, (const void *)&msg);
	zassert_ok(ret, "ipc_msg_service_send() failed");

	zassert_ok(k_sem_take(&evt_done_sem, K_MSEC(100)), "done event not received");

	/* Deregister the endpoint and ensure that we fail correctly. */
	ret = ipc_msg_service_deregister_endpoint(&ept_10);
	zassert_ok(ret, "ipc_msg_service_deregister_endpoint() failed");

	ret = ipc_msg_service_deregister_endpoint(&ept_20);
	zassert_ok(ret, "ipc_msg_service_deregister_endpoint() failed");
}

ZTEST(ipc_msg_service, test_ipc_msg_endpoint_not_registered)
{
	const struct device *dev_10;
	struct ipc_msg_ept ept_10;
	int ret;

	dev_10 = DEVICE_DT_GET(DT_NODELABEL(ipc10));

	/* Register then de-register endpoint. */
	ret = ipc_msg_service_register_endpoint(dev_10, &ept_10, &ept_cfg);
	zassert_ok(ret, "ipc_msg_service_register_endpoint() failed");

	ret = ipc_msg_service_deregister_endpoint(&ept_10);
	zassert_ok(ret, "ipc_msg_service_deregister_endpoint() failed");

	/* Should fail as endpoint has already been de-registered. */
	ret = ipc_msg_service_send(&ept_10, IPC_MSG_TYPE_CMD, NULL);
	zassert_equal(ret, -ENOENT, "ipc_msg_service_send() should return -ENOENT");
}

ZTEST(ipc_msg_service, test_ipc_msg_wrong_message_type)
{
	const struct device *dev_10;
	struct ipc_msg_ept ept_10;
	struct ipc_msg_type_cmd msg = {.cmd = 10};
	int ret;

	dev_10 = DEVICE_DT_GET(DT_NODELABEL(ipc10));

	ret = ipc_msg_service_register_endpoint(dev_10, &ept_10, &ept_cfg);
	zassert_ok(ret, "ipc_msg_service_register_endpoint() failed");

	/* IPC_MSG_TYPE_CUSTOM_START is not a valid type in this test. */
	ret = ipc_msg_service_send(&ept_10, IPC_MSG_TYPE_CUSTOM_START, (const void *)&msg);
	zassert_equal(ret, -ENOTSUP, "ipc_msg_service_send() should return -ENOTSUP");

	zassert_equal(k_sem_take(&evt_done_sem, K_MSEC(100)), -EAGAIN,
		      "done event received but should not");

	ret = ipc_msg_service_deregister_endpoint(&ept_10);
	zassert_ok(ret, "ipc_msg_service_deregister_endpoint() failed");
}

ZTEST(ipc_msg_service, test_ipc_msg_endpoint_query)
{
	const struct device *dev_10;
	struct ipc_msg_ept ept_10;
	int ret;

	dev_10 = DEVICE_DT_GET(DT_NODELABEL(ipc10));

	/* Since endpointe has never registered, API pointer is not valid, hence -EIO. */
	ret = ipc_msg_service_query(&ept_10, IPC_MSG_QUERY_IS_READY, NULL, NULL);
	zassert_equal(ret, -EIO, "ipc_msg_service_query() should return -EIO");

	ret = ipc_msg_service_register_endpoint(dev_10, &ept_10, &ept_cfg);
	zassert_ok(ret, "ipc_msg_service_register_endpoint() failed");

	ret = ipc_msg_service_query(&ept_10, IPC_MSG_QUERY_IS_READY, NULL, NULL);
	zassert_equal(ret, 0, "ipc_msg_service_query() should return 0");

	ret = ipc_msg_service_deregister_endpoint(&ept_10);
	zassert_ok(ret, "ipc_msg_service_deregister_endpoint() failed");

	/* Now this will -ENOENT as API pointer has been set above. */
	ret = ipc_msg_service_query(&ept_10, IPC_MSG_QUERY_IS_READY, NULL, NULL);
	zassert_equal(ret, -ENOENT, "ipc_msg_service_query() should return -ENOENT");
}

ZTEST_SUITE(ipc_msg_service, NULL, NULL, NULL, NULL, NULL);
