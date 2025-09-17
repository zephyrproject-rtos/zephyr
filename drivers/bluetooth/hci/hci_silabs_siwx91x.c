/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/bluetooth.h>

#define DT_DRV_COMPAT silabs_siwx91x_bt_hci
#define LOG_LEVEL     CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci_driver_siwg917);

#include "rsi_ble.h"

static void siwx91x_bt_resp_rcvd(uint16_t status, rsi_ble_event_rcp_rcvd_info_t *resp_buf);

struct hci_data {
	bt_hci_recv_t recv;
	rsi_data_packet_t rsi_data_packet;
};

static int siwx91x_bt_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct hci_data *hci = dev->data;
	int status = rsi_ble_enhanced_gap_extended_register_callbacks(RSI_BLE_ON_RCP_EVENT,
								      (void *)siwx91x_bt_resp_rcvd);

	if (!status) {
		hci->recv = recv;
	}
	return status ? -EIO : 0;
}

static int siwx91x_bt_send(const struct device *dev, struct net_buf *buf)
{
	struct hci_data *hci = dev->data;
	int sc = -EOVERFLOW;

	if (buf->len < sizeof(hci->rsi_data_packet.data)) {
		memcpy(&hci->rsi_data_packet, buf->data, buf->len);
		sc = rsi_bt_driver_send_cmd(RSI_BLE_REQ_HCI_RAW, &hci->rsi_data_packet, NULL);
		/* TODO SILABS ZEPHYR Convert to errno. A common function from rsi/sl_status should
		 * be introduced
		 */
		if (sc) {
			LOG_ERR("BT command send failure: %d", sc);
			sc = -EIO;
		}
	}

	if (sc != 0) {
		return sc;
	}

	net_buf_unref(buf);
	return 0;
}

static void siwx91x_bt_resp_rcvd(uint16_t status, rsi_ble_event_rcp_rcvd_info_t *resp_buf)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct hci_data *hci = dev->data;
	uint8_t packet_type = BT_HCI_H4_NONE;
	size_t len = 0;
	struct net_buf *buf = NULL;

	/* TODO SILABS ZEPHYR This horror expression is from the WiseConnect from the HCI example...
	 * No workaround have been found until now.
	 */
	memcpy(&packet_type, (resp_buf->data - 12), 1);
	switch (packet_type) {
	case BT_HCI_H4_EVT: {
		struct bt_hci_evt_hdr *hdr = (void *)resp_buf->data;

		len = hdr->len + sizeof(*hdr);
		buf = bt_buf_get_evt(hdr->evt, false, K_FOREVER);
		break;
	}
	case BT_HCI_H4_ACL: {
		struct bt_hci_acl_hdr *hdr = (void *)resp_buf->data;

		len = hdr->len + sizeof(*hdr);
		buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
		break;
	}
	default:
		LOG_ERR("Unknown/Unhandled HCI type: %d", packet_type);
		break;
	}

	if (buf && (len <= net_buf_tailroom(buf))) {
		net_buf_add_mem(buf, resp_buf->data, len);
		hci->recv(dev, buf);
	}
}

static DEVICE_API(bt_hci, siwx91x_api) = {
	.open = siwx91x_bt_open,
	.send = siwx91x_bt_send,
};

#define HCI_DEVICE_INIT(inst)                                                                      \
	static struct hci_data hci_data_##inst;                                                    \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &hci_data_##inst, NULL, POST_KERNEL,               \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &siwx91x_api)

/* Only one instance supported right now */
HCI_DEVICE_INIT(0)
