/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <sys/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <drivers/bluetooth/hci_driver.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_hci_driver
#include "common/log.h"

#define RPMSG_CMD 0x01
#define RPMSG_ACL 0x02
#define RPMSG_SCO 0x03
#define RPMSG_EVT 0x04
#define RPMSG_ISO 0x05

int bt_rpmsg_platform_init(void);
int bt_rpmsg_platform_send(struct net_buf *buf);
int bt_rpmsg_platform_endpoint_is_bound(void);

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
		case BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT:
			return true;
		default:
			return false;
		}
	}
	default:
		return false;
	}
}

static struct net_buf *bt_rpmsg_evt_recv(uint8_t *data, size_t remaining)
{
	bool discardable;
	struct bt_hci_evt_hdr hdr;
	struct net_buf *buf;

	if (remaining < sizeof(hdr)) {
		BT_ERR("Not enough data for event header");
		return NULL;
	}

	discardable = is_hci_event_discardable(data);

	memcpy((void *)&hdr, data, sizeof(hdr));
	data += sizeof(hdr);
	remaining -= sizeof(hdr);

	if (remaining != hdr.len) {
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
	net_buf_add_mem(buf, data, remaining);

	return buf;
}

static struct net_buf *bt_rpmsg_acl_recv(uint8_t *data, size_t remaining)
{
	struct bt_hci_acl_hdr hdr;
	struct net_buf *buf;

	if (remaining < sizeof(hdr)) {
		BT_ERR("Not enough data for ACL header");
		return NULL;
	}

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
	if (buf) {
		memcpy((void *)&hdr, data, sizeof(hdr));
		data += sizeof(hdr);
		remaining -= sizeof(hdr);

		net_buf_add_mem(buf, &hdr, sizeof(hdr));
	} else {
		BT_ERR("No available ACL buffers!");
		return NULL;
	}

	if (remaining != sys_le16_to_cpu(hdr.len)) {
		BT_ERR("ACL payload length is not correct");
		net_buf_unref(buf);
		return NULL;
	}

	BT_DBG("len %u", remaining);
	net_buf_add_mem(buf, data, remaining);

	return buf;
}

static struct net_buf *bt_rpmsg_iso_recv(uint8_t *data, size_t remaining)
{
	struct bt_hci_iso_hdr hdr;
	struct net_buf *buf;

	if (remaining < sizeof(hdr)) {
		BT_ERR("Not enough data for ISO header");
		return NULL;
	}

	buf = bt_buf_get_rx(BT_BUF_ISO_IN, K_NO_WAIT);
	if (buf) {
		memcpy((void *)&hdr, data, sizeof(hdr));
		data += sizeof(hdr);
		remaining -= sizeof(hdr);

		net_buf_add_mem(buf, &hdr, sizeof(hdr));
	} else {
		BT_ERR("No available ISO buffers!");
		return NULL;
	}

	if (remaining != sys_le16_to_cpu(hdr.len)) {
		BT_ERR("ISO payload length is not correct");
		net_buf_unref(buf);
		return NULL;
	}

	BT_DBG("len %zu", remaining);
	net_buf_add_mem(buf, data, remaining);

	return buf;
}

void bt_rpmsg_rx(uint8_t *data, size_t len)
{
	uint8_t pkt_indicator;
	struct net_buf *buf = NULL;
	size_t remaining = len;

	BT_HEXDUMP_DBG(data, len, "RPMsg data:");

	pkt_indicator = *data++;
	remaining -= sizeof(pkt_indicator);

	switch (pkt_indicator) {
	case RPMSG_EVT:
		buf = bt_rpmsg_evt_recv(data, remaining);
		break;

	case RPMSG_ACL:
		buf = bt_rpmsg_acl_recv(data, remaining);
		break;

	case RPMSG_ISO:
		buf = bt_rpmsg_iso_recv(data, remaining);
		break;

	default:
		BT_ERR("Unknown HCI type %u", pkt_indicator);
		return;
	}

	if (buf) {
		BT_DBG("Calling bt_recv(%p)", buf);

		bt_recv(buf);

		BT_HEXDUMP_DBG(buf->data, buf->len, "RX buf payload:");
	}
}

static int bt_rpmsg_send(struct net_buf *buf)
{
	int err;
	uint8_t pkt_indicator;

	BT_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_OUT:
		pkt_indicator = RPMSG_ACL;
		break;
	case BT_BUF_CMD:
		pkt_indicator = RPMSG_CMD;
		break;
	case BT_BUF_ISO_OUT:
		pkt_indicator = RPMSG_ISO;
		break;
	default:
		BT_ERR("Unknown type %u", bt_buf_get_type(buf));
		goto done;
	}
	net_buf_push_u8(buf, pkt_indicator);

	BT_HEXDUMP_DBG(buf->data, buf->len, "Final HCI buffer:");
	err = bt_rpmsg_platform_send(buf);
	if (err < 0) {
		BT_ERR("Failed to send (err %d)", err);
	}

done:
	net_buf_unref(buf);
	return 0;
}

static int bt_rpmsg_open(void)
{
	BT_DBG("");

	while (!bt_rpmsg_platform_endpoint_is_bound()) {
		k_sleep(K_MSEC(1));
	}
	return 0;
}

static const struct bt_hci_driver drv = {
	.name		= "RPMsg",
	.open		= bt_rpmsg_open,
	.send		= bt_rpmsg_send,
	.bus		= BT_HCI_DRIVER_BUS_IPM,
#if defined(CONFIG_BT_DRIVER_QUIRK_NO_AUTO_DLE)
	.quirks         = BT_QUIRK_NO_AUTO_DLE,
#endif
};

static int bt_rpmsg_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	int err;

	err = bt_rpmsg_platform_init();
	if (err < 0) {
		BT_ERR("Failed to initialize BT RPMSG (err %d)", err);
		return err;
	}

	err = bt_hci_driver_register(&drv);
	if (err < 0) {
		BT_ERR("Failed to register BT HIC driver (err %d)", err);
	}

	return err;
}

SYS_INIT(bt_rpmsg_init, POST_KERNEL, CONFIG_RPMSG_SERVICE_EP_REG_PRIORITY);
