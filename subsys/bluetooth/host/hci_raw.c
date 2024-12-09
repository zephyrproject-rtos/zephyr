/* hci_userchan.c - HCI user channel Bluetooth handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_raw.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/iso.h>

#include <zephyr/bluetooth/hci.h>

#include "monitor.h"
#include "hci_raw_internal.h"

#define LOG_LEVEL CONFIG_BT_HCI_CORE_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci_raw);

static struct k_fifo *raw_rx;

#if defined(CONFIG_BT_HCI_RAW_H4_ENABLE)
static uint8_t raw_mode = BT_HCI_RAW_MODE_H4;
#else
static uint8_t raw_mode;
#endif

static bt_buf_rx_freed_cb_t buf_rx_freed_cb;

static void hci_rx_buf_destroy(struct net_buf *buf)
{
	net_buf_destroy(buf);

	if (buf_rx_freed_cb) {
		/* bt_buf_get_rx is used for all types of RX buffers */
		buf_rx_freed_cb(BT_BUF_EVT | BT_BUF_ACL_IN | BT_BUF_ISO_IN);
	}
}

NET_BUF_POOL_FIXED_DEFINE(hci_rx_pool, BT_BUF_RX_COUNT, BT_BUF_RX_SIZE, sizeof(struct bt_buf_data),
			  hci_rx_buf_destroy);
NET_BUF_POOL_FIXED_DEFINE(hci_cmd_pool, CONFIG_BT_BUF_CMD_TX_COUNT,
			  BT_BUF_CMD_SIZE(CONFIG_BT_BUF_CMD_TX_SIZE),
			  sizeof(struct bt_buf_data), NULL);
NET_BUF_POOL_FIXED_DEFINE(hci_acl_pool, CONFIG_BT_BUF_ACL_TX_COUNT,
			  BT_BUF_ACL_SIZE(CONFIG_BT_BUF_ACL_TX_SIZE),
			  sizeof(struct bt_buf_data), NULL);
#if defined(CONFIG_BT_ISO)
NET_BUF_POOL_FIXED_DEFINE(hci_iso_pool, CONFIG_BT_ISO_TX_BUF_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  sizeof(struct bt_buf_data), NULL);
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
struct bt_hci_raw_cmd_ext *cmd_ext;
static size_t cmd_ext_size;

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

	net_buf_reserve(buf, BT_BUF_RESERVE);
	bt_buf_set_type(buf, type);

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
	case BT_BUF_H4:
		if (IS_ENABLED(CONFIG_BT_HCI_RAW_H4) &&
		    raw_mode == BT_HCI_RAW_MODE_H4) {
			uint8_t h4_type = ((uint8_t *)data)[0];

			switch (h4_type) {
			case BT_HCI_H4_CMD:
				type = BT_BUF_CMD;
				pool = &hci_cmd_pool;
				break;
			case BT_HCI_H4_ACL:
				type = BT_BUF_ACL_OUT;
				pool = &hci_acl_pool;
				break;
#if defined(CONFIG_BT_ISO)
			case BT_HCI_H4_ISO:
				type = BT_BUF_ISO_OUT;
				pool = &hci_iso_pool;
				break;
#endif /* CONFIG_BT_ISO */
			default:
				LOG_ERR("Unknown H4 type %u", h4_type);
				return NULL;
			}

			/* Adjust data pointer to discard the header */
			data = (uint8_t *)data + 1;
			size--;
			break;
		}
		__fallthrough;
	default:
		LOG_ERR("Invalid tx type: %u", type);
		return NULL;
	}

	buf = net_buf_alloc(pool, timeout);
	if (!buf) {
		return buf;
	}

	net_buf_reserve(buf, BT_BUF_RESERVE);
	bt_buf_set_type(buf, type);

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
	return bt_buf_get_rx(BT_BUF_EVT, timeout);
}

