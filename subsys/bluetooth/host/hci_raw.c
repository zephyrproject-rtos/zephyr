/* hci_userchan.c - HCI user channel Bluetooth handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_raw.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys_clock.h>
#include <zephyr/toolchain.h>

#include "common/hci_common_internal.h"
#include "hci_raw_internal.h"
#include "monitor.h"

#define LOG_LEVEL CONFIG_BT_HCI_CORE_LOG_LEVEL
LOG_MODULE_REGISTER(bt_hci_raw);

static struct k_fifo *raw_rx;

static bt_buf_rx_freed_cb_t buf_rx_freed_cb;

static void hci_rx_buf_destroy(struct net_buf *buf)
{
	net_buf_destroy(buf);

	if (buf_rx_freed_cb) {
		/* bt_buf_get_rx is used for all types of RX buffers */
		buf_rx_freed_cb(BT_BUF_EVT | BT_BUF_ACL_IN | BT_BUF_ISO_IN);
	}
}

NET_BUF_POOL_FIXED_DEFINE(hci_rx_pool, BT_BUF_RX_COUNT, BT_BUF_RX_SIZE, 0, hci_rx_buf_destroy);
NET_BUF_POOL_FIXED_DEFINE(hci_cmd_pool, BT_BUF_CMD_TX_COUNT,
			  BT_BUF_CMD_SIZE(CONFIG_BT_BUF_CMD_TX_SIZE), 0, NULL);
NET_BUF_POOL_FIXED_DEFINE(hci_acl_pool, CONFIG_BT_BUF_ACL_TX_COUNT,
			  BT_BUF_ACL_SIZE(CONFIG_BT_BUF_ACL_TX_SIZE), 0, NULL);
#if defined(CONFIG_BT_ISO)
NET_BUF_POOL_FIXED_DEFINE(hci_iso_pool, CONFIG_BT_ISO_TX_BUF_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 0, NULL);
#endif /* CONFIG_BT_ISO */

#if DT_HAS_CHOSEN(zephyr_bt_hci)
#define BT_HCI_NODE   DT_CHOSEN(zephyr_bt_hci)
#define BT_HCI_DEV    DEVICE_DT_GET(BT_HCI_NODE)
#define BT_HCI_BUS    BT_DT_HCI_BUS_GET(BT_HCI_NODE)
#define BT_HCI_NAME   BT_DT_HCI_NAME_GET(BT_HCI_NODE)
#else
/* The zephyr,bt-hci chosen property is mandatory, except for unit tests */
BUILD_ASSERT(IS_ENABLED(CONFIG_ZTEST), "Missing DT chosen property for HCI");
#define BT_HCI_DEV    NULL
#define BT_HCI_BUS    0
#define BT_HCI_NAME   ""
#endif

struct bt_dev_raw bt_dev = {
	.hci = BT_HCI_DEV,
};

struct net_buf *bt_buf_get_rx(enum bt_buf_type type, k_timeout_t timeout)
{
	struct net_buf *buf;

	switch (type) {
	case BT_BUF_EVT:
	case BT_BUF_ACL_IN:
	case BT_BUF_ISO_IN:
		break;
	default:
		LOG_ERR("Invalid rx type: %u", type);
		return NULL;
	}

	buf = net_buf_alloc(&hci_rx_pool, timeout);
	if (!buf) {
		return buf;
	}

	net_buf_add_u8(buf, bt_buf_type_to_h4(type));

	return buf;
}

void bt_buf_rx_freed_cb_set(bt_buf_rx_freed_cb_t cb)
{
	buf_rx_freed_cb = cb;
}

struct net_buf *bt_buf_get_tx(enum bt_buf_type type, k_timeout_t timeout,
			      const void *data, size_t size)
{
	struct net_buf_pool *pool;
	struct net_buf *buf;

	switch (type) {
	case BT_BUF_CMD:
		pool = &hci_cmd_pool;
		break;
	case BT_BUF_ACL_OUT:
		pool = &hci_acl_pool;
		break;
#if defined(CONFIG_BT_ISO)
	case BT_BUF_ISO_OUT:
		pool = &hci_iso_pool;
		break;
#endif /* CONFIG_BT_ISO */
	default:
		LOG_ERR("Invalid tx type: %u", type);
		return NULL;
	}

	buf = net_buf_alloc(pool, timeout);
	if (!buf) {
		return buf;
	}

	net_buf_add_u8(buf, bt_buf_type_to_h4(type));

	if (data && size) {
		if (net_buf_tailroom(buf) < size) {
			net_buf_unref(buf);
			return NULL;
		}

		net_buf_add_mem(buf, data, size);
	}

	return buf;
}

struct net_buf *bt_buf_get_evt(uint8_t evt, bool discardable, k_timeout_t timeout)
{
	ARG_UNUSED(evt);
	ARG_UNUSED(discardable);

	return bt_buf_get_rx(BT_BUF_EVT, timeout);
}

int bt_hci_recv(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = buf->data[0];

	ARG_UNUSED(dev);

	LOG_DBG("buf %p len %u", buf, buf->len);

	bt_monitor_send(bt_monitor_opcode(type, BT_MONITOR_RX), buf->data + 1, buf->len - 1);

	k_fifo_put(raw_rx, buf);

	return 0;
}

int bt_send(struct net_buf *buf)
{
	if (buf->len == 0) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	LOG_DBG("buf %p type %u len %u", buf, buf->data[0], buf->len);

	bt_monitor_send(bt_monitor_opcode(buf->data[0], BT_MONITOR_TX),
			buf->data + 1, buf->len - 1);

	return bt_hci_send(bt_dev.hci, buf);
}

int bt_enable_raw(struct k_fifo *rx_queue)
{
	int err;

	LOG_DBG("");

	raw_rx = rx_queue;

	if (!device_is_ready(bt_dev.hci)) {
		LOG_ERR("HCI driver is not ready");
		return -ENODEV;
	}

	bt_monitor_new_index(BT_MONITOR_TYPE_PRIMARY, BT_HCI_BUS, BT_ADDR_ANY, BT_HCI_NAME);

	err = bt_hci_open(bt_dev.hci, bt_hci_recv);
	if (err) {
		LOG_ERR("HCI driver open failed (%d)", err);
		return err;
	}

	LOG_INF("Lower HCI transport: %s", BT_HCI_NAME);
	LOG_INF("Bluetooth enabled in RAW mode");

	return 0;
}
