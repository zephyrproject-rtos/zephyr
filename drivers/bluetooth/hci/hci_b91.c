/*
 * Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_hci_driver_b91
#include "common/log.h"

#include <sys/byteorder.h>
#include <drivers/bluetooth/hci_driver.h>

#include <b91_bt.h>

#define HCI_CMD                 0x01
#define HCI_ACL                 0x02
#define HCI_EVT                 0x04

#define HCI_BT_B91_TIMEOUT K_MSEC(2000)

static K_SEM_DEFINE(hci_send_sem, 1, 1);

static bool is_hci_event_discardable(const uint8_t *evt_data)
{
	uint8_t evt_type = evt_data[0];

	switch (evt_type) {
#if defined(CONFIG_BT_BREDR)
	case BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI:
	case BT_HCI_EVT_EXTENDED_INQUIRY_RESULT:
		return true;
#endif
	case BT_HCI_EVT_LE_META_EVENT: {
		uint8_t subevt_type = evt_data[sizeof(struct bt_hci_evt_hdr)];

		switch (subevt_type) {
		case BT_HCI_EVT_LE_ADVERTISING_REPORT:
			return true;
		default:
			return false;
		}
	}
	default:
		return false;
	}
}

static struct net_buf *bt_b91_evt_recv(uint8_t *data, size_t len)
{
	bool discardable;
	struct bt_hci_evt_hdr hdr;
	struct net_buf *buf;
	size_t buf_tailroom;

	if (len < sizeof(hdr)) {
		BT_ERR("Not enough data for event header");
		return NULL;
	}

	discardable = is_hci_event_discardable(data);

	memcpy((void *)&hdr, data, sizeof(hdr));
	data += sizeof(hdr);
	len -= sizeof(hdr);

	if (len != hdr.len) {
		BT_ERR("Event payload length is not correct");
		return NULL;
	}
	BT_DBG("len %u", hdr.len);

	buf = bt_buf_get_evt(hdr.evt, discardable, K_NO_WAIT);
	if (!buf) {
		if (discardable) {
			BT_DBG("Discardable buffer pool full, ignoring event");
		} else {
			BT_ERR("No available event buffers!");
		}
		return buf;
	}

	net_buf_add_mem(buf, &hdr, sizeof(hdr));

	buf_tailroom = net_buf_tailroom(buf);
	if (buf_tailroom < len) {
		BT_ERR("Not enough space in buffer %zu/%zu",
		       len, buf_tailroom);
		net_buf_unref(buf);
		return NULL;
	}

	net_buf_add_mem(buf, data, len);

	return buf;
}

static struct net_buf *bt_b91_acl_recv(uint8_t *data, size_t len)
{
	struct bt_hci_acl_hdr hdr;
	struct net_buf *buf;
	size_t buf_tailroom;

	if (len < sizeof(hdr)) {
		BT_ERR("Not enough data for ACL header");
		return NULL;
	}

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
	if (buf) {
		memcpy((void *)&hdr, data, sizeof(hdr));
		data += sizeof(hdr);
		len -= sizeof(hdr);
	} else {
		BT_ERR("No available ACL buffers!");
		return NULL;
	}

	if (len != sys_le16_to_cpu(hdr.len)) {
		BT_ERR("ACL payload length is not correct");
		net_buf_unref(buf);
		return NULL;
	}

	net_buf_add_mem(buf, &hdr, sizeof(hdr));
	buf_tailroom = net_buf_tailroom(buf);
	if (buf_tailroom < len) {
		BT_ERR("Not enough space in buffer %zu/%zu",
		       len, buf_tailroom);
		net_buf_unref(buf);
		return NULL;
	}

	BT_DBG("len %u", len);
	net_buf_add_mem(buf, data, len);

	return buf;
}

static void hci_b91_host_rcv_pkt(uint8_t *data, uint16_t len)
{
	uint8_t pkt_indicator;
	struct net_buf *buf;

	BT_HEXDUMP_DBG(data, len, "host packet data:");

	pkt_indicator = *data++;
	len -= sizeof(pkt_indicator);

	switch (pkt_indicator) {
	case HCI_EVT:
		buf = bt_b91_evt_recv(data, len);
		break;

	case HCI_ACL:
		buf = bt_b91_acl_recv(data, len);
		break;

	default:
		buf = NULL;
		BT_ERR("Unknown HCI type %u", pkt_indicator);
	}

	if (buf) {
		BT_DBG("Calling bt_recv(%p)", buf);
		bt_recv(buf);
	}
}

static void hci_b91_controller_rcv_pkt_ready(void)
{
	k_sem_give(&hci_send_sem);
}

static b91_bt_host_callback_t vhci_host_cb = {
	.host_send_available = hci_b91_controller_rcv_pkt_ready,
	.host_read_packet = hci_b91_host_rcv_pkt
};

static int bt_b91_send(struct net_buf *buf)
{
	int err = 0;
	uint8_t type;

	BT_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_OUT:
		type = HCI_ACL;
		break;

	case BT_BUF_CMD:
		type = HCI_CMD;
		break;

	default:
		BT_ERR("Unknown type %u", bt_buf_get_type(buf));
		goto done;
	}

	BT_HEXDUMP_DBG(buf->data, buf->len, "Final HCI buffer:");

	if (k_sem_take(&hci_send_sem, HCI_BT_B91_TIMEOUT) == 0) {
		b91_bt_host_send_packet(type, buf->data, buf->len);
	} else {
		BT_ERR("Send packet timeout error");
		err = -ETIMEDOUT;
	}

done:
	net_buf_unref(buf);
	k_sem_give(&hci_send_sem);

	return err;
}

static int hci_b91_open(void)
{
	int status;

	status = b91_bt_controller_init();
	if (status) {
		BT_ERR("Bluetooth controller init failed %d", status);
		return status;
	}

	b91_bt_host_callback_register(&vhci_host_cb);

	BT_DBG("B91 BT started");

	return 0;
}

static const struct bt_hci_driver drv = {
	.name   = "BT B91",
	.open   = hci_b91_open,
	.send   = bt_b91_send,
	.bus    = BT_HCI_DRIVER_BUS_IPM,
#if defined(CONFIG_BT_DRIVER_QUIRK_NO_AUTO_DLE)
	.quirks = BT_QUIRK_NO_AUTO_DLE,
#endif
};

static int bt_b91_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	bt_hci_driver_register(&drv);

	return 0;
}

SYS_INIT(bt_b91_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
