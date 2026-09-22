/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <errno.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/sys/byteorder.h>

static enum bt_buf_type freed_buf_type;
static K_SEM_DEFINE(rx_sem, 0, 1);

void bt_buf_rx_freed_cb(enum bt_buf_type type)
{
	freed_buf_type = type;
	k_sem_give(&rx_sem);
}

ZTEST_SUITE(test_buf_data_api, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_buf_data_api, test_buf_freed_cb)
{
	struct net_buf *buf;
	int err;

	bt_buf_rx_freed_cb_set(bt_buf_rx_freed_cb);

	/* Test that the callback is called for the BT_BUF_EVT type */
	buf = bt_buf_get_rx(BT_BUF_EVT, K_NO_WAIT);
	zassert_not_null(buf, "Failed to get event buffer");

	net_buf_unref(buf);
	/* The freed buf cb is called from net_buf_unref, therefore the semaphore should
	 * already by given.
	 */
	err = k_sem_take(&rx_sem, K_NO_WAIT);
	zassert_equal(err, 0, "Timeout while waiting for event buffer to be freed");
	zassert_equal(BT_BUF_EVT, BT_BUF_EVT & freed_buf_type, "Event buffer wasn't freed");

	/* Test that the callback is called for the BT_BUF_ACL_IN type */
	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
	zassert_not_null(buf, "Failed to get ACL buffer");

	net_buf_unref(buf);
	/* The freed buf cb is called from net_buf_unref, therefore the semaphore should
	 * already by given.
	 */
	err = k_sem_take(&rx_sem, K_NO_WAIT);
	zassert_equal(err, 0, "Timeout while waiting for ACL buffer to be freed");
	zassert_equal(BT_BUF_ACL_IN, BT_BUF_ACL_IN & freed_buf_type, "ACL buffer wasn't freed");

	/* Test that the callback is called for the BT_BUF_ISO_IN type */
	buf = bt_buf_get_rx(BT_BUF_ISO_IN, K_NO_WAIT);
	zassert_not_null(buf, "Failed to get ISO buffer");

	net_buf_unref(buf);
	/* The freed buf cb is called from net_buf_unref, therefore the semaphore should
	 * already by given.
	 */
	err = k_sem_take(&rx_sem, K_NO_WAIT);
	zassert_equal(err, 0, "Timeout while waiting for ISO buffer to be freed");
	zassert_equal(BT_BUF_ISO_IN, BT_BUF_ISO_IN & freed_buf_type, "ISO buffer wasn't freed");
}