int bt_hci_recv(const struct device *dev, struct net_buf *buf)
{
	ARG_UNUSED(dev);

	LOG_DBG("buf %p len %u", buf, buf->len);

	bt_monitor_send(bt_monitor_opcode(buf), buf->data, buf->len);

	if (IS_ENABLED(CONFIG_BT_HCI_RAW_H4) &&
	    raw_mode == BT_HCI_RAW_MODE_H4) {
		switch (bt_buf_get_type(buf)) {
		case BT_BUF_EVT:
			net_buf_push_u8(buf, BT_HCI_H4_EVT);
			break;
		case BT_BUF_ACL_IN:
			net_buf_push_u8(buf, BT_HCI_H4_ACL);
			break;
		case BT_BUF_ISO_IN:
			if (IS_ENABLED(CONFIG_BT_ISO)) {
				net_buf_push_u8(buf, BT_HCI_H4_ISO);
				break;
			}
			__fallthrough;
		default:
			LOG_ERR("Unknown type %u", bt_buf_get_type(buf));
			return -EINVAL;
		}
	}

	/* Queue to RAW rx queue */
	k_fifo_put(raw_rx, buf);

	return 0;
}

static void bt_cmd_complete_ext(uint16_t op, uint8_t status)
{
	struct net_buf *buf;
	struct bt_hci_evt_cc_status *cc;

	if (status == BT_HCI_ERR_EXT_HANDLED) {
		return;
	}

	buf = bt_hci_cmd_complete_create(op, sizeof(*cc));
	cc = net_buf_add(buf, sizeof(*cc));
	cc->status = status;

	bt_hci_recv(bt_dev.hci, buf);
}

static uint8_t bt_send_ext(struct net_buf *buf)
{
	struct bt_hci_cmd_hdr *hdr;
	struct net_buf_simple_state state;
	int i;
	uint16_t op;
	uint8_t status;

	status = BT_HCI_ERR_SUCCESS;

	if (!cmd_ext) {
		return status;
	}

	net_buf_simple_save(&buf->b, &state);

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("No HCI Command header");
		return BT_HCI_ERR_INVALID_PARAM;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	if (buf->len < hdr->param_len) {
		LOG_ERR("Invalid HCI CMD packet length");
		return BT_HCI_ERR_INVALID_PARAM;
	}

	op = sys_le16_to_cpu(hdr->opcode);

	for (i = 0; i < cmd_ext_size; i++) {
		struct bt_hci_raw_cmd_ext *cmd = &cmd_ext[i];

		if (cmd->op == op) {
			if (buf->len < cmd->min_len) {
				status = BT_HCI_ERR_INVALID_PARAM;
			} else {
				status = cmd->func(buf);
			}

			break;
		}
	}

	if (status) {
		bt_cmd_complete_ext(op, status);
		return status;
	}

	net_buf_simple_restore(&buf->b, &state);

	return status;
}

int bt_send(struct net_buf *buf)
{
	LOG_DBG("buf %p len %u", buf, buf->len);

	if (buf->len == 0) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	bt_monitor_send(bt_monitor_opcode(buf), buf->data, buf->len);

	if (IS_ENABLED(CONFIG_BT_HCI_RAW_CMD_EXT) &&
	    bt_buf_get_type(buf) == BT_BUF_CMD) {
		uint8_t status;

		status = bt_send_ext(buf);
		if (status) {
			return status;
		}
	}

	return bt_hci_send(bt_dev.hci, buf);
}

int bt_hci_raw_set_mode(uint8_t mode)
{
	LOG_DBG("mode %u", mode);

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

uint8_t bt_hci_raw_get_mode(void)
{
	if (IS_ENABLED(CONFIG_BT_HCI_RAW_H4)) {
		return raw_mode;
	}

	return BT_HCI_RAW_MODE_PASSTHROUGH;
}

void bt_hci_raw_cmd_ext_register(struct bt_hci_raw_cmd_ext *cmds, size_t size)
{
	if (IS_ENABLED(CONFIG_BT_HCI_RAW_CMD_EXT)) {
		cmd_ext = cmds;
		cmd_ext_size = size;
	}
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

	LOG_INF("Bluetooth enabled in RAW mode");

	return 0;
}
