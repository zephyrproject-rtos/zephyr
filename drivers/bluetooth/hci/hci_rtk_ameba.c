/*
 * Copyright (c) 2026 Realtek Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include "hci/hci_common.h"
#include "hci/hci_if_zephyr.h"
#include "hci/hci_transport.h"
#include "hci_platform.h"

#include <zephyr/drivers/bluetooth.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_hci_ameba, CONFIG_BT_HCI_DRIVER_LOG_LEVEL);

#define DT_DRV_COMPAT realtek_ameba_bt_hci

extern bool hci_rx_pkt_free(struct hci_rx_packet_t *pkt);
static void zephyr_recv(struct hci_rx_packet_t *pkt)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct net_buf *buf = NULL;

	if (pkt->type == HCI_EVT) {
		buf = bt_buf_get_evt(((struct bt_hci_evt_hdr *)(pkt->buf))->evt, pkt->discardable,
				     pkt->discardable ? K_NO_WAIT : K_FOREVER);
	} else if (pkt->type == HCI_ACL) {
		buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	} else if (pkt->type == HCI_ISO) {
		buf = bt_buf_get_rx(BT_BUF_ISO_IN, K_FOREVER);
	} else {
		LOG_ERR("unknown pkt type %u, dropping", pkt->type);
		goto end;
	}

	if (!buf) {
		if (!pkt->discardable) {
			LOG_ERR("buf alloc failed for non-discardable pkt type %u", pkt->type);
		}
		goto end;
	}

	if (buf->size < pkt->len) {
		LOG_ERR("buf too small: size=%u pkt_len=%u", buf->size, pkt->len);
		net_buf_unref(buf);
		goto end;
	}

	net_buf_add_mem(buf, pkt->buf, pkt->len);
	bt_hci_recv(dev, buf);

end:
	hci_rx_pkt_free(pkt);
}

static struct hci_transport_cb zephyr_stack_cb = {
	.recv = zephyr_recv,
};

static int hci_open(const struct device *dev)
{
	if (!hci_controller_open()) {
		LOG_ERR("hci_controller_open failed");
		return -EIO;
	}

	/* HCI Transport Bridge to Zephyr Stack */
	hci_transport_register(&zephyr_stack_cb);

	return 0;
}

static int hci_send(const struct device *dev, struct net_buf *buf)
{
	ARG_UNUSED(dev);
	uint8_t type = net_buf_pull_u8(buf);

	switch (type) {
	case HCI_CMD:
	case HCI_ACL:
	case HCI_ISO:
		break;
	default:
		LOG_ERR("unknown H:4 type 0x%02x", type);
		return -EINVAL;
	}

	if (hci_transport_send(type, buf->data, buf->len, true) != buf->len) {
		LOG_ERR("transport send failed");
		return -EIO;
	}

	net_buf_unref(buf);

	return 0;
}

static int hci_close(const struct device *dev)
{
	ARG_UNUSED(dev);

	hci_controller_close();
	hci_controller_free();

	return 0;
}

static DEVICE_API(bt_hci, drv) = {
	.open = hci_open,
	.send = hci_send,
	.close = hci_close,
};

static struct bt_hci_driver_data bt_rtk_data;
static const struct bt_hci_driver_config bt_rtk_config =
	BT_DT_HCI_DRIVER_CONFIG_INST_GET(0);
DEVICE_DT_INST_DEFINE(0, NULL, NULL, &bt_rtk_data, &bt_rtk_config, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &drv)
