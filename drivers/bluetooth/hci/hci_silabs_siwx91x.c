/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/hci_lockstep.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>

#define DT_DRV_COMPAT silabs_siwx91x_bt_hci
#define LOG_LEVEL     CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci_driver_siwg917);

#include "rsi_ble.h"
#include "rsi_ble_common_config.h"
#include "siwx91x_nwp.h"

#define BLE_RF_POWER_INDEX     0x0006
#define BT_OP_VS_RF_POWER_MODE BT_OP(BT_OGF_VS, BLE_RF_POWER_INDEX)
#define BT_LE_MODE             2

static void siwx91x_bt_resp_rcvd(uint16_t status, rsi_ble_event_rcp_rcvd_info_t *resp_buf);

struct hci_config {
	/* bt_hci_driver_config must be first */
	struct bt_hci_driver_config common;
	const struct device *nwp_dev;
};

struct hci_data {
	/* bt_hci_driver_data must be first */
	struct bt_hci_driver_data common;
	struct bt_hci_lockstep lockstep;
	rsi_data_packet_t rsi_data_packet;
};

static int siwx91x_bt_send_raw(const struct device *dev, const uint8_t *pkt, size_t len)
{
	struct hci_data *hci = dev->data;
	int sc;

	if (len >= sizeof(hci->rsi_data_packet.data)) {
		return -EOVERFLOW;
	}

	memcpy(&hci->rsi_data_packet, pkt, len);
	sc = rsi_bt_driver_send_cmd(RSI_BLE_REQ_HCI_RAW, &hci->rsi_data_packet, NULL);
	/* TODO SILABS ZEPHYR Convert to errno. A common function from rsi/sl_status should
	 * be introduced
	 */
	if (sc) {
		LOG_ERR("BT command send failure: %d", sc);
		return -EIO;
	}

	return 0;
}

/**
 * @brief Send RF power mode configuration command to controller
 * @param hci Driver data
 * @return 0 on success, negative errno on failure
 */
static int siwx91x_bt_set_rf_power(struct hci_data *hci, uint8_t protocol_mode,
				   uint8_t le_tx_power_index)
{
	BT_HCI_PKT_CMD_DEFINE(cmd, sizeof(protocol_mode) + sizeof(le_tx_power_index));
	int err;

	net_buf_simple_add_u8(&cmd, protocol_mode);
	net_buf_simple_add_u8(&cmd, le_tx_power_index);
	LOG_DBG("Sending RF Power Mode command (OCF 0x%04X) with power index %d",
		BLE_RF_POWER_INDEX, le_tx_power_index);

	err = bt_hci_lockstep_cmd_send_sync(&hci->lockstep, BT_OP_VS_RF_POWER_MODE, &cmd, NULL);
	if (err != 0) {
		return err;
	}

	LOG_DBG("RF Power Mode configured successfully");
	return 0;
}

static int siwx91x_bt_open(const struct device *dev)
{
	struct hci_data *hci = dev->data;
	int status;

	status = rsi_ble_enhanced_gap_extended_register_callbacks(RSI_BLE_ON_RCP_EVENT,
								  (void *)siwx91x_bt_resp_rcvd);
	if (status != 0) {
		return -EIO;
	}

	return siwx91x_bt_set_rf_power(hci, BT_LE_MODE, RSI_BLE_PWR_INX);
}

static int siwx91x_bt_send(const struct device *dev, struct net_buf *buf)
{
	int err;

	err = siwx91x_bt_send_raw(dev, buf->data, buf->len);
	if (err != 0) {
		return err;
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

		/* Responses to the driver's own commands, sent while opening */
		if (bt_hci_lockstep_feed(&hci->lockstep, buf->data, buf->len)) {
			net_buf_unref(buf);
			return;
		}

		bt_hci_recv(dev, buf);
	}
}

static int siwx91x_bt_init(const struct device *dev)
{
	const struct hci_config *hci_config = dev->config;
	struct hci_data *hci = dev->data;

	if (!device_is_ready(hci_config->nwp_dev)) {
		LOG_ERR("NWP device not ready");
		return -ENODEV;
	}

	bt_hci_lockstep_init(&hci->lockstep, dev, siwx91x_bt_send_raw);

	return 0;
}

static DEVICE_API(bt_hci, siwx91x_api) = {
	.open = siwx91x_bt_open,
	.send = siwx91x_bt_send,
};

#define HCI_DEVICE_INIT(inst)                                                                      \
	static struct hci_config hci_config_##inst = {                                             \
		.common = BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst),                                  \
		.nwp_dev = DEVICE_DT_GET(DT_INST_PARENT(inst))                                     \
	};                                                                                         \
	static struct hci_data hci_data_##inst;                                                    \
	DEVICE_DT_INST_DEFINE(inst, siwx91x_bt_init, NULL, &hci_data_##inst, &hci_config_##inst,   \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &siwx91x_api)

/* Only one instance supported right now */
HCI_DEVICE_INIT(0)
