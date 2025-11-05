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
#include "rsi_ble_common_config.h"
#define BLE_RF_POWER_INDEX     0x0006
#define BT_OP_VS_RF_POWER_MODE BT_OP(BT_OGF_VS, BLE_RF_POWER_INDEX)
#define BT_LE_MODE             2

static int rsi_bt_driver_send_tx_pwr_vs_cmd(const struct device *dev, uint8_t protocol_mode,
					    uint8_t le_tx_power_index);
static void siwx91x_bt_resp_rcvd(uint16_t status, rsi_ble_event_rcp_rcvd_info_t *resp_buf);

struct hci_config {
	const struct device *nwp_dev;
};

struct hci_data {
	bt_hci_recv_t recv;
	rsi_data_packet_t rsi_data_packet;
};

/**
 * @brief Send RF power mode configuration command to controller
 * @param dev Pointer to the device structure
 * @return 0 on success, negative errno on failure
 */
static int rsi_bt_driver_send_tx_pwr_vs_cmd(const struct device *dev, uint8_t protocol_mode,
					    uint8_t le_tx_power_index)
{
	struct net_buf *buf;
	int err;

	/* Allocate HCI command buffer with timeout */
	buf = bt_hci_cmd_alloc(K_FOREVER);
	if (!buf) {
		LOG_ERR("Failed to allocate HCI command buffer");
		return -ENOMEM;
	}
	net_buf_add_u8(buf, protocol_mode);
	net_buf_add_u8(buf, le_tx_power_index);
	LOG_DBG("Sending RF Power Mode command (OCF 0x%04X) with power index %d",
		BLE_RF_POWER_INDEX, le_tx_power_index);

	err = bt_hci_cmd_send_sync(BT_OP_VS_RF_POWER_MODE, buf, NULL);
	if (err) {
		LOG_ERR("RF Power Mode command failed: %d", err);
		return err;
	}

	LOG_DBG("RF Power Mode configured successfully");
	return 0;
}

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

static int siwx91x_bt_setup(const struct device *dev, const struct bt_hci_setup_params *params)
{
	int err = rsi_bt_driver_send_tx_pwr_vs_cmd(dev, BT_LE_MODE, RSI_BLE_PWR_INX);

	if (err < 0) {
		LOG_ERR("Failed to send RF power config command: %d", err);
		return err;
	}
	return 0;
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

static int siwx91x_bt_init(const struct device *dev)
{
	const struct hci_config *hci_config = dev->config;

	if (!device_is_ready(hci_config->nwp_dev)) {
		LOG_ERR("NWP device not ready");
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(bt_hci, siwx91x_api) = {
	.open = siwx91x_bt_open,
	.send = siwx91x_bt_send,
	.setup = siwx91x_bt_setup,
};

#define HCI_DEVICE_INIT(inst)                                                                      \
	static struct hci_config hci_config_##inst = {                                             \
		.nwp_dev = DEVICE_DT_GET(DT_INST_PARENT(inst))                                     \
	};                                                                                         \
	static struct hci_data hci_data_##inst;                                                    \
	DEVICE_DT_INST_DEFINE(inst, siwx91x_bt_init, NULL, &hci_data_##inst, &hci_config_##inst,   \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &siwx91x_api)

/* Only one instance supported right now */
HCI_DEVICE_INIT(0)
