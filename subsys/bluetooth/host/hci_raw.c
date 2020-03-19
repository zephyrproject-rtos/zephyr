/* hci_userchan.c - HCI user channel Bluetooth handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <sys/atomic.h>

#include <drivers/bluetooth/hci_driver.h>
#include <bluetooth/hci_raw.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_CORE)
#define LOG_MODULE_NAME bt_hci_raw
#include "common/log.h"

#include "hci_ecc.h"
#include "monitor.h"
#include "hci_raw_internal.h"

#define H4_CMD 0x01
#define H4_ACL 0x02
#define H4_SCO 0x03
#define H4_EVT 0x04

static struct k_fifo *raw_rx;

#if defined(CONFIG_BT_HCI_RAW_H4_ENABLE)
static u8_t raw_mode = BT_HCI_RAW_MODE_H4;
#else
static u8_t raw_mode;
#endif

NET_BUF_POOL_FIXED_DEFINE(hci_rx_pool, CONFIG_BT_RX_BUF_COUNT,
			  BT_BUF_RX_SIZE, NULL);

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
		net_buf_reserve(buf, BT_BUF_RESERVE);
		bt_buf_set_type(buf, type);
	}

	return buf;
}

struct net_buf *bt_buf_get_cmd_complete(s32_t timeout)
{
	return bt_buf_get_rx(BT_BUF_EVT, timeout);
}

struct net_buf *bt_buf_get_evt(u8_t evt, bool discardable, s32_t timeout)
{
	return bt_buf_get_rx(BT_BUF_EVT, timeout);
}

static int bt_h4_recv(struct net_buf *buf)
{
	BT_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_IN:
		net_buf_push_u8(buf, H4_ACL);
		break;
	case BT_BUF_EVT:
		net_buf_push_u8(buf, H4_EVT);
		break;
	default:
		BT_ERR("Unknown type %u", bt_buf_get_type(buf));
		net_buf_unref(buf);
		return -EINVAL;
	}

	return 0;
}

int bt_recv(struct net_buf *buf)
{
	BT_DBG("buf %p len %u", buf, buf->len);

	if (IS_ENABLED(CONFIG_BT_HCI_RAW_H4) &&
	    raw_mode == BT_HCI_RAW_MODE_H4) {
		int err = bt_h4_recv(buf);

		if (err) {
			return err;
		}
	}

	bt_monitor_send(bt_monitor_opcode(buf), buf->data, buf->len);

	/* Queue to RAW rx queue */
	net_buf_put(raw_rx, buf);

	return 0;
}

int bt_recv_prio(struct net_buf *buf)
{
	return bt_recv(buf);
}

static int bt_h4_send(struct net_buf *buf)
{
	u8_t type;

	type = net_buf_pull_u8(buf);

	switch (type) {
	case H4_CMD:
		bt_buf_set_type(buf, BT_BUF_CMD);
		break;
	case H4_ACL:
		bt_buf_set_type(buf, BT_BUF_ACL_OUT);
		break;
	default:
		LOG_ERR("Unknown H4 type %u", type);
		return -EINVAL;
	}

	LOG_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);

	return 0;
}

int bt_send(struct net_buf *buf)
{
	BT_DBG("buf %p len %u", buf, buf->len);

	if (IS_ENABLED(CONFIG_BT_HCI_RAW_H4) &&
	    raw_mode == BT_HCI_RAW_MODE_H4) {
		int err = bt_h4_send(buf);

		if (err) {
			return err;
		}
	}

	bt_monitor_send(bt_monitor_opcode(buf), buf->data, buf->len);

	if (IS_ENABLED(CONFIG_BT_TINYCRYPT_ECC)) {
		return bt_hci_ecc_send(buf);
	}

	return bt_dev.drv->send(buf);
}

int bt_hci_raw_set_mode(u8_t mode)
{
	BT_DBG("mode %u", mode);

	if (IS_ENABLED(CONFIG_BT_HCI_RAW_H4)) {
		switch (mode) {
		case BT_HCI_RAW_MODE_PASSTHROUGH:
		case BT_HCI_RAW_MODE_H4:
			raw_mode = mode;
			return 0;
		}
	}

	return -EINVAL;
}

u8_t bt_hci_raw_get_mode(void)
{
	if (IS_ENABLED(CONFIG_BT_HCI_RAW_H4)) {
		return raw_mode;
	}

	return BT_HCI_RAW_MODE_PASSTHROUGH;
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
