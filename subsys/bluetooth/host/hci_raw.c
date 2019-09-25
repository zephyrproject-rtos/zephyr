/* hci_userchan.c - HCI user channel Bluetooth handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <sys/atomic.h>

#include <bluetooth/hci_driver.h>
#include <bluetooth/hci_raw.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_CORE)
#define LOG_MODULE_NAME bt_hci_raw
#include "common/log.h"

#include "hci_ecc.h"
#include "monitor.h"
#include "hci_raw_internal.h"

static struct k_fifo *raw_rx;

NET_BUF_POOL_DEFINE(hci_rx_pool, CONFIG_BT_RX_BUF_COUNT,
		    BT_BUF_RX_SIZE, BT_BUF_USER_DATA_MIN, NULL);

struct bt_dev_raw bt_dev;

int bt_hci_driver_register(const struct bt_hci_driver *drv)
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

struct net_buf *bt_buf_get_rx(enum bt_buf_type type, s32_t timeout)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&hci_rx_pool, timeout);

	if (buf) {
		bt_buf_set_type(buf, type);
	}

	return buf;
}

struct net_buf *bt_buf_get_cmd_complete(s32_t timeout)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&hci_rx_pool, timeout);
	if (buf) {
		bt_buf_set_type(buf, BT_BUF_EVT);
	}

	return buf;
}

struct net_buf *bt_buf_get_evt(u8_t evt, bool discardable, s32_t timeout)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&hci_rx_pool, timeout);
	if (buf) {
		bt_buf_set_type(buf, BT_BUF_EVT);
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

	if (IS_ENABLED(CONFIG_BT_TINYCRYPT_ECC)) {
		return bt_hci_ecc_send(buf);
	}

	return bt_dev.drv->send(buf);
}

int bt_enable_raw(struct k_fifo *rx_queue)
{
	const struct bt_hci_driver *drv = bt_dev.drv;
	int err;

	BT_DBG("");

	raw_rx = rx_queue;

	if (!bt_dev.drv) {
		BT_ERR("No HCI driver registered");
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_BT_TINYCRYPT_ECC)) {
		bt_hci_ecc_init();
	}

	err = drv->open();
	if (err) {
		BT_ERR("HCI driver open failed (%d)", err);
		return err;
	}

	BT_INFO("Bluetooth enabled in RAW mode");

	return 0;
}
