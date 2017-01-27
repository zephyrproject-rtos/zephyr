/* hci_userchan.c - HCI user channel Bluetooth handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <atomic.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BLUETOOTH_DEBUG_HCI_CORE)
#include <bluetooth/log.h>
#include <bluetooth/hci_driver.h>
#include <bluetooth/hci_raw.h>

#include "hci_ecc.h"
#include "monitor.h"
#include "hci_raw_internal.h"

static struct k_fifo *raw_rx;

NET_BUF_POOL_DEFINE(hci_rx_pool, CONFIG_BLUETOOTH_RX_BUF_COUNT,
		    BT_BUF_RX_SIZE, BT_BUF_USER_DATA_MIN, NULL);

struct bt_dev_raw bt_dev;

int bt_hci_driver_register(struct bt_hci_driver *drv)
{
	if (bt_dev.drv) {
		return -EALREADY;
	}

	if (!drv->open || !drv->send) {
		return -EINVAL;
	}

	bt_dev.drv = drv;

	BT_DBG("Registered %s", drv->name ? drv->name : "");

	bt_monitor_new_index(BT_MONITOR_TYPE_PRIMARY, drv->bus,
			     BT_ADDR_ANY, drv->name ? drv->name : "bt0");

	return 0;
}

struct net_buf *bt_buf_get_rx(int32_t timeout)
{
	return net_buf_alloc(&hci_rx_pool, timeout);
}

struct net_buf *bt_buf_get_evt(uint8_t opcode, int timeout)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&hci_rx_pool, timeout);
	if (buf) {
		bt_buf_set_type(buf, BT_BUF_EVT);
	}

	return buf;
}

struct net_buf *bt_buf_get_acl(int32_t timeout)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&hci_rx_pool, timeout);
	if (buf) {
		bt_buf_set_type(buf, BT_BUF_ACL_IN);
	}

	return buf;
}

int bt_recv(struct net_buf *buf)
{
	BT_DBG("buf %p len %u", buf, buf->len);

	bt_monitor_send(bt_monitor_opcode(buf), buf->data, buf->len);

	/* Queue to RAW rx queue */
	net_buf_put(raw_rx, buf);

	return 0;
}

int bt_recv_prio(struct net_buf *buf)
{
	return bt_recv(buf);
}

int bt_send(struct net_buf *buf)
{
	BT_DBG("buf %p len %u", buf, buf->len);

	bt_monitor_send(bt_monitor_opcode(buf), buf->data, buf->len);

	return bt_dev.drv->send(buf);
}

int bt_enable_raw(struct k_fifo *rx_queue)
{
	struct bt_hci_driver *drv = bt_dev.drv;
	int err;

	BT_DBG("");

	raw_rx = rx_queue;

	if (!bt_dev.drv) {
		BT_ERR("No HCI driver registered");
		return -ENODEV;
	}

	bt_hci_ecc_init();

	err = drv->open();
	if (err) {
		BT_ERR("HCI driver open failed (%d)", err);
		return err;
	}

	BT_INFO("Bluetooth enabled in RAW mode");

	return 0;
}
